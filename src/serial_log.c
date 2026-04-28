#include "serial_log.h"
#include "stm32g4xx_hal.h"
#include "utils.h"
#include "priorities.h"

#include <unistd.h>
#include <string.h>

static USART_HandleTypeDef s_huart = {
    .Instance = USART1,
    .Init = {
        .BaudRate = 115200,
        .WordLength = UART_WORDLENGTH_8B,
        .StopBits = UART_STOPBITS_1,
        .Parity = UART_PARITY_NONE,
        .ClockPrescaler = UART_PRESCALER_DIV1,
        .Mode = USART_MODE_TX_RX,
    },
};

#define s_buflen 1024
static uint8_t s_buffer[s_buflen];
static ssize_t s_writepos = 0;
static ssize_t s_readpos = 0;

void USART1_IRQHandler(void)
{
    HAL_USART_IRQHandler(&s_huart);
}

void HAL_USART_TxCpltCallback(USART_HandleTypeDef *husart)
{
    (void)husart;
    ssize_t read_len = s_writepos >= s_readpos ? s_writepos - s_readpos : s_buflen - s_readpos;
    if (HAL_USART_Transmit_IT(&s_huart, &s_buffer[s_readpos], read_len) == HAL_OK) {
        s_readpos = (s_readpos + read_len) % s_buflen;
    }
}

void log_init(void)
{
    // Configure USART1 as "console"
    static const RCC_PeriphCLKInitTypeDef usart1_pclock = {
        .PeriphClockSelection = RCC_PERIPHCLK_USART1,
        .Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&usart1_pclock) != HAL_OK) {
        while (1)
            ;
    }
    __HAL_RCC_USART1_CLK_ENABLE();
    if (HAL_USART_Init(&s_huart) != HAL_OK) {
        while (1)
            ;
    }

    HAL_NVIC_SetPriority(USART1_IRQn, prio_log_uart, 0);
    NVIC_EnableIRQ(USART1_IRQn);
}

ssize_t _write(int fd, const void *buf, size_t count)
{
    (void)fd;
    // FIXME: Limit write to one byte before read-pos
    ssize_t write_len = MIN((ssize_t)count, s_buflen - s_writepos);
    memcpy(&s_buffer[s_writepos], buf, write_len);
    s_writepos = (s_writepos + write_len) % s_buflen;

    // FIXME: Might want to disable the USART interrupt here
    ssize_t read_len = s_writepos >= s_readpos ? s_writepos - s_readpos : s_buflen - s_readpos;
    if (HAL_USART_Transmit_IT(&s_huart, &s_buffer[s_readpos], read_len) == HAL_OK) {
        s_readpos = (s_readpos + read_len) % s_buflen;
    }
    return write_len;
}
