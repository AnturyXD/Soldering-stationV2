#include "stm32f10x.h"
#include "delay.h"
#include "timer.h"
#include "board_config.h"
#include "board_io.h"
#include "debug_uart.h"
#include "relay.h"
#include "presence.h"
#include "esp01s.h"
#include "lower_link.h"
#include "display_ui.h"
#include "st7789.h"
#include <string.h>

static AppState_t app;
static uint32_t lastStatusReportMs;
static uint32_t lastDebugReportMs;
static uint32_t lastRelayApplyMs;
#if APP_TEMP_DEMO_ENABLE
static uint32_t lastTempDemoMs;
static int16_t tempDemoValue = 180;
static int8_t tempDemoStep = 1;
#endif

static void App_SetLoad(LoadChannel_t channel, uint8_t on)
{
    if (channel >= LOAD_COUNT)
    {
        return;
    }

    app.load[channel] = on ? 1U : 0U;
}

/* 手动/自动模式的核心区别：
 * 手动模式由网页命令直接改三路负载；自动模式预留给人体传感器联动。
 * V0.3 传感器未接入，自动策略默认关闭，避免上电后误开灯和风扇。
 */
static void App_AutoControlTask(void)
{
    if ((app.mode != APP_MODE_AUTO) || !app.autoFeatureEnabled)
    {
        return;
    }

    if (app.presenceDetected)
    {
        app.load[LOAD_LIGHT] = 1U;
        app.load[LOAD_FAN] = 1U;
    }
    else
    {
        app.load[LOAD_LIGHT] = 0U;
        app.load[LOAD_FAN] = 0U;
    }
}

static void App_PresenceTask(uint32_t nowMs)
{
    Presence_Task(nowMs);
    app.presenceKnown = app.autoFeatureEnabled ? 1U : 0U;
    app.presenceDetected = app.autoFeatureEnabled ? Presence_IsDetected() : 0U;
}

static void App_ProcessCommand(const EspCommand_t *cmd)
{
    uint8_t ok = 1U;
    const char *reason = "NONE";

    if (cmd == 0)
    {
        return;
    }

    app.commandCount++;

    if (strcmp(cmd->target, "mode") == 0)
    {
        app.mode = (cmd->value != 0) ? APP_MODE_AUTO : APP_MODE_MANUAL;
    }
    else if (strcmp(cmd->target, "all") == 0)
    {
        if (cmd->value == 0)
        {
            App_SetLoad(LOAD_LIGHT, 0U);
            App_SetLoad(LOAD_FAN, 0U);
            App_SetLoad(LOAD_AUX, 0U);
        }
        else
        {
            ok = 0U;
            reason = "BAD_VALUE";
        }
    }
    else if ((app.mode == APP_MODE_AUTO) && !app.autoFeatureEnabled)
    {
        ok = 0U;
        reason = "AUTO_RESERVED";
    }
    else if (app.mode != APP_MODE_MANUAL)
    {
        ok = 0U;
        reason = "AUTO_MODE";
    }
    else if (strcmp(cmd->target, "light") == 0)
    {
        App_SetLoad(LOAD_LIGHT, (uint8_t)(cmd->value != 0));
    }
    else if (strcmp(cmd->target, "fan") == 0)
    {
        App_SetLoad(LOAD_FAN, (uint8_t)(cmd->value != 0));
    }
    else if (strcmp(cmd->target, "aux") == 0)
    {
        App_SetLoad(LOAD_AUX, (uint8_t)(cmd->value != 0));
    }
    else
    {
        ok = 0U;
        reason = "BAD_TARGET";
    }

    ESP01S_SendAck(cmd->seq, ok, reason);
}

static void App_CommandTask(void)
{
    EspCommand_t cmd;
    uint8_t count = 0U;

    while ((count < 4U) && ESP01S_PopCommand(&cmd))
    {
        count++;
        App_ProcessCommand(&cmd);
    }
}

static void App_RelayTask(uint32_t nowMs)
{
    if ((nowMs - lastRelayApplyMs) < APP_RELAY_APPLY_MS)
    {
        return;
    }
    lastRelayApplyMs = nowMs;
    Relay_ApplyAll(app.load);
}

