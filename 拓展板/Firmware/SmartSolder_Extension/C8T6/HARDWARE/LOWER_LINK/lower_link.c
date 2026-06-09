#include "lower_link.h"
#include "board_config.h"
#include "lower_uart.h"
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int16_t currentTemp;
    int16_t setTemp;
    uint8_t powerPercent;
    char mode;
} LowerStatusFrame_t;

static uint32_t lastValidRxMs;
static uint32_t lastSeenRxBytes;
static uint32_t lastSeenOverflow;

static void CopyDebugText(char *dst, uint8_t dstLen, const char *src)
{
    uint8_t i;
    char ch;

    if ((dst == 0) || (dstLen == 0U))
    {
        return;
    }

    if (src == 0)
    {
        dst[0] = '\0';
        return;
    }

    for (i = 0U; i < (dstLen - 1U) && src[i] != '\0'; i++)
    {
        ch = src[i];
        if ((ch < ' ') || (ch > '~') || (ch == '"') || (ch == '\\'))
        {
            ch = '?';
        }
        dst[i] = ch;
    }
    dst[i] = '\0';
}

static void SetLowerError(AppState_t *state, const char *reason, const char *line)
{
    CopyDebugText(state->lowerLastError, sizeof(state->lowerLastError), reason);
    CopyDebugText(state->lowerLastRaw, sizeof(state->lowerLastRaw), line);
}

static void SetLowerRxActivityError(AppState_t *state, const char *reason)
{
    CopyDebugText(state->lowerLastError, sizeof(state->lowerLastError), reason);
    LowerUart_GetRecentHex(state->lowerLastRaw, sizeof(state->lowerLastRaw));
}

static uint8_t ParseNumber(const char **cursor, int16_t *value, char delimiter)
{
    char *end;
    long parsed;

    if ((cursor == 0) || (*cursor == 0) || (value == 0))
    {
        return 0U;
    }

    parsed = strtol(*cursor, &end, 10);
    if ((end == *cursor) || (*end != delimiter) || (parsed < -32768L) || (parsed > 32767L))
    {
        return 0U;
    }

    *value = (int16_t)parsed;
    *cursor = end + 1;
    return 1U;
}

static uint8_t ParseNumberField(const char **cursor, int16_t *value, char delimiter, const char **reason)
{
    if (!ParseNumber(cursor, value, delimiter))
    {
        *reason = "BAD_NUMBER";
        return 0U;
    }
    return 1U;
}

static uint8_t IsValidMode(char mode)
{
    return (strchr("EOSBWHL", mode) != 0) ? 1U : 0U;
}

static uint8_t ParseStatusFrame(const char *line, LowerStatusFrame_t *frame, const char **reason)
{
    const char *cursor;
    int16_t power;

    if ((line == 0) || (frame == 0) || (reason == 0))
    {
        return 0U;
    }

    if (strncmp(line, "$T,", 3) != 0)
    {
        *reason = "BAD_HEADER";
        return 0U;
    }

    cursor = line + 3;
    if (!ParseNumberField(&cursor, &frame->currentTemp, ',', reason))
    {
        *reason = "BAD_TEMP";
        return 0U;
    }
    if (!ParseNumberField(&cursor, &frame->setTemp, ',', reason))
    {
        *reason = "BAD_SET";
        return 0U;
    }
    if (!ParseNumberField(&cursor, &power, ',', reason))
    {
        *reason = "BAD_POWER";
        return 0U;
    }

    if ((power < 0) || (power > 100))
    {
        *reason = "POWER_RANGE";
        return 0U;
    }
    if (!IsValidMode(cursor[0]))
    {
        *reason = "BAD_MODE";
        return 0U;
    }
    if (cursor[1] != '\0')
    {
        *reason = "TRAILING";
        return 0U;
    }

    frame->powerPercent = (uint8_t)power;
    frame->mode = cursor[0];
    return 1U;
}

static void ApplyStatusFrame(AppState_t *state, const LowerStatusFrame_t *frame, const char *line, uint32_t nowMs)
{
    state->ironTemp = frame->currentTemp;
    state->setTemp = frame->setTemp;
    state->heaterPower = frame->powerPercent;
    state->lowerMode = frame->mode;

    lastValidRxMs = nowMs;
    state->lastLowerRxMs = nowMs;
    state->lowerOnline = 1U;

    strncpy(state->lastDebug, line, sizeof(state->lastDebug) - 1U);
    state->lastDebug[sizeof(state->lastDebug) - 1U] = '\0';
    CopyDebugText(state->lowerLastError, sizeof(state->lowerLastError), "OK");
    CopyDebugText(state->lowerLastRaw, sizeof(state->lowerLastRaw), line);
}

void LowerLink_Init(void)
{
    lastValidRxMs = 0U;
    lastSeenRxBytes = 0U;
    lastSeenOverflow = 0U;
    LowerUart_Init();
}

void LowerLink_Task(AppState_t *state, uint32_t nowMs)
{
    char line[LOWER_UART_LINE_MAX];
    LowerStatusFrame_t frame;
    const char *reason = "UNKNOWN";
    uint32_t rxBytes = LowerUart_GetRxByteCount();
    uint32_t overflowCount = LowerUart_GetOverflowCount();
    uint8_t processed = 0U;

    if (rxBytes != lastSeenRxBytes)
    {
        lastSeenRxBytes = rxBytes;
        state->lowerRxBytes = rxBytes;
        state->lastLowerAnyRxMs = nowMs;

        if ((state->lowerRxLines == 0U) && (state->lowerValidFrames == 0U))
        {
            SetLowerRxActivityError(state, "RX_NO_LF");
        }
    }
    state->lowerOverflowCount = overflowCount;
    if (overflowCount != lastSeenOverflow)
    {
        lastSeenOverflow = overflowCount;
        SetLowerRxActivityError(state, "RX_OVERFLOW");
    }

    while ((processed < 8U) && LowerUart_GetLine(line, sizeof(line)))
    {
        processed++;
        state->lowerRxLines++;
        if (ParseStatusFrame(line, &frame, &reason))
        {
            state->lowerValidFrames++;
            ApplyStatusFrame(state, &frame, line, nowMs);
        }
        else
        {
            state->lowerInvalidFrames++;
            SetLowerError(state, reason, line);
        }
    }

    state->lowerOnline = ((lastValidRxMs != 0U) &&
                          ((nowMs - lastValidRxMs) <= LOWER_ONLINE_TIMEOUT_MS)) ? 1U : 0U;
}
