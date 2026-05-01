#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

#include "utils.h"
#include "stm32g4xx_hal.h"
#include "system/serial_log.h"
#include "system/audio_codec.h"
#include "system/gpio.h"
#include "system/analog_in.h"
#include "system/analog_out.h"
#include "dsp/dsp.h"

static void SystemClock_Config(void);

static void lamp_test(void)
{
    uint32_t t0 = HAL_GetTick();
    uint32_t dt = HAL_GetTick() - t0;
    while (dt < 1000) {
        dt = HAL_GetTick() - t0;
        for (unsigned i = 0; i < 6; i++) {
            gpio_set_led(i, dt > i * 100);
        }
        gpio_update_leds();
        __WFI();
    }
    for (int i = 0; i < 6; i++) {
        gpio_set_led(i, false);
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMAMUX1_CLK_ENABLE();

    gpio_init();
    log_init();
    
    printf("***** Starting up *****\r\n");
    lamp_test();

    if (analog_in_init() != 0) {
        printf("Error: analog in init failed\r\n");
        assert(false);
    }
    if (audio_codec_init() != 0) {
        printf("Error: audio coded init failed\r\n");
        assert(false);
    }
    if (analog_out_init() != 0) {
        printf("Error: analog out init failed\r\n");
        assert(false);
    }

    if (audio_run(dsp_do) != 0) {
        printf("Error: audio streaming failed\r\n");
        assert(false);
    }

    // Enable cycle counter in DWT
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    ITM->LAR = 0xC5ACCE55;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    DWT->CYCCNT = 0;

    int i = 0;
    while (1) {
        const uint32_t t = HAL_GetTick();

        while (HAL_GetTick() < t + 100) {
            gpio_update_leds();
            __WFI();
        }

        i++;
        gpio_set(PIN_LED0, !(i % 5));
        if (!(i % 10)) {
            dsp_dump_stats();
            audio_dump_stats();
        }
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

/* Define some functions to make the linker shut up */
void _close(void)
{
}

void _fstat(void)
{
}

void _isatty(void)
{
}

void _lseek(void)
{
}

void _read(void)
{
}

void _getpid(void)
{
}

void _kill(void)
{
}

void _exit(int status)
{
    (void)status;
    __disable_irq();
    for (int i = 0; i < 6; i++) {
        gpio_set_led(i, true);
    }
    while (1) {
        gpio_update_leds();
        for (int i = 0; i < 100000; i++) {
            __NOP();
        }
        log_emergency();
    }
}
