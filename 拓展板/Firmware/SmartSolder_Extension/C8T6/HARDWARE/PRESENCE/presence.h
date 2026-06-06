#ifndef __PRESENCE_H
#define __PRESENCE_H

#include "stm32f10x.h"

void Presence_Init(void);
uint8_t Presence_ReadRaw(void);
uint8_t Presence_IsEnabled(void);
uint8_t Presence_IsDetected(void);

#endif
