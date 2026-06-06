#ifndef __RELAY_H
#define __RELAY_H

#include "stm32f10x.h"
#include "app_state.h"

void Relay_Init(void);
void Relay_Set(LoadChannel_t channel, uint8_t on);
uint8_t Relay_Get(LoadChannel_t channel);
void Relay_ApplyAll(const uint8_t load[LOAD_COUNT]);
void Relay_AllOff(void);

#endif
