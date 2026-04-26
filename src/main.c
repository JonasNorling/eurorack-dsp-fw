#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "stm32g4xx_hal.h"
#include "utils.h"
#include "serial_log.h"
#include "audio_codec.h"

static void SystemClock_Config(void);

enum pin {
    PIN_LED0 = 0,
    PIN_GPIO1,
    PIN_GPIO2,
    PIN_GPIO3,
    PIN_GPIO4,
    PIN_UART_TX,
    PIN_UART_RX,
    I2S_MCLK,
    I2S_BCLK,
    I2S_LRCLK,
    I2S_DOUT,
    I2S_DIN,
};

static const struct {
    GPIO_TypeDef *port;
    GPIO_InitTypeDef init;
} pin_config[] = {
    [PIN_LED0] = {GPIOB, {.Pin=GPIO_PIN_8, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_GPIO1] = {GPIOA, {.Pin=GPIO_PIN_7, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_GPIO2] = {GPIOA, {.Pin=GPIO_PIN_15, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_GPIO3] = {GPIOB, {.Pin=GPIO_PIN_4, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_GPIO4] = {GPIOB, {.Pin=GPIO_PIN_9, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_UART_TX] = {GPIOB, {.Pin=GPIO_PIN_6, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF7_USART1}},
    [PIN_UART_RX] = {GPIOB, {.Pin=GPIO_PIN_7, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF7_USART1}},
    [I2S_MCLK] = {GPIOA, {.Pin=GPIO_PIN_3, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF13_SAI1, .Speed=GPIO_SPEED_FREQ_VERY_HIGH}},
    [I2S_BCLK] = {GPIOA, {.Pin=GPIO_PIN_8, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF14_SAI1}},
    [I2S_LRCLK] = {GPIOA, {.Pin=GPIO_PIN_9, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF14_SAI1}},
    [I2S_DOUT] = {GPIOA, {.Pin=GPIO_PIN_10, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF14_SAI1}},
    [I2S_DIN] = {GPIOB, {.Pin=GPIO_PIN_5, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF12_SAI1}},
};


int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    // Configure pins
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    for (size_t i = 0; i < ARRAY_SIZE(pin_config); i++) {
        HAL_GPIO_Init(pin_config[i].port, &pin_config[i].init);
    }
    
    log_init();
    printf("***** Starting up *****\r\n");

    if (audio_codec_init() != 0) {
        printf("Error: audio coded init failed\r\n");
    }

    if (audio_run(NULL) != 0) {
        printf("Error: audio streaming init failed\r\n");
    }

    while (1) {
        uint32_t t0 = HAL_GetTick();
        while (HAL_GetTick() < t0 + 100)
        ;
        HAL_GPIO_WritePin(pin_config[PIN_LED0].port, pin_config[PIN_LED0].init.Pin, GPIO_PIN_SET);
        t0 = HAL_GetTick();
        while (HAL_GetTick() < t0 + 100)
        ;
        HAL_GPIO_WritePin(pin_config[PIN_LED0].port, pin_config[PIN_LED0].init.Pin, GPIO_PIN_RESET);
        printf(".");
        fflush(0);
    }
}

static void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    
    /* Enable voltage range 1 boost mode for frequency above 150 Mhz */
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
    __HAL_RCC_PWR_CLK_DISABLE();
    
    /* Activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM            = RCC_PLLM_DIV1;
    RCC_OscInitStruct.PLL.PLLN            = 26;  // Output 2*159.744 MHz from PLL
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV7;  // To ADC clock
    RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;  // To SAI (I2S) 159.744 MHz, set MCKDIV=13, MCLK=256 -> 48kHz
    RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;  // To CPU core 159.744 MHz
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* Initialization Error */
        while(1);
    }
    
    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
    clocks dividers */
    RCC_ClkInitStruct.ClockType           = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | \
        RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
        RCC_ClkInitStruct.SYSCLKSource        = RCC_SYSCLKSOURCE_PLLCLK;
        RCC_ClkInitStruct.AHBCLKDivider       = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB1CLKDivider      = RCC_HCLK_DIV1;
        RCC_ClkInitStruct.APB2CLKDivider      = RCC_HCLK_DIV1;
        if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_8) != HAL_OK)
        {
            /* Initialization Error */
            while(1);
        }
    }
    