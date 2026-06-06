#include "esp01s.h"
#include "board_config.h"
#include "debug_uart.h"
#include "esp_uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static EspCommand_t pendingCmd;
static uint8_t pendingValid;
static uint32_t lastRxMs;
static char lastIp[16] = "0.0.0.0";
static uint8_t wsOnline;

static void StoreIp(AppState_t *state, const char *ip)
{
    if ((state == 0) || (ip == 0) || (ip[0] == '\0'))
    {
        return;
    }

    strncpy(lastIp, ip, sizeof(lastIp) - 1U);
    lastIp[sizeof(lastIp) - 1U] = '\0';
    strncpy(state->ip, lastIp, sizeof(state->ip) - 1U);
    state->ip[sizeof(state->ip) - 1U] = '\0';
}

static uint8_t ParseField(const char *line, const char *key, char *out, uint8_t outLen)
{
    const char *p;
    uint8_t i = 0U;

    if ((line == 0) || (key == 0) || (out == 0) || (outLen == 0U))
    {
        return 0U;
    }

    p = strstr(line, key);
    if (p == 0)
    {
        return 0U;
    }

    p += strlen(key);
    while ((*p != '\0') && (*p != ' ') && (i < (outLen - 1U)))
    {
        out[i++] = *p++;
    }
    out[i] = '\0';

    return (i > 0U) ? 1U : 0U;
}

static void HandleLine(AppState_t *state, const char *line, uint32_t nowMs)
{
    char valueBuf[12];
    char ipBuf[16];

    lastRxMs = nowMs;

    /* ESP桥接固件只负责网络状态和Web命令，业务决策全部留在STM32。 */
    if (strncmp(line, "IP=", 3) == 0)
    {
        StoreIp(state, line + 3);
        return;
    }

    if (ParseField(line, "ip=", ipBuf, sizeof(ipBuf)))
    {
        StoreIp(state, ipBuf);
    }

    if (strncmp(line, "WS=", 3) == 0)
    {
        wsOnline = (line[3] == '1') ? 1U : 0U;
        state->webOnline = wsOnline;
        return;
    }

    if (strncmp(line, "CMD ", 4) == 0)
    {
        memset(&pendingCmd, 0, sizeof(pendingCmd));
        if (!ParseField(line, "seq=", pendingCmd.seq, sizeof(pendingCmd.seq)))
        {
            strcpy(pendingCmd.seq, "0");
        }
        if (!ParseField(line, "target=", pendingCmd.target, sizeof(pendingCmd.target)))
        {
            return;
        }
        if (!ParseField(line, "value=", valueBuf, sizeof(valueBuf)))
        {
            return;
        }
        pendingCmd.value = atoi(valueBuf);
        pendingValid = 1U;
    }
}

void ESP01S_Init(void)
{
    EspUart_Init();
}

void ESP01S_Task(AppState_t *state, uint32_t nowMs)
{
    char line[ESP_UART_LINE_MAX];
    uint8_t processed = 0U;

    while ((processed < 8U) && EspUart_GetLine(line, sizeof(line)))
    {
        processed++;
        HandleLine(state, line, nowMs);
        if (strncmp(line, "DIAG ", 5) == 0)
        {
            strncpy(state->lastDebug, line + 5, sizeof(state->lastDebug) - 1U);
            state->lastDebug[sizeof(state->lastDebug) - 1U] = '\0';
        }
    }

    state->espOnline = ((lastRxMs != 0U) && ((nowMs - lastRxMs) <= ESP_ONLINE_TIMEOUT_MS)) ? 1U : 0U;
    if (!state->espOnline)
    {
        wsOnline = 0U;
    }
    state->webOnline = state->espOnline ? wsOnline : 0U;
}

void ESP01S_SendState(const AppState_t *state)
{
    char buf[420];

    /* 状态行保持单行JSON，ESP收到后原样推送到离线HTML页面。 */
    snprintf(buf,
             sizeof(buf),
             "STAT {\"fw\":\"%s\",\"mode\":\"%s\",\"espOnline\":%u,\"webOnline\":%u,"
             "\"lowerOnline\":%u,\"temp\":%d,\"setTemp\":%d,\"vin\":%u,"
             "\"presence\":{\"enabled\":%u,\"detected\":%u},"
             "\"relay\":{\"light\":%u,\"fan\":%u,\"aux\":%u},"
             "\"runTimeMs\":%lu,\"ip\":\"%s\",\"debug\":\"%s\"}\r\n",
             FW_VERSION_TEXT,
             AppMode_ToText(state->mode),
             (unsigned int)state->espOnline,
             (unsigned int)state->webOnline,
             (unsigned int)state->lowerOnline,
             (int)state->ironTemp,
             (int)state->setTemp,
             (unsigned int)state->inputMv,
             (unsigned int)state->autoFeatureEnabled,
             (unsigned int)state->presenceDetected,
             (unsigned int)state->load[LOAD_LIGHT],
             (unsigned int)state->load[LOAD_FAN],
             (unsigned int)state->load[LOAD_AUX],
             (unsigned long)state->runTimeMs,
             state->ip,
             state->lastDebug);

    EspUart_SendString(buf);
}

void ESP01S_SendAck(const char *seq, uint8_t ok, const char *reason)
{
    char buf[96];

    snprintf(buf,
             sizeof(buf),
             "ACK seq=%s ok=%u reason=%s\r\n",
             (seq != 0) ? seq : "0",
             (unsigned int)(ok ? 1U : 0U),
             (reason != 0) ? reason : "NONE");
    EspUart_SendString(buf);
}

uint8_t ESP01S_PopCommand(EspCommand_t *cmd)
{
    if ((cmd == 0) || !pendingValid)
    {
        return 0U;
    }

    *cmd = pendingCmd;
    pendingValid = 0U;
    return 1U;
}