static void App_TempDemoTask(uint32_t nowMs)
{
#if APP_TEMP_DEMO_ENABLE
    /* 没有真实下位机数据时，测试源要持续维持在线状态。
     * 否则 LowerLink_Task 会在每个循环把 lowerOnline 清零，导致大数字只闪现一帧。
     */
    if ((app.lastLowerRxMs != 0U) && ((nowMs - app.lastLowerRxMs) < LOWER_ONLINE_TIMEOUT_MS))
    {
        return;
    }

    app.lowerOnline = 1U;
    app.setTemp = 350;
    if (app.ironTemp < 0)
    {
        app.ironTemp = tempDemoValue;
    }

    if ((nowMs - lastTempDemoMs) < APP_TEMP_DEMO_MS)
    {
        return;
    }
    lastTempDemoMs = nowMs;

    /* 曲线测试源：没有真实下位机温度时，生成一个缓慢变化的读数用于观察屏幕曲线刷新。 */
    tempDemoValue = (int16_t)(tempDemoValue + (int16_t)tempDemoStep * 6);
    if (tempDemoValue >= 360)
    {
        tempDemoValue = 360;
        tempDemoStep = -1;
    }
    else if (tempDemoValue <= 160)
    {
        tempDemoValue = 160;
        tempDemoStep = 1;
    }

    app.ironTemp = tempDemoValue;
    strcpy(app.lastDebug, "demo temp curve");
#else
    (void)nowMs;
#endif
}

static void App_ReportTask(uint32_t nowMs)
{
    app.runTimeMs = nowMs;

    if ((nowMs - lastStatusReportMs) >= APP_STATUS_REPORT_MS)
    {
        lastStatusReportMs = nowMs;
        ESP01S_SendState(&app);
    }

    if ((nowMs - lastDebugReportMs) >= APP_DEBUG_REPORT_MS)
    {
        lastDebugReportMs = nowMs;
        DebugUart_Printf("[APP] t=%lu mode=%s esp=%u web=%u lower=%u lowerMode=%c power=%u presence=%u temp=%d set=%d load=%u,%u,%u ip=%s lowerRx=%lu lines=%lu valid=%lu bad=%lu ovf=%lu ore=%lu fe=%lu ne=%lu pe=%lu err=%s raw=%s\r\n",
                         (unsigned long)nowMs,
                         AppMode_ToText(app.mode),
                         (unsigned int)app.espOnline,
                         (unsigned int)app.webOnline,
                         (unsigned int)app.lowerOnline,
                         app.lowerMode,
                         (unsigned int)app.heaterPower,
                         (unsigned int)app.presenceDetected,
                         (int)app.ironTemp,
                         (int)app.setTemp,
                         (unsigned int)app.load[LOAD_LIGHT],
                         (unsigned int)app.load[LOAD_FAN],
                         (unsigned int)app.load[LOAD_AUX],
                         app.ip,
                         (unsigned long)app.lowerRxBytes,
                         (unsigned long)app.lowerRxLines,
                         (unsigned long)app.lowerValidFrames,
                         (unsigned long)app.lowerInvalidFrames,
                         (unsigned long)app.lowerOverflowCount,
                         (unsigned long)app.lowerOverrunCount,
                         (unsigned long)app.lowerFrameErrorCount,
                         (unsigned long)app.lowerNoiseErrorCount,
                         (unsigned long)app.lowerParityErrorCount,
                         app.lowerLastError,
                         app.lowerLastRaw);
    }
}

static void App_Init(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    AppState_Init(&app);
    app.autoFeatureEnabled = Presence_IsEnabled();

    Timer_Init1ms();
    DebugUart_Init();
    BoardIo_Init();
    Relay_Init();
    Presence_Init();
    LowerLink_Init();
    ESP01S_Init();

    Relay_AllOff();
    DisplayUi_Init(&app);

    DebugUart_Write("\r\nSmartSolder Extension V0.3 boot\r\n");
}

int main(void)
{
    uint32_t nowMs;

    SystemInit();
    delay_init(72);
    App_Init();

    while (1)
    {
        nowMs = Timer_GetMs();

        ST7789_Task();
        ESP01S_Task(&app, nowMs);
        LowerLink_Task(&app, nowMs);

        App_CommandTask();
        App_PresenceTask(nowMs);
        App_AutoControlTask();
        App_TempDemoTask(nowMs);
        App_RelayTask(nowMs);
        App_ReportTask(nowMs);

        DisplayUi_Task(&app, nowMs);
        BoardIo_Task(nowMs, app.espOnline);
    }
}
