#ifndef __LOWER_UART_H
#define __LOWER_UART_H

#include "stm32f10x.h"

#define LOWER_UART_LINE_MAX     96U
#define LOWER_UART_QUEUE_DEPTH  8U

void LowerUart_Init(void);
void LowerUart_SendString(const char *str);
uint8_t LowerUart_GetLine(char *buf, uint16_t maxLen);

#endif
