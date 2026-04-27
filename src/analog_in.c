#include "analog_in.h"
#include "stm32g4xx_hal.h"

#include <stdio.h>

static float s_values[8] = {};

static ADC_HandleTypeDef s_hadc = {
    .Instance = ADC1,
    .Init = {
        .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4,
        .Resolution = ADC_RESOLUTION_12B,
        .DataAlign = ADC_DATAALIGN_LEFT,
        .GainCompensation = 0,
        .ScanConvMode = ADC_SCAN_DISABLE,
        .EOCSelection = ADC_EOC_SINGLE_CONV,
        .LowPowerAutoWait = DISABLE,
        .ContinuousConvMode = DISABLE,
        .NbrOfConversion = 1,
        .DiscontinuousConvMode = DISABLE,
        .ExternalTrigConv = ADC_SOFTWARE_START,
        .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE,
        .DMAContinuousRequests = DISABLE,
        .Overrun = ADC_OVR_DATA_OVERWRITTEN,
        .OversamplingMode = DISABLE,
    },
};

int analog_in_init(void)
{
    static const RCC_PeriphCLKInitTypeDef adc12_pclock = {
        .PeriphClockSelection = RCC_PERIPHCLK_ADC12,
        .Usart1ClockSelection = RCC_ADC12CLKSOURCE_SYSCLK,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&adc12_pclock) != HAL_OK) {
        return 1;
    }
    __HAL_RCC_ADC12_CLK_ENABLE();
    if (HAL_ADC_Init(&s_hadc) != HAL_OK) {
        return 1;
    }

    ADC_ChannelConfTypeDef channel_config = {
        .Channel = ADC_CHANNEL_1,
        .Rank = ADC_REGULAR_RANK_1,
        .SamplingTime = ADC_SAMPLETIME_24CYCLES_5,
        .SingleDiff = ADC_SINGLE_ENDED,
        .OffsetNumber = ADC_OFFSET_NONE,
        .Offset = 0,
        .OffsetSign = ADC_OFFSET_SIGN_NEGATIVE,
        .OffsetSaturation = DISABLE,
    };
    if (HAL_ADC_ConfigChannel(&s_hadc, &channel_config) != HAL_OK) {
        return 1;
    }

    if (HAL_ADCEx_Calibration_Start(&s_hadc, ADC_SINGLE_ENDED) != HAL_OK) {
        return 1;
    }

    return 0;
}

int analog_in_run(void)
{
    if (HAL_ADC_Start(&s_hadc) != HAL_OK) {
        return 1;
    }
    if (HAL_ADC_PollForConversion(&s_hadc, 100) != HAL_OK) {
        return 1;
    }
    s_values[0] = HAL_ADC_GetValue(&s_hadc);
    return 0;
}

float analog_in_get(int n)
{
    return (0x10000-s_values[n]) * (1.0f/UINT16_MAX);
}
