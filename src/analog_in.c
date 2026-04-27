#include "analog_in.h"
#include "stm32g4xx_hal.h"

#include <stdio.h>

#define CHANNEL_COUNT 5

static volatile uint16_t s_dma_data[CHANNEL_COUNT];

static const struct {
    uint32_t channel;
    uint32_t rank;
} s_channel_info[CHANNEL_COUNT] = {
    {ADC_CHANNEL_1, ADC_REGULAR_RANK_1},
    {ADC_CHANNEL_2, ADC_REGULAR_RANK_2},
    {ADC_CHANNEL_3, ADC_REGULAR_RANK_3},
    //{ADC_CHANNEL_12, ADC_REGULAR_RANK_4},  // ADC2 in 12
    {ADC_CHANNEL_11, ADC_REGULAR_RANK_4},
    //{ADC_CHANNEL_5, ADC_REGULAR_RANK_6},  // ADC3 in 5
    {ADC_CHANNEL_5, ADC_REGULAR_RANK_5},
    //{ADC_CHANNEL_15, ADC_REGULAR_RANK_6},  // ADC2 in 15
};

static ADC_HandleTypeDef s_hadc = {
    .Instance = ADC1,
    .Init = {
        .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4,
        .Resolution = ADC_RESOLUTION_12B,
        .DataAlign = ADC_DATAALIGN_LEFT,
        .GainCompensation = 0,
        .ScanConvMode = ADC_SCAN_ENABLE,
        .EOCSelection = ADC_EOC_SEQ_CONV,
        .LowPowerAutoWait = DISABLE,
        .ContinuousConvMode = ENABLE,
        .NbrOfConversion = CHANNEL_COUNT,
        .DiscontinuousConvMode = DISABLE,
        .ExternalTrigConv = ADC_SOFTWARE_START,
        .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE,
        .DMAContinuousRequests = ENABLE,
        .Overrun = ADC_OVR_DATA_OVERWRITTEN,
        .OversamplingMode = DISABLE,
    },
};

static DMA_HandleTypeDef s_hdma = {
    .Instance = DMA1_Channel1,
    .Init.Request = DMA_REQUEST_ADC1,
    .Init.Direction = DMA_PERIPH_TO_MEMORY,
    .Init.PeriphInc = DMA_PINC_DISABLE,
    .Init.MemInc = DMA_MINC_ENABLE,
    .Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD,
    .Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD,
    .Init.Mode = DMA_CIRCULAR,
    .Init.Priority = DMA_PRIORITY_MEDIUM,
};

void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_hdma);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;
    printf("ADC error\r\n");
}

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

    for (int i = 0; i < CHANNEL_COUNT; i++) {
        ADC_ChannelConfTypeDef channel_config = {
            .Channel = s_channel_info[i].channel,
            .Rank = s_channel_info[i].rank,
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
    }

    if (HAL_ADCEx_Calibration_Start(&s_hadc, ADC_SINGLE_ENDED) != HAL_OK) {
        return 1;
    }

    if (HAL_DMA_Init(&s_hdma) != HAL_OK) {
        return 1;
    }

    __HAL_LINKDMA(&s_hadc, DMA_Handle, s_hdma);
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    if (HAL_ADC_Start_DMA(&s_hadc, (uint32_t*)s_dma_data, CHANNEL_COUNT) != HAL_OK) {
        return 1;
    }
    return 0;
}

float analog_in_get(int n)
{
    return s_dma_data[n] * (1.0f/UINT16_MAX);
}
