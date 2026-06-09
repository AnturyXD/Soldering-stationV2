#ifndef __PRESENCE_H
#define __PRESENCE_H

#include "stm32f10x.h"

void Presence_Init(void);
void Presence_Task(uint32_t nowMs);
uint8_t Presence_ReadRaw(void);
uint8_t Presence_IsEnabled(void);
uint8_t Presence_IsDetected(void);

#endif
