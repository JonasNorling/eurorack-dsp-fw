#include "analog_out.h"
#include "stm32g4xx_hal.h"

static DAC_HandleTypeDef s_hdac1 = {
    .Instance = DAC1,
};
static DAC_HandleTypeDef s_hdac2 = {
    .Instance = DAC2,
};

int analog_out_init(void)
{
    __HAL_RCC_DAC1_CLK_ENABLE();
    __HAL_RCC_DAC2_CLK_ENABLE();
    if (HAL_DAC_Init(&s_hdac1) != HAL_OK) {
        return 1;
    }
    if (HAL_DAC_Init(&s_hdac2) != HAL_OK) {
        return 1;
    }

    DAC_ChannelConfTypeDef channel_config = {
        .DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC,
        .DAC_DMADoubleDataMode = DISABLE,
        .DAC_SignedFormat = DISABLE,
        .DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE,
        .DAC_Trigger = DAC_TRIGGER_NONE,
        .DAC_Trigger2 = DAC_TRIGGER_NONE,
        .DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE,
        .DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE,
        .DAC_UserTrimming = DAC_TRIMMING_FACTORY,
    };
    if (HAL_DAC_ConfigChannel(&s_hdac1, &channel_config, DAC_CHANNEL_1) != HAL_OK) {
        return 1;
    }
    if (HAL_DAC_ConfigChannel(&s_hdac1, &channel_config, DAC_CHANNEL_2) != HAL_OK) {
        return 1;
    }
    if (HAL_DAC_ConfigChannel(&s_hdac2, &channel_config, DAC_CHANNEL_1) != HAL_OK) {
        return 1;
    }

    if (HAL_DACEx_SelfCalibrate(&s_hdac1, &channel_config, DAC_CHANNEL_1) != HAL_OK) {
        return 1;
    }
    if (HAL_DACEx_SelfCalibrate(&s_hdac1, &channel_config, DAC_CHANNEL_2) != HAL_OK) {
        return 1;
    }
    if (HAL_DACEx_SelfCalibrate(&s_hdac2, &channel_config, DAC_CHANNEL_1) != HAL_OK) {
        return 1;
    }

    if (HAL_DAC_Start(&s_hdac1, DAC_CHANNEL_1) != HAL_OK) {
        return 1;
    }
    if (HAL_DAC_Start(&s_hdac1, DAC_CHANNEL_2) != HAL_OK) {
        return 1;
    }
    if (HAL_DAC_Start(&s_hdac2, DAC_CHANNEL_1) != HAL_OK) {
        return 1;
    }

    return 0;
}

void analog_out_set(unsigned c1, unsigned c2, unsigned c3)
{
    // Invert 12-bit values to compensate for inverting opamp amplifier
    HAL_DAC_SetValue(&s_hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0xfff-c1);
    HAL_DAC_SetValue(&s_hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 0xfff-c2);
    HAL_DAC_SetValue(&s_hdac2, DAC_CHANNEL_1, DAC_ALIGN_12B_R, 0xfff-c3);
}
