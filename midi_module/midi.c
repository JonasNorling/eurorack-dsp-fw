#include "midi.h"
#include "stm32g4xx_hal.h"
#include "priorities.h"
#include "gpio.h"
#include "utils.h"
#include "voices.h"

#include <stdint.h>
#include <stdio.h>

static UART_HandleTypeDef s_huart = {
    .Instance = USART3,
    .Init = {
        .BaudRate = 31250,
        .WordLength = UART_WORDLENGTH_8B,
        .StopBits = UART_STOPBITS_1,
        .Parity = UART_PARITY_NONE,
        .ClockPrescaler = UART_PRESCALER_DIV1,
        .Mode = USART_MODE_RX,
    },
};

static uint8_t s_rx_buffer[1];

typedef struct {
    uint8_t current_status;
    uint8_t data[2];
    size_t data_pt;
    bool overrun;  // Got too much data
} midi_parser_state_t;

static midi_parser_state_t s_state;

static void handle(uint8_t b)
{
    if (b & 0x80) {
        switch (b & 0xf0) {
            case 0x80:  // note off
            case 0x90:  // note on
            case 0xe0:  // pitch bend
                s_state.current_status = b;
                break;
            default:
                s_state.current_status = 0;
        }
    }
    else {
        if (s_state.data_pt >= ARRAY_SIZE(s_state.data)) {
            printf("Data overrun, status=%x\r\n", (unsigned)s_state.current_status);
        }
        else {
            s_state.data[s_state.data_pt++] = b;
        }
        switch (s_state.current_status) {
            case 0x80:  // note off
                if (s_state.data_pt == 2) {
                    voices_note_event(s_state.current_status & 0x0f, s_state.data[0], 0);
                    s_state.data_pt = 0;
                }
                break;
            case 0x90:  // note on
                if (s_state.data_pt == 2) {
                    voices_note_event(s_state.current_status & 0x0f, s_state.data[0], s_state.data[1]);
                    s_state.data_pt = 0;
                }
                break;
            case 0xe0:  // pitch bend
                if (s_state.data_pt == 2) {
                    const int16_t bend = ((s_state.data[1] << 7) | s_state.data[0]) - 0x2000;
                    voices_pb_event(s_state.current_status & 0x0f, bend);
                    s_state.data_pt = 0;
                }
                break;
            default:
                // Discard data
                s_state.data_pt = 0;
        }
    }
}

void USART3_IRQHandler(void)
{
    HAL_UART_IRQHandler(&s_huart);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *husart)
{
    handle(s_rx_buffer[0]);
    HAL_UART_Receive_IT(husart, s_rx_buffer, 1);
}

int midi_init(void)
{
    static const RCC_PeriphCLKInitTypeDef usart3_pclock = {
        .PeriphClockSelection = RCC_PERIPHCLK_USART3,
        .Usart1ClockSelection = RCC_USART3CLKSOURCE_PCLK1,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&usart3_pclock) != HAL_OK) {
        return 1;
    }
    __HAL_RCC_USART3_CLK_ENABLE();
    if (HAL_UART_Init(&s_huart) != HAL_OK) {
        return 1;
    }

    HAL_NVIC_SetPriority(USART3_IRQn, prio_midi_uart, 0);
    NVIC_EnableIRQ(USART3_IRQn);
    return 0;
}

int midi_start(void)
{
    if (HAL_UART_Receive_IT(&s_huart, s_rx_buffer, 1) != HAL_OK) {
        return 1;
    }
    return 0;
}