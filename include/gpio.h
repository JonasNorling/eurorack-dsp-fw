#pragma once
#include <stdbool.h>

typedef enum {
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
} GpioPin;

void gpio_init(void);
void gpio_set(GpioPin pin, bool value);
void gpio_toggle(GpioPin pin);
