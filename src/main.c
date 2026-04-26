#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "stm32g4xx_hal.h"
#include "utils.h"
#include "serial_log.h"
#include "audio_codec.h"
#include "dsp.h"
#include "gpio.h"

static void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    gpio_init();
    log_init();
    printf("***** Starting up *****\r\n");

    if (audio_codec_init() != 0) {
        printf("Error: audio coded init failed\r\n");
    }

    if (audio_run(dsp_do) != 0) {
        printf("Error: audio streaming failed\r\n");
    }

    // Enable cycle counter in DWT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    ITM->LAR = 0xC5ACCE55;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;

    int i = 0;
    while (1) {
        const uint32_t t = HAL_GetTick();

        while (HAL_GetTick() < t + 1000)
            ;

        i++;
        gpio_toggle(PIN_LED0);
        gpio_set(PIN_GPIO1, (i/3) % 2);
        gpio_set(PIN_GPIO2, !((i-0) % 3));
        gpio_set(PIN_GPIO3, !((i-1) % 3));
        gpio_set(PIN_GPIO4, !((i-2) % 3));
        dsp_dump_stats();
        audio_dump_stats();
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
    