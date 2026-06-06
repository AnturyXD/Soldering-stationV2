#ifndef __LOWER_LINK_H
#define __LOWER_LINK_H

#include "stm32f10x.h"
#include "app_state.h"

void LowerLink_Init(void);
void LowerLink_Task(AppState_t *state, uint32_t nowMs);

#endif
