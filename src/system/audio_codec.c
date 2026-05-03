#include "audio_codec.h"
#include "stm32g4xx_hal.h"
#include "gpio.h"
#include "dsp/audio_util.h"
#include "utils.h"
#include "priorities.h"

#include <stdio.h>

/*
 * We're using the SAI (I2S) peripheral with DMA. The process is to configure
 * SAI and circular DMA, separately for receive and transmit sides, then start them and
 * let DMA run the shop from then on. We get an interrupt for when the DMA has finished
 * half of the transfer and the complete transfer, giving us a natural way to divide
 * the rx/tx buffers into two parts. When DMA has finished transferring the first half
 * we trigger the DSP code to start filling that half, etc. We use the interrupt from
 * the receive side only because that's when we know for sure we have the data to feed
 * to the DSP algorithms. Buffer underruns aren't handled, they just lead to glitches
 * in the audio.
 */

static int32_t s_out_buffer[2 * 2 * FRAMES_PER_BLOCK] = {};
static int32_t s_in_buffer[2 * 2 * FRAMES_PER_BLOCK] = {};
static void(*s_dsp_fn)(const frame_t * const in, frame_t *const out);

static uint32_t s_idle_time_start = 0;
static uint32_t s_max_idle_time = 0;
static uint32_t s_min_idle_time = UINT32_MAX;

static SAI_HandleTypeDef s_hsai_tx = {
    .Instance = SAI1_Block_A,
    .Init = {
        .AudioMode = SAI_MODEMASTER_TX,
        .Synchro = SAI_ASYNCHRONOUS,
        .OutputDrive = SAI_OUTPUTDRIVE_ENABLE,
        .NoDivider = SAI_MASTERDIVIDER_ENABLE,
        .MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE,
        .FIFOThreshold = SAI_FIFOTHRESHOLD_HF,
        .AudioFrequency = SAI_AUDIO_FREQUENCY_48K,
        .MckOutput = SAI_MCK_OUTPUT_ENABLE,
        .SynchroExt = SAI_SYNCEXT_DISABLE,
        .MonoStereoMode = SAI_STEREOMODE,
        .CompandingMode = SAI_NOCOMPANDING,
        .TriState = SAI_OUTPUT_NOTRELEASED,
    },
};

static SAI_HandleTypeDef s_hsai_rx = {
    .Instance = SAI1_Block_B,
    .Init = {
        .AudioMode = SAI_MODESLAVE_RX,
        .Synchro = SAI_SYNCHRONOUS,
        .OutputDrive = SAI_OUTPUTDRIVE_DISABLE,
        .NoDivider = SAI_MASTERDIVIDER_ENABLE,
        .MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE,
        .FIFOThreshold = SAI_FIFOTHRESHOLD_HF,
        .AudioFrequency = SAI_AUDIO_FREQUENCY_48K,
        .MckOutput = SAI_MCK_OUTPUT_DISABLE,
        .SynchroExt = SAI_SYNCEXT_DISABLE,
        .MonoStereoMode = SAI_STEREOMODE,
        .CompandingMode = SAI_NOCOMPANDING,
        .TriState = SAI_OUTPUT_RELEASED,
    },
};

static DMA_HandleTypeDef s_hdma_tx = {
    .Instance = DMA1_Channel4,
    .Init.Request = DMA_REQUEST_SAI1_A,
    .Init.Direction = DMA_MEMORY_TO_PERIPH,
    .Init.PeriphInc = DMA_PINC_DISABLE,
    .Init.MemInc = DMA_MINC_ENABLE,
    .Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD,
    .Init.MemDataAlignment = DMA_MDATAALIGN_WORD,
    .Init.Mode = DMA_CIRCULAR,
    .Init.Priority = DMA_PRIORITY_HIGH,
};

static DMA_HandleTypeDef s_hdma_rx = {
    .Instance = DMA1_Channel5,
    .Init.Request = DMA_REQUEST_SAI1_B,
    .Init.Direction = DMA_PERIPH_TO_MEMORY,
    .Init.PeriphInc = DMA_PINC_DISABLE,
    .Init.MemInc = DMA_MINC_ENABLE,
    .Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD,
    .Init.MemDataAlignment = DMA_MDATAALIGN_WORD,
    .Init.Mode = DMA_CIRCULAR,
    .Init.Priority = DMA_PRIORITY_HIGH,
};

static void handle_buffers(bool complete)
{
    /* This is called when the receive DMA transfer has transferred the first
     * half of the buffer (complete=false) or the second half (complete=true).
     * We pick up the right data, reformat it to floating point, then send it
     * to the DSP function.
     */

    const uint32_t idle_time = cycle_count() - s_idle_time_start;
    s_max_idle_time = MAX(idle_time, s_max_idle_time);
    s_min_idle_time = MIN(idle_time, s_min_idle_time);

    static frame_t in_frames[FRAMES_PER_BLOCK];
    static frame_t out_frames[FRAMES_PER_BLOCK];

    const int32_t *in_buf = s_in_buffer + (int)complete * 2 * FRAMES_PER_BLOCK;
    int32_t *out_buf = s_out_buffer + (int)complete * 2 * FRAMES_PER_BLOCK;
 
    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
        // 24-bit data in LSB. Shift up and down to sign-extend.
        in_frames[i].s[0] = (float)((in_buf[2*i+0] << 8) >> 8);
        in_frames[i].s[1] = (float)((in_buf[2*i+1] << 8) >> 8);
    }
    
    (void)in_frames;
    s_dsp_fn(in_frames, out_frames);
    
    for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
        out_buf[2*i+0] = float_to_codec(out_frames[i].s[0]);
        out_buf[2*i+1] = float_to_codec(out_frames[i].s[1]);
    }

    s_idle_time_start = cycle_count();
}

void DMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_hdma_rx);
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    handle_buffers(true);
}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    handle_buffers(false);
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
    
    if (HAL_SAI_InitProtocol(&s_hsai_tx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
        return 1;
    }
    if (HAL_SAI_InitProtocol(&s_hsai_rx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_24BIT, 2) != HAL_OK) {
        return 1;
    }
    
    if (HAL_DMA_Init(&s_hdma_tx) != HAL_OK) {
        return 1;
    }
    if (HAL_DMA_Init(&s_hdma_rx) != HAL_OK) {
        return 1;
    }

    __HAL_LINKDMA(&s_hsai_rx, hdmarx, s_hdma_rx);
    __HAL_LINKDMA(&s_hsai_tx, hdmatx, s_hdma_tx);
    HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, prio_sai_dma, 0);
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    
    return 0;
}

int audio_run(void(*dsp_fn)(const frame_t * const in, frame_t *const out))
{
    s_dsp_fn = dsp_fn;
    printf("Starting I2S streaming\r\n");
    
    // Start RX first so we're guaranteed to receive the first word when we start
    // transmitting and don't start out of sync.
    if (HAL_SAI_Receive_DMA(&s_hsai_rx, (uint8_t*)s_in_buffer, ARRAY_SIZE(s_out_buffer)) != HAL_OK) {
        return 1;
    }
    if (HAL_SAI_Transmit_DMA(&s_hsai_tx, (uint8_t*)s_out_buffer, ARRAY_SIZE(s_out_buffer)) != HAL_OK) {
        return 1;
    }

    return 0;
}

void audio_set_dsp_function(void(*dsp_fn)(const frame_t * const in, frame_t *const out))
{
    s_dsp_fn = dsp_fn;
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
