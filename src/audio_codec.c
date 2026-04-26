#include "audio_codec.h"
#include "stm32g4xx_hal.h"

#include <stdio.h>
#include <string.h>

#define SAI_HALF_FIFO_SIZE (4)
#define BYTES_PER_SAMPLE 4
#define SAMPLES_PER_BLOCK (FRAMES_PER_BLOCK * 2)
#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)

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

int audio_run(void(*dsp_fn)(const frame_t * const in, frame_t *const out))
{
    (void)dsp_fn;
	int ret;
	int32_t out_block[SAI_HALF_FIFO_SIZE] = {};
    int32_t in_block[SAI_HALF_FIFO_SIZE] = {};

	printf("Starting I2S streaming\r\n");

	/*
	 * Startup:
	 * HAL_SAI_Transmit() will fill the FIFO then enable the TX SAI.
	 * First call to HAL_SAI_Receive() will enable RX SAI first.
	 */

	ret = HAL_SAI_Transmit(&hsai_tx, (void*)out_block, SAI_HALF_FIFO_SIZE, 0);
	if (ret != HAL_OK) {
		printf("Initial buffer fill failed: %d\r\n", ret);
		return 1;
	}
	ret = HAL_SAI_Transmit(&hsai_tx, (void*)out_block, SAI_HALF_FIFO_SIZE, 0);
	if (ret != HAL_OK) {
		printf("Initial buffer fill 2 failed: %d\r\n", ret);
		return 1;
	}

	while (1) {
		// This is where we're blocking waiting for data
		ret = HAL_SAI_Receive(&hsai_rx, (void*)in_block, SAI_HALF_FIFO_SIZE, 10);
		if (ret != HAL_OK) {
			printf("Audio RX failed: %d\r\n", ret);
			return 1;
		}

		static frame_t in_frames[FRAMES_PER_BLOCK];
		static frame_t out_frames[FRAMES_PER_BLOCK];

		// Fit 24-bit input into 16-bit words
		for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
			in_frames[i].s[0] = in_block[2*i+0] >> 8;
			in_frames[i].s[1] = in_block[2*i+1] >> 8;
		}

        memcpy(out_frames, in_frames, sizeof(out_frames));

        // Fit 16-bit data into 32-bit words, from which 24 LSB are sent
		for (int i = 0; i < FRAMES_PER_BLOCK; i++) {
			out_block[2*i+0] = out_frames[i].s[0] << 8;
			out_block[2*i+1] = out_frames[i].s[1] << 8;
		}

		ret = HAL_SAI_Transmit(&hsai_tx, (void*)out_block, SAI_HALF_FIFO_SIZE, 0);
		if (ret != HAL_OK) {
            printf("Audio TX failed: %d\r\n", ret);
		}
	}

	return 0;
}
