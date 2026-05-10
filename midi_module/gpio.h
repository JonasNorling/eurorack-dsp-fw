#pragma once
#include <stdbool.h>

extern volatile bool led_state[6];

typedef enum {
    PIN_LED0 = 0,
    PIN_GPIO1,
    PIN_GPIO2,
    PIN_GPIO3,
    PIN_GPIO4,
    PIN_GPIO5,
    PIN_GPIO6,
    PIN_GPIO7,
    PIN_UART_TX,
    PIN_UART_RX,
    I2S_MCLK,
    I2S_BCLK,
    I2S_LRCLK,
    I2S_DOUT,
    I2S_DIN,
    AIN1,
    AIN2,
    AIN3,
    AIN4,
    AIN5,
    AIN6,
    AIN7,
    AIN8,
    AOUT1,
    AOUT2,
    AOUT3,
} GpioPin;

#define PIN_BUTTON_1 PIN_GPIO7
#define PIN_BUTTON_2 PIN_GPIO6

void gpio_init(void);
void gpio_set(GpioPin pin, bool value);
void gpio_toggle(GpioPin pin);
bool gpio_get(GpioPin pin);

static inline void gpio_set_led(int n, bool on)
{
    led_state[n] = on;
}

// Call every millisecond or so
void gpio_update_leds(void);
