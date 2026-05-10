#include "gpio.h"
#include "stm32g4xx_hal.h"
#include "utils.h"

static const struct {
    GPIO_TypeDef *port;
    GPIO_InitTypeDef init;
} pin_config[] = {
    [PIN_LED0] = {GPIOB, {.Pin=GPIO_PIN_8, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_GPIO1] = {GPIOA, {.Pin=GPIO_PIN_7, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_GPIO2] = {GPIOA, {.Pin=GPIO_PIN_15, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_GPIO3] = {GPIOB, {.Pin=GPIO_PIN_4, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_GPIO4] = {GPIOB, {.Pin=GPIO_PIN_9, .Mode=GPIO_MODE_OUTPUT_PP}},
    [PIN_GPIO5] = {GPIOC, {.Pin=GPIO_PIN_13, .Mode=GPIO_MODE_INPUT}},
    [PIN_GPIO6] = {GPIOC, {.Pin=GPIO_PIN_14, .Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP}},
    [PIN_GPIO7] = {GPIOC, {.Pin=GPIO_PIN_15, .Mode=GPIO_MODE_INPUT, .Pull=GPIO_PULLUP}},
    [PIN_UART_TX] = {GPIOB, {.Pin=GPIO_PIN_6, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF7_USART1}},
    [PIN_UART_RX] = {GPIOB, {.Pin=GPIO_PIN_7, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF7_USART1}},
    [I2S_MCLK] = {GPIOA, {.Pin=GPIO_PIN_3, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF13_SAI1, .Speed=GPIO_SPEED_FREQ_VERY_HIGH}},
    [I2S_BCLK] = {GPIOA, {.Pin=GPIO_PIN_8, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF14_SAI1}},
    [I2S_LRCLK] = {GPIOA, {.Pin=GPIO_PIN_9, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF14_SAI1}},
    [I2S_DOUT] = {GPIOA, {.Pin=GPIO_PIN_10, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF14_SAI1}},
    [I2S_DIN] = {GPIOB, {.Pin=GPIO_PIN_5, .Mode=GPIO_MODE_AF_PP, .Alternate=GPIO_AF12_SAI1}},
    [AIN1] = {GPIOA, {.Pin=GPIO_PIN_0, .Mode=GPIO_MODE_ANALOG}},
    [AIN2] = {GPIOA, {.Pin=GPIO_PIN_1, .Mode=GPIO_MODE_ANALOG}},
    [AIN3] = {GPIOA, {.Pin=GPIO_PIN_2, .Mode=GPIO_MODE_ANALOG}},
    [AIN4] = {GPIOB, {.Pin=GPIO_PIN_2, .Mode=GPIO_MODE_ANALOG}},
    [AIN5] = {GPIOB, {.Pin=GPIO_PIN_12, .Mode=GPIO_MODE_ANALOG}},
    [AIN6] = {GPIOB, {.Pin=GPIO_PIN_13, .Mode=GPIO_MODE_ANALOG}},
    [AIN7] = {GPIOB, {.Pin=GPIO_PIN_14, .Mode=GPIO_MODE_ANALOG}},
    [AIN8] = {GPIOB, {.Pin=GPIO_PIN_15, .Mode=GPIO_MODE_ANALOG}},
    [AOUT1] = {GPIOA, {.Pin=GPIO_PIN_4, .Mode=GPIO_MODE_ANALOG}},
    [AOUT2] = {GPIOA, {.Pin=GPIO_PIN_5, .Mode=GPIO_MODE_ANALOG}},
    [AOUT3] = {GPIOA, {.Pin=GPIO_PIN_6, .Mode=GPIO_MODE_ANALOG}},    
};

volatile bool led_state[6] = {};

void gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    for (size_t i = 0; i < ARRAY_SIZE(pin_config); i++) {
        HAL_GPIO_Init(pin_config[i].port, &pin_config[i].init);
    }
}

void gpio_set(GpioPin pin, bool value)
{
    HAL_GPIO_WritePin(pin_config[pin].port, pin_config[pin].init.Pin, (int)value);
}

void gpio_toggle(GpioPin pin)
{
    HAL_GPIO_TogglePin(pin_config[pin].port, pin_config[pin].init.Pin);
}

bool gpio_get(GpioPin pin)
{
    return HAL_GPIO_ReadPin(pin_config[pin].port, pin_config[pin].init.Pin) == GPIO_PIN_SET;
}

void gpio_update_leds(void)
{
    static bool group;
    // Disable PA7 when changing GPIO state to avoid weird blinking artifacts.
    // FIXME: This still looks shit.
    pin_config[PIN_GPIO1].port->MODER &= ~(1 << (2*7));
    if (group) {
        gpio_set(PIN_GPIO1, false);
        gpio_set(PIN_GPIO2, led_state[3]);
        gpio_set(PIN_GPIO3, led_state[4]);
        gpio_set(PIN_GPIO4, led_state[5]);
    }
    else {
        gpio_set(PIN_GPIO1, true);
        gpio_set(PIN_GPIO2, !led_state[0]);
        gpio_set(PIN_GPIO3, !led_state[1]);
        gpio_set(PIN_GPIO4, !led_state[2]);
    }
    pin_config[PIN_GPIO1].port->MODER |= (1 << (2*7));
    group = !group;
}
