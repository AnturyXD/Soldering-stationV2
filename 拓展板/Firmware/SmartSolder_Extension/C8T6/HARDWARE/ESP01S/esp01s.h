#ifndef __ESP01S_H
#define __ESP01S_H

#include "stm32f10x.h"
#include "app_state.h"

#define ESP_CMD_SEQ_MAX      16U
#define ESP_CMD_TARGET_MAX   16U

typedef struct
{
    char seq[ESP_CMD_SEQ_MAX];
    char target[ESP_CMD_TARGET_MAX];
    int value;
} EspCommand_t;

void ESP01S_Init(void);
void ESP01S_Task(AppState_t *state, uint32_t nowMs);
void ESP01S_SendState(const AppState_t *state);
void ESP01S_SendAck(const char *seq, uint8_t ok, const char *reason);
uint8_t ESP01S_PopCommand(EspCommand_t *cmd);

#endif
