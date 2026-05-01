#include "analog_in.h"
#include "utils.h"
#include "stm32g4xx_hal.h"
#include "priorities.h"

#include <stdio.h>

typedef struct {
    uint32_t channel;
    uint32_t rank;
} channel_info_t;

channel_info_t s_channel_info1[] = {
    {ADC_CHANNEL_1, ADC_REGULAR_RANK_1},  // AIN1
    {ADC_CHANNEL_2, ADC_REGULAR_RANK_2},  // AIN2
    {ADC_CHANNEL_3, ADC_REGULAR_RANK_3},  // AIN3
    {ADC_CHANNEL_11, ADC_REGULAR_RANK_4},  // CV1
    {ADC_CHANNEL_5, ADC_REGULAR_RANK_5},  // CV3
};

channel_info_t s_channel_info2[] = {
    {ADC_CHANNEL_12, ADC_REGULAR_RANK_1},  // AIN4
    {ADC_CHANNEL_15, ADC_REGULAR_RANK_2},  // CV4
};

channel_info_t s_channel_info3[] = {
    {ADC_CHANNEL_5, ADC_REGULAR_RANK_1},  // CV2
};

static ADC_HandleTypeDef s_hadc1 = {
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
        .NbrOfConversion = ARRAY_SIZE(s_channel_info1),
        .DMAContinuousRequests = ENABLE,
        .Overrun = ADC_OVR_DATA_OVERWRITTEN,
        .OversamplingMode = ENABLE,
        .Oversampling = {
            .Ratio = ADC_OVERSAMPLING_RATIO_256,
            .RightBitShift = ADC_RIGHTBITSHIFT_4,
            .TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER,
        }
    },
};

static ADC_HandleTypeDef s_hadc2 = {
    .Instance = ADC2,
    .Init = {
        .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4,
        .Resolution = ADC_RESOLUTION_12B,
        .DataAlign = ADC_DATAALIGN_LEFT,
        .GainCompensation = 0,
        .ScanConvMode = ADC_SCAN_ENABLE,
        .EOCSelection = ADC_EOC_SEQ_CONV,
        .LowPowerAutoWait = DISABLE,
        .ContinuousConvMode = ENABLE,
        .NbrOfConversion = ARRAY_SIZE(s_channel_info2),
        .DMAContinuousRequests = ENABLE,
        .Overrun = ADC_OVR_DATA_OVERWRITTEN,
        .OversamplingMode = ENABLE,
        .Oversampling = {
            .Ratio = ADC_OVERSAMPLING_RATIO_256,
            .RightBitShift = ADC_RIGHTBITSHIFT_4,
            .TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER,
        }
    },
};

static ADC_HandleTypeDef s_hadc3 = {
    .Instance = ADC3,
    .Init = {
        .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4,
        .Resolution = ADC_RESOLUTION_12B,
        .DataAlign = ADC_DATAALIGN_LEFT,
        .GainCompensation = 0,
        .ScanConvMode = ADC_SCAN_ENABLE,
        .EOCSelection = ADC_EOC_SEQ_CONV,
        .LowPowerAutoWait = DISABLE,
        .ContinuousConvMode = ENABLE,
        .NbrOfConversion = ARRAY_SIZE(s_channel_info3),
        .DMAContinuousRequests = ENABLE,
        .Overrun = ADC_OVR_DATA_OVERWRITTEN,
        .OversamplingMode = ENABLE,
        .Oversampling = {
            .Ratio = ADC_OVERSAMPLING_RATIO_256,
            .RightBitShift = ADC_RIGHTBITSHIFT_4,
            .TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER,
        }
    },
};

static DMA_HandleTypeDef s_hdma1 = {
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

static DMA_HandleTypeDef s_hdma2 = {
    .Instance = DMA1_Channel2,
    .Init.Request = DMA_REQUEST_ADC2,
    .Init.Direction = DMA_PERIPH_TO_MEMORY,
    .Init.PeriphInc = DMA_PINC_DISABLE,
    .Init.MemInc = DMA_MINC_ENABLE,
    .Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD,
    .Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD,
    .Init.Mode = DMA_CIRCULAR,
    .Init.Priority = DMA_PRIORITY_MEDIUM,
};

static DMA_HandleTypeDef s_hdma3 = {
    .Instance = DMA1_Channel3,
    .Init.Request = DMA_REQUEST_ADC3,
    .Init.Direction = DMA_PERIPH_TO_MEMORY,
    .Init.PeriphInc = DMA_PINC_DISABLE,
    .Init.MemInc = DMA_MINC_ENABLE,
    .Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD,
    .Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD,
    .Init.Mode = DMA_CIRCULAR,
    .Init.Priority = DMA_PRIORITY_MEDIUM,
};

static volatile uint16_t s_dma_data1[ARRAY_SIZE(s_channel_info1)];
static volatile uint16_t s_dma_data2[ARRAY_SIZE(s_channel_info2)];
static volatile uint16_t s_dma_data3[ARRAY_SIZE(s_channel_info3)];

static const volatile uint16_t *s_channel_data[] = {
    &s_dma_data1[0],
    &s_dma_data1[1],
    &s_dma_data1[2],
    &s_dma_data2[0],
    &s_dma_data1[3],
    &s_dma_data3[0],
    &s_dma_data1[4],
    &s_dma_data2[1],
};


void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_hdma1);
}

void DMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_hdma2);
}

void DMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&s_hdma3);
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
    static const RCC_PeriphCLKInitTypeDef adc345_pclock = {
        .PeriphClockSelection = RCC_PERIPHCLK_ADC345,
        .Usart1ClockSelection = RCC_ADC345CLKSOURCE_SYSCLK,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&adc345_pclock) != HAL_OK) {
        return 1;
    }

    __HAL_RCC_ADC12_CLK_ENABLE();
    __HAL_RCC_ADC345_CLK_ENABLE();

    if (HAL_ADC_Init(&s_hadc1) != HAL_OK) {
        return 1;
    }

    if (HAL_ADC_Init(&s_hadc2) != HAL_OK) {
        return 1;
    }

    if (HAL_ADC_Init(&s_hadc3) != HAL_OK) {
        return 1;
    }

    for (size_t i = 0; i < ARRAY_SIZE(s_channel_info1); i++) {
        ADC_ChannelConfTypeDef channel_config = {
            .Channel = s_channel_info1[i].channel,
            .Rank = s_channel_info1[i].rank,
            .SamplingTime = ADC_SAMPLETIME_24CYCLES_5,
            .SingleDiff = ADC_SINGLE_ENDED,
            .OffsetNumber = ADC_OFFSET_NONE,
            .Offset = 0,
            .OffsetSign = ADC_OFFSET_SIGN_NEGATIVE,
            .OffsetSaturation = DISABLE,
        };
        if (HAL_ADC_ConfigChannel(&s_hadc1, &channel_config) != HAL_OK) {
            return 1;
        }
    }

    for (size_t i = 0; i < ARRAY_SIZE(s_channel_info2); i++) {
        ADC_ChannelConfTypeDef channel_config = {
            .Channel = s_channel_info2[i].channel,
            .Rank = s_channel_info2[i].rank,
            .SamplingTime = ADC_SAMPLETIME_24CYCLES_5,
            .SingleDiff = ADC_SINGLE_ENDED,
            .OffsetNumber = ADC_OFFSET_NONE,
            .Offset = 0,
            .OffsetSign = ADC_OFFSET_SIGN_NEGATIVE,
            .OffsetSaturation = DISABLE,
        };
        if (HAL_ADC_ConfigChannel(&s_hadc2, &channel_config) != HAL_OK) {
            return 1;
        }
    }

    for (size_t i = 0; i < ARRAY_SIZE(s_channel_info3); i++) {
        ADC_ChannelConfTypeDef channel_config = {
            .Channel = s_channel_info3[i].channel,
            .Rank = s_channel_info3[i].rank,
            .SamplingTime = ADC_SAMPLETIME_24CYCLES_5,
            .SingleDiff = ADC_SINGLE_ENDED,
            .OffsetNumber = ADC_OFFSET_NONE,
            .Offset = 0,
            .OffsetSign = ADC_OFFSET_SIGN_NEGATIVE,
            .OffsetSaturation = DISABLE,
        };
        if (HAL_ADC_ConfigChannel(&s_hadc3, &channel_config) != HAL_OK) {
            return 1;
        }
    }

    if (HAL_ADCEx_Calibration_Start(&s_hadc1, ADC_SINGLE_ENDED) != HAL_OK) {
        return 1;
    }
    if (HAL_ADCEx_Calibration_Start(&s_hadc2, ADC_SINGLE_ENDED) != HAL_OK) {
        return 1;
    }
    if (HAL_ADCEx_Calibration_Start(&s_hadc3, ADC_SINGLE_ENDED) != HAL_OK) {
        return 1;
    }

    if (HAL_DMA_Init(&s_hdma1) != HAL_OK) {
        return 1;
    }
    if (HAL_DMA_Init(&s_hdma2) != HAL_OK) {
        return 1;
    }
    if (HAL_DMA_Init(&s_hdma3) != HAL_OK) {
        return 1;
    }

    __HAL_LINKDMA(&s_hadc1, DMA_Handle, s_hdma1);
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, prio_adc_dma, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    __HAL_LINKDMA(&s_hadc2, DMA_Handle, s_hdma2);
    HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, prio_adc_dma, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
    __HAL_LINKDMA(&s_hadc3, DMA_Handle, s_hdma3);
    HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, prio_adc_dma, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

    if (HAL_ADC_Start_DMA(&s_hadc1, (uint32_t*)s_dma_data1, ARRAY_SIZE(s_channel_info1)) != HAL_OK) {
        return 1;
    }
    if (HAL_ADC_Start_DMA(&s_hadc2, (uint32_t*)s_dma_data2, ARRAY_SIZE(s_channel_info2)) != HAL_OK) {
        return 1;
    }
    if (HAL_ADC_Start_DMA(&s_hadc3, (uint32_t*)s_dma_data3, ARRAY_SIZE(s_channel_info3)) != HAL_OK) {
        return 1;
    }
    return 0;
}

float analog_in_get(int n)
{
    return (0x10000 - *s_channel_data[n]) * (1.0f/UINT16_MAX);
}
