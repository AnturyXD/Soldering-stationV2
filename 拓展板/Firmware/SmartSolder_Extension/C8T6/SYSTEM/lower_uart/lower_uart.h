#ifndef __LOWER_UART_H
#define __LOWER_UART_H

#include "stm32f10x.h"

#define LOWER_UART_FRAME_MAX    32U
#define LOWER_UART_LINE_MAX     (LOWER_UART_FRAME_MAX + 1U)
#define LOWER_UART_QUEUE_DEPTH  8U
#define LOWER_UART_RECENT_BYTES 8U

void LowerUart_Init(void);
uint32_t LowerUart_GetRxByteCount(void);
uint32_t LowerUart_GetOverflowCount(void);
uint32_t LowerUart_GetOverrunCount(void);
uint32_t LowerUart_GetFrameErrorCount(void);
uint32_t LowerUart_GetNoiseErrorCount(void);
uint32_t LowerUart_GetParityErrorCount(void);
void LowerUart_GetRecentHex(char *buf, uint16_t maxLen);
uint8_t LowerUart_GetLine(char *buf, uint16_t maxLen);

#endif
