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
    uint8_t heaterPower;
    uint8_t load[LOAD_COUNT];
    char lowerMode;
    int16_t ironTemp;
    int16_t setTemp;
    uint16_t inputMv;
    uint32_t runTimeMs;
    uint32_t lastLowerAnyRxMs;
    uint32_t lastLowerRxMs;
    uint32_t lowerRxBytes;
    uint32_t lowerRxLines;
    uint32_t lowerValidFrames;
    uint32_t lowerInvalidFrames;
    uint32_t lowerOverflowCount;
    uint32_t lowerOverrunCount;
    uint32_t lowerFrameErrorCount;
    uint32_t lowerNoiseErrorCount;
    uint32_t lowerParityErrorCount;
    uint32_t commandCount;
    char ip[16];
    char lowerLastError[24];
    char lowerLastRaw[64];
    char lastDebug[96];
} AppState_t;

void AppState_Init(AppState_t *state);
const char *AppMode_ToText(AppMode_t mode);

#endif
