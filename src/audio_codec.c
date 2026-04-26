#include "audio_codec.h"
#include "stm32g4xx_hal.h"
#include "gpio.h"
#include "audio_util.h"

#include <stdio.h>

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
    return 0;
}

int audio_run(void(*dsp_fn)(const frame_t * const in, frame_t *const out), void(*do_work)())
{
    (void)dsp_fn;
    int ret;
    int32_t out_block[2 * FRAMES_PER_BLOCK] = {};
    int32_t in_block[2 * FRAMES_PER_BLOCK] = {};
    
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
    
    while (1) {
        do_work();
        
        // This is where we're blocking waiting for data
        ret = HAL_SAI_Receive(&hsai_rx, (void*)in_block, 2 * FRAMES_PER_BLOCK, 10);
        if (ret != HAL_OK) {
            printf("Audio RX failed: %d\r\n", ret);
            return 1;
        }
        
        static frame_t in_frames[FRAMES_PER_BLOCK];
        static frame_t out_frames[FRAMES_PER_BLOCK];
        
        for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
            // 24-bit data in LSB. Shift up and down to sign-extend.
            in_frames[i].s[0] = (float)((in_block[2*i+0] << 8) >> 8);
            in_frames[i].s[1] = (float)((in_block[2*i+1] << 8) >> 8);
        }
        
        gpio_set(PIN_GPIO2, true);
        dsp_fn(in_frames, out_frames);
        gpio_set(PIN_GPIO2, false);
        
        for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
            out_block[2*i+0] = float_to_codec(out_frames[i].s[0]);
            out_block[2*i+1] = float_to_codec(out_frames[i].s[1]);
        }
        
        gpio_set(PIN_GPIO1, true);
        ret = HAL_SAI_Transmit(&hsai_tx, (void*)out_block, 2 * FRAMES_PER_BLOCK, 0);
        if (ret != HAL_OK) {
            printf("Audio TX failed: %d\r\n", ret);
            return 1;
        }
        gpio_set(PIN_GPIO1, false);
    }
    
    return 0;
}
