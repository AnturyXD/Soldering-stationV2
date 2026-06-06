#ifndef __ESP_UART_H
#define __ESP_UART_H

#include "stm32f10x.h"

#define ESP_UART_LINE_MAX      160U
#define ESP_UART_QUEUE_DEPTH   12U

void EspUart_Init(void);
void EspUart_SendString(const char *str);
uint8_t EspUart_GetLine(char *buf, uint16_t maxLen);

#endif
