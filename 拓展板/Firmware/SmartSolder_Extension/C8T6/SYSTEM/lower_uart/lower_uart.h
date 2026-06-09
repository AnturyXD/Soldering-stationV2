#ifndef __LOWER_UART_H
#define __LOWER_UART_H

#include "stm32f10x.h"

#define LOWER_UART_LINE_MAX     96U
#define LOWER_UART_QUEUE_DEPTH  8U
#define LOWER_UART_RECENT_BYTES 8U

void LowerUart_Init(void);
uint32_t LowerUart_GetRxByteCount(void);
uint32_t LowerUart_GetOverflowCount(void);
void LowerUart_GetRecentHex(char *buf, uint16_t maxLen);
uint8_t LowerUart_GetLine(char *buf, uint16_t maxLen);

#endif
