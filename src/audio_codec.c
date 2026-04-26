#include "audio_codec.h"
#include "stm32g4xx_hal.h"
#include "gpio.h"
#include "audio_util.h"
#include "utils.h"

#include <stdio.h>

static int32_t out_block[2 * FRAMES_PER_BLOCK] = {};
static int32_t in_block[2 * FRAMES_PER_BLOCK] = {};
static frame_t in_frames[FRAMES_PER_BLOCK];
static frame_t out_frames[FRAMES_PER_BLOCK];
static void(*s_dsp_fn)(const frame_t * const in, frame_t *const out);

static uint32_t s_idle_time_start = 0;
static uint32_t s_max_idle_time = 0;
static uint32_t s_min_idle_time = UINT32_MAX;

static SAI_HandleTypeDef hsai_tx = {
    .Instance = SAI1_Block_A,
    .Init = {
        .AudioMode = SAI_MODEMASTER_TX,
        .Synchro = SAI_ASYNCHRONOUS,
        .OutputDrive = SAI_OUTPUTDRIVE_ENABLE,
        .NoDivider = SAI_MASTERDIVIDER_ENABLE,
        .MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE,
        .FIFOThreshold = SAI_FIFOTHRESHOLD_1QF,
        .AudioFrequency = SAI_AUDIO_FREQUENCY_48K,
        .MckOutput = SAI_MCK_OUTPUT_ENABLE,
        .SynchroExt = SAI_SYNCEXT_DISABLE,
        .MonoStereoMode = SAI_STEREOMODE,
        .CompandingMode = SAI_NOCOMPANDING,
        .TriState = SAI_OUTPUT_NOTRELEASED,
    },
};

static SAI_HandleTypeDef hsai_rx = {
    .Instance = SAI1_Block_B,
    .Init = {
        .AudioMode = SAI_MODESLAVE_RX,
        .Synchro = SAI_SYNCHRONOUS,
        .OutputDrive = SAI_OUTPUTDRIVE_DISABLE,
        .NoDivider = SAI_MASTERDIVIDER_ENABLE,
        .MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE,
        .FIFOThreshold = SAI_FIFOTHRESHOLD_1QF,
        .AudioFrequency = SAI_AUDIO_FREQUENCY_48K,
        .MckOutput = SAI_MCK_OUTPUT_DISABLE,
        .SynchroExt = SAI_SYNCEXT_DISABLE,
        .MonoStereoMode = SAI_STEREOMODE,
        .CompandingMode = SAI_NOCOMPANDING,
        .TriState = SAI_OUTPUT_RELEASED,
    },
};

void SAI1_IRQHandler(void)
{
    HAL_SAI_IRQHandler(&hsai_rx);
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    const uint32_t idle_time = cycle_count() - s_idle_time_start;
    s_max_idle_time = MAX(idle_time, s_max_idle_time);
    s_min_idle_time = MIN(idle_time, s_min_idle_time);

    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
        // 24-bit data in LSB. Shift up and down to sign-extend.
        in_frames[i].s[0] = (float)((in_block[2*i+0] << 8) >> 8);
        in_frames[i].s[1] = (float)((in_block[2*i+1] << 8) >> 8);
    }
    
    s_dsp_fn(in_frames, out_frames);
    
    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
        out_block[2*i+0] = float_to_codec(out_frames[i].s[0]);
        out_block[2*i+1] = float_to_codec(out_frames[i].s[1]);
    }
    
    HAL_StatusTypeDef ret = HAL_SAI_Transmit(&hsai_tx, (void*)out_block, 2 * FRAMES_PER_BLOCK, 0);
    if (ret != HAL_OK) {
        printf("Audio TX failed: %d\r\n", ret);
    }

    ret = HAL_SAI_Receive_IT(&hsai_rx, (void*)in_block, 2 * FRAMES_PER_BLOCK);
    if (ret != HAL_OK) {
        printf("Audio RX failed: %d\r\n", ret);
    }

    s_idle_time_start = cycle_count();
}

void HAL_SAI_ErrorCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    printf("SAI error\r\n");
}

int audio_codec_init(void)
{
    static const RCC_PeriphCLKInitTypeDef sai1_pclock = {
        .PeriphClockSelection = RCC_PERIPHCLK_SAI1,
        .Usart1ClockSelection = RCC_SAI1CLKSOURCE_PLL,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&sai1_pclock) != HAL_OK) {
        return 1;
    }
    
    __HAL_RCC_SAI1_CLK_ENABLE();
    
    if (HAL_SAI_InitProtocol(&hsai_tx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
        return 1;
    }
    if (HAL_SAI_InitProtocol(&hsai_rx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
        return 1;
    }
    
    NVIC_SetPriority(SAI1_IRQn, 0x80);
    NVIC_EnableIRQ(SAI1_IRQn);
    
    return 0;
}

int audio_run(void(*dsp_fn)(const frame_t * const in, frame_t *const out))
{
    int ret;
    
    s_dsp_fn = dsp_fn;
    printf("Starting I2S streaming\r\n");
    
    /*
    * Startup:
    * HAL_SAI_Transmit() will fill the FIFO then enable the TX SAI.
    * First call to HAL_SAI_Receive() will enable RX SAI first.
    */
    
    ret = HAL_SAI_Transmit(&hsai_tx, (void*)out_block, 2 * FRAMES_PER_BLOCK, 0);
    if (ret != HAL_OK) {
        printf("Initial buffer fill failed: %d\r\n", ret);
        return 1;
    }
    ret = HAL_SAI_Transmit(&hsai_tx, (void*)out_block, 2 * FRAMES_PER_BLOCK, 0);
    if (ret != HAL_OK) {
        printf("Initial buffer fill 2 failed: %d\r\n", ret);
        return 1;
    }
    
    // Get the party started. The interrupt handler will take over later on.
    ret = HAL_SAI_Receive_IT(&hsai_rx, (void*)in_block, 2 * FRAMES_PER_BLOCK);
    if (ret != HAL_OK) {
        printf("Audio RX failed: %d\r\n", ret);
        return 1;
    }
    
    return 0;
}

void audio_dump_stats(void)
{
    #define CPU_FREQ 159744000
    static const int cycles_per_block = FRAMES_PER_BLOCK * CPU_FREQ / SAMPLE_RATE;
    int min_idle = 100 * s_min_idle_time / cycles_per_block;
    int max_idle = 100 * s_max_idle_time / cycles_per_block;
    printf("DSP idle %d%%-%d%%\r\n", min_idle, max_idle);
    s_max_idle_time = 0;
    s_min_idle_time = UINT32_MAX;
}
