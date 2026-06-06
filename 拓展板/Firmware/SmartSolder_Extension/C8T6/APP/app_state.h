#ifndef __APP_STATE_H
#define __APP_STATE_H

#include "stm32f10x.h"

typedef enum
{
    APP_MODE_MANUAL = 0,
    APP_MODE_AUTO = 1
} AppMode_t;

typedef enum
{
    LOAD_LIGHT = 0,
    LOAD_FAN,
    LOAD_AUX,
    LOAD_COUNT
} LoadChannel_t;

typedef struct
{
    AppMode_t mode;
    uint8_t autoFeatureEnabled;
    uint8_t espOnline;
    uint8_t webOnline;
    uint8_t lowerOnline;
    uint8_t presenceKnown;
    uint8_t presenceDetected;
    uint8_t load[LOAD_COUNT];
    int16_t ironTemp;
    int16_t setTemp;
    uint16_t inputMv;
    uint32_t runTimeMs;
    uint32_t lastLowerRxMs;
    uint32_t commandCount;
    char ip[16];
    char lastDebug[96];
} AppState_t;

void AppState_Init(AppState_t *state);
const char *AppMode_ToText(AppMode_t mode);

#endif
