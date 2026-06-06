#ifndef __DISPLAY_UI_H
#define __DISPLAY_UI_H

#include "app_state.h"

void DisplayUi_Init(const AppState_t *state);
void DisplayUi_Task(const AppState_t *state, uint32_t nowMs);

#endif
