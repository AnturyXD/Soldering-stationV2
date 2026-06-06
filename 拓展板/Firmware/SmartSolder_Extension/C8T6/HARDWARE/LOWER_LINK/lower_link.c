#include "lower_link.h"
#include "board_config.h"
#include "lower_uart.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static uint32_t lastRxMs;

static uint8_t ParseNumberAfter(const char *line, const char *key, int16_t *out)
{
    const char *p = strstr(line, key);

    if ((p == 0) || (out == 0))
    {
        return 0U;
    }

    p += strlen(key);
    while ((*p == ' ') || (*p == ':') || (*p == '='))
    {
        p++;
    }

    if ((*p == '-') || isdigit((unsigned char)*p))
    {
        *out = (int16_t)atoi(p);
        return 1U;
    }

    return 0U;
}

static void HandleLine(AppState_t *state, const char *line, uint32_t nowMs)
{
    int16_t value;

    /* 下位机短报文是在线判定依据：收到任意完整文本行即刷新在线时间。 */
    lastRxMs = nowMs;
    state->lastLowerRxMs = nowMs;
    state->lowerOnline = 1U;

    /* 兼容之前ATmega固件的 T:320,S:350,V:24，同时保留TEMP/T=等写法。 */
    if (ParseNumberAfter(line, "TEMP", &value) ||
        ParseNumberAfter(line, "Temp", &value) ||
        ParseNumberAfter(line, "T", &value))
    {
        state->ironTemp = value;
    }

    if (ParseNumberAfter(line, "S", &value))
    {
        state->setTemp = value;
    }

    if (ParseNumberAfter(line, "V", &value))
    {
        state->inputMv = (uint16_t)value;
    }

    strncpy(state->lastDebug, line, sizeof(state->lastDebug) - 1U);
    state->lastDebug[sizeof(state->lastDebug) - 1U] = '\0';
}

void LowerLink_Init(void)
{
    LowerUart_Init();
}

void LowerLink_Task(AppState_t *state, uint32_t nowMs)
{
    char line[LOWER_UART_LINE_MAX];
    uint8_t processed = 0U;

    while ((processed < 8U) && LowerUart_GetLine(line, sizeof(line)))
    {
        processed++;
        HandleLine(state, line, nowMs);
    }

    state->lowerOnline = ((lastRxMs != 0U) && ((nowMs - lastRxMs) <= LOWER_ONLINE_TIMEOUT_MS)) ? 1U : 0U;
}
