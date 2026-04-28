#pragma once

/*
 * Interrupt priorities passed to HAL_NVIC_SetPriority().
 * Lower value indicates higher priority.
 */
static const unsigned prio_sai_dma = 0;
static const unsigned prio_adc_dma = 1;
static const unsigned prio_log_uart = 2;
