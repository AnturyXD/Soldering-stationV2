#ifndef __DEBUG_UART_H
#define __DEBUG_UART_H

#include "stm32f10x.h"
#include <stdio.h>

void DebugUart_Init(void);
uint16_t DebugUart_Write(const char *str);
uint16_t DebugUart_Printf(const char *fmt, ...);

#endif
