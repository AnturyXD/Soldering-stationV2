#include "display_ui.h"
#include "st7789.h"
#include "ui_assets.h"
#include <stdio.h>
#include <string.h>

#define UI_BG             0x0008
#define UI_BAR            0x0000
#define UI_PANEL          0x0861
#define UI_PANEL_DARK     0x0021
#define UI_BORDER         0x1945
#define UI_TEXT           0xFFFF
#define UI_TEXT_DIM       0x9CD3
#define UI_GRID           0x2947
#define UI_CYAN           0x05FF
#define UI_ORANGE         0xFD20
#define UI_GREEN          0x07E0
#define UI_OFF            0x6B4D
#define UI_DIGIT_SHADOW   0x3186

#define CHART_BOX_X       8U
#define CHART_BOX_Y       126U
#define CHART_BOX_W       224U
#define CHART_BOX_H       62U
#define CHART_X           22U
#define CHART_Y           133U
#define CHART_W           206U
#define CHART_H           38U
#define CHART_RIGHT       (CHART_X + CHART_W - 1U)
#define CHART_BOTTOM      (CHART_Y + CHART_H - 1U)
#define CHART_MIN_TEMP    150
#define CHART_MAX_TEMP    400

#define TEMP_HISTORY_SIZE 60U
#define BIG_DIGIT_W       26U
#define BIG_DIGIT_H       44U
#define BIG_DIGIT_T       6U
#define BIG_DIGIT_Y       42U

static AppState_t last;
static uint8_t initialized;
static int16_t tempHistory[TEMP_HISTORY_SIZE];
static uint8_t tempCount;

static uint8_t bigDigitCache[4] = { 0xFFU, 0xFFU, 0xFFU, 0xFFU };
static uint8_t bigDotCache = 0xFFU;
static uint16_t bigDigitColorCache = 0x0000U;

static char timeCache[6];
static char infoCache[24];
static char ipCache[16];
static uint8_t uartCache = 0xFFU;
static uint8_t uartTextCache = 0xFFU;
static uint8_t workTextCache = 0xFFU;
static uint8_t infoModeCache = 0xFFU;
static uint8_t relayCache[LOAD_COUNT] = { 0xFFU, 0xFFU, 0xFFU };
static uint32_t lastTopSecond = 0xFFFFFFFFUL;
static uint8_t pageCache = 0xFFU;

static const uint8_t digitSegMap[10] =
{
    0x3FU, 0x06U, 0x5BU, 0x4FU, 0x66U,
    0x6DU, 0x7DU, 0x07U, 0x7FU, 0x6FU
};

static uint16_t BigDigitX(uint8_t idx)
{
    static const uint16_t x[4] = { 18U, 52U, 86U, 132U };

    return (idx < 4U) ? x[idx] : 18U;
}

static void DrawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    ST7789_DrawLine(x, y, (uint16_t)(x + w - 1U), y, color);
    ST7789_DrawLine(x, (uint16_t)(y + h - 1U), (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U), color);
    ST7789_DrawLine(x, y, x, (uint16_t)(y + h - 1U), color);
    ST7789_DrawLine((uint16_t)(x + w - 1U), y, (uint16_t)(x + w - 1U), (uint16_t)(y + h - 1U), color);
}

static void DrawThickLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    ST7789_DrawLine(x0, y0, x1, y1, color);
    if ((y0 > 0U) && (y1 > 0U))
    {
        ST7789_DrawLine(x0, (uint16_t)(y0 - 1U), x1, (uint16_t)(y1 - 1U), color);
    }
    if ((y0 < 239U) && (y1 < 239U))
    {
        ST7789_DrawLine(x0, (uint16_t)(y0 + 1U), x1, (uint16_t)(y1 + 1U), color);
    }
}

static void DrawCachedString(uint16_t x,
                             uint16_t y,
                             const char *text,
                             char *cache,
                             uint8_t chars,
                             uint16_t color,
                             uint16_t bg,
                             uint8_t scale)
{
    uint8_t i;
    char ch;

    for (i = 0U; i < chars; i++)
    {
        ch = ((text != 0) && (text[i] != '\0')) ? text[i] : ' ';
        if (cache[i] != ch)
        {
            cache[i] = ch;
            ST7789_DrawChar((uint16_t)(x + i * 6U * scale), y, ch, color, bg, scale);
        }
    }
}

static void ResetDrawCaches(void)
{
    memset(&last, 0xFF, sizeof(last));
    memset(timeCache, 0xFF, sizeof(timeCache));
    memset(infoCache, 0xFF, sizeof(infoCache));
    memset(bigDigitCache, 0xFF, sizeof(bigDigitCache));
    memset(relayCache, 0xFF, sizeof(relayCache));
    ipCache[0] = '\0';
    bigDotCache = 0xFFU;
    bigDigitColorCache = 0x0000U;
    uartCache = 0xFFU;
    uartTextCache = 0xFFU;
    workTextCache = 0xFFU;
    infoModeCache = 0xFFU;
    tempCount = 0U;
    lastTopSecond = 0xFFFFFFFFUL;
}

static void BigSegRect(uint16_t x, uint16_t y, uint8_t seg, uint16_t color)
{
    const uint16_t w = BIG_DIGIT_W;
    const uint16_t h = BIG_DIGIT_H;
    const uint16_t t = BIG_DIGIT_T;

    switch (seg)
    {
    case 0U:
        ST7789_FillRect((uint16_t)(x + t - 1U), y, (uint16_t)(w - 2U * t + 2U), t, color);
        break;
    case 1U:
        ST7789_FillRect((uint16_t)(x + w - t), (uint16_t)(y + t - 1U), t, (uint16_t)(h / 2U - t + 2U), color);
        break;
    case 2U:
        ST7789_FillRect((uint16_t)(x + w - t), (uint16_t)(y + h / 2U - 1U), t, (uint16_t)(h / 2U - t + 2U), color);
        break;
    case 3U:
        ST7789_FillRect((uint16_t)(x + t - 1U), (uint16_t)(y + h - t), (uint16_t)(w - 2U * t + 2U), t, color);
        break;
    case 4U:
        ST7789_FillRect(x, (uint16_t)(y + h / 2U - 1U), t, (uint16_t)(h / 2U - t + 2U), color);
        break;
    case 5U:
        ST7789_FillRect(x, (uint16_t)(y + t - 1U), t, (uint16_t)(h / 2U - t + 2U), color);
        break;
    case 6U:
        ST7789_FillRect((uint16_t)(x + t - 1U), (uint16_t)(y + h / 2U - t / 2U), (uint16_t)(w - 2U * t + 2U), t, color);
        break;
    default:
        break;
    }
}

static void DrawBigDigit(uint8_t idx, uint8_t mask, uint16_t color)
{
    uint8_t seg;
    uint16_t x = BigDigitX(idx);

    if (idx >= 4U)
    {
        return;
    }

    if (bigDigitColorCache != color)
    {
        memset(bigDigitCache, 0xFF, sizeof(bigDigitCache));
        bigDotCache = 0xFFU;
        bigDigitColorCache = color;
    }

    if (bigDigitCache[idx] == mask)
    {
        return;
    }
    bigDigitCache[idx] = mask;

    /* 单位数字变化时完整重绘该位，避免段缓存造成笔画残缺。 */
    ST7789_FillRect(x, BIG_DIGIT_Y, BIG_DIGIT_W, BIG_DIGIT_H, UI_PANEL);
    for (seg = 0U; seg < 7U; seg++)
    {
        if (mask & (uint8_t)(1U << seg))
        {
            BigSegRect(x, BIG_DIGIT_Y, seg, color);
        }
    }
}

static void DrawBigDot(uint8_t on, uint16_t color)
{
    if (bigDotCache == on)
    {
        return;
    }
    bigDotCache = on;
    ST7789_FillRect(120, 78, 7, 7, on ? color : UI_PANEL);
}

static void DrawDegreeC(uint16_t color)
{
    static uint16_t lastColor = 0x0000U;

    if (lastColor == color)
    {
        return;
    }
    lastColor = color;

    ST7789_FillRect(162, 58, 38, 26, UI_PANEL);
    ST7789_FillRect(162, 58, 5, 1, color);
    ST7789_FillRect(162, 62, 5, 1, color);
    ST7789_FillRect(162, 58, 1, 5, color);
    ST7789_FillRect(166, 58, 1, 5, color);
    ST7789_DrawChar(172, 57, 'C', color, UI_PANEL, 3);
}

static void DrawStatusBar(const AppState_t *state, uint32_t nowMs)
{
    char timeText[6];
    UiTextZh_t workText;
    uint8_t workKey;
    uint32_t totalSecond = nowMs / 1000UL;
    uint32_t minute = (totalSecond / 60UL) % 100UL;
    uint32_t second = totalSecond % 60UL;
    uint8_t uartOn = state->lowerOnline ? 1U : 0U;

    if (uartCache != uartOn)
    {
        uartCache = uartOn;
        ST7789_FillRect(42, 8, 8, 8, uartOn ? UI_GREEN : UI_OFF);
    }
    if (uartTextCache != uartOn)
    {
        uartTextCache = uartOn;
        ST7789_FillRect(8, 4, 34, 16, UI_BAR);
        UiAssets_DrawText16(8, 4, uartOn ? UI_TEXT_ZH_ONLINE : UI_TEXT_ZH_OFFLINE,
                            uartOn ? UI_GREEN : UI_OFF,
                            UI_BAR);
    }

    if (!state->lowerOnline)
    {
        workText = UI_TEXT_ZH_OFFLINE;
        workKey = 0U;
    }
    else if (state->mode == APP_MODE_AUTO)
    {
        workText = UI_TEXT_ZH_AUTO;
        workKey = 1U;
    }
    else
    {
        workText = UI_TEXT_ZH_WORKING;
        workKey = 2U;
    }
    if (workTextCache != workKey)
    {
        workTextCache = workKey;
        ST7789_FillRect(90, 4, 60, 16, UI_BAR);
        UiAssets_DrawText16(96, 4, workText, UI_TEXT, UI_BAR);
    }

    snprintf(timeText, sizeof(timeText), "%02lu:%02lu", (unsigned long)minute, (unsigned long)second);
    DrawCachedString(176, 5, timeText, timeCache, 5U, UI_TEXT, UI_BAR, 2);
}

static int16_t GetTargetTemp(const AppState_t *state)
{
    return (state->setTemp > 0) ? state->setTemp : 350;
}

static uint8_t CalcPowerPercent(const AppState_t *state)
{
    int16_t target = GetTargetTemp(state);
    int16_t diff;
    int16_t power;

    if (!state->lowerOnline || (state->ironTemp < 0))
    {
        return 0U;
    }

    diff = (int16_t)(target - state->ironTemp);
    if (diff <= 0)
    {
        return 0U;
    }

    power = (int16_t)(diff * 3);
    if (power < 18)
    {
        power = 18;
    }
    if (power > 99)
    {
        power = 99;
    }
    return (uint8_t)power;
}

static void DrawMainTemperature(const AppState_t *state)
{
    int16_t temp = state->ironTemp;
    uint8_t hundreds;
    uint8_t tens;
    uint8_t ones;
    uint16_t color = state->lowerOnline ? UI_TEXT : UI_TEXT_DIM;

    if (!state->lowerOnline || (temp < 0))
    {
        DrawBigDigit(0U, 0x40U, color);
        DrawBigDigit(1U, 0x40U, color);
        DrawBigDigit(2U, 0x00U, color);
        DrawBigDot(1U, color);
        DrawBigDigit(3U, 0x40U, color);
        DrawDegreeC(color);
        return;
    }

    if (temp > 999)
    {
        temp = 999;
    }

    hundreds = (uint8_t)((temp / 100) % 10);
    tens = (uint8_t)((temp / 10) % 10);
    ones = (uint8_t)(temp % 10);

    DrawBigDigit(0U, (temp >= 100) ? digitSegMap[hundreds] : 0x00U, color);
    DrawBigDigit(1U, (temp >= 10) ? digitSegMap[tens] : 0x00U, color);
    DrawBigDigit(2U, digitSegMap[ones], color);
    DrawBigDot(1U, color);
    DrawBigDigit(3U, digitSegMap[0], color);
    DrawDegreeC(color);
}

static void DrawInfoRow(const AppState_t *state)
{
    char line[24];
    int16_t target = GetTargetTemp(state);
    uint8_t power = CalcPowerPercent(state);
    const char *status;

    if (!state->lowerOnline)
    {
        status = "OFFLINE";
    }
    else if ((state->ironTemp >= 0) && (state->ironTemp < target - 3))
    {
        status = "HEATING";
    }
    else
    {
        status = "READY";
    }

    if (infoModeCache != (uint8_t)state->mode)
    {
        infoModeCache = (uint8_t)state->mode;
        ST7789_FillRect(14, 98, 34, 16, UI_PANEL);
        UiAssets_DrawText16(14, 98,
                            (state->mode == APP_MODE_AUTO) ? UI_TEXT_ZH_AUTO : UI_TEXT_ZH_MANUAL,
                            UI_ORANGE,
                            UI_PANEL);
    }

    snprintf(line, sizeof(line), "T%dC %s %u%%", (int)target, status, (unsigned int)power);
    DrawCachedString(54, 102, line, infoCache, 18U, UI_ORANGE, UI_PANEL, 1);
}

static void PushTemp(int16_t temp)
{
    uint8_t i;

    if (temp < 0)
    {
        return;
    }

    if (tempCount < TEMP_HISTORY_SIZE)
    {
        tempHistory[tempCount++] = temp;
    }
    else
    {
        for (i = 1U; i < TEMP_HISTORY_SIZE; i++)
        {
            tempHistory[i - 1U] = tempHistory[i];
        }
        tempHistory[TEMP_HISTORY_SIZE - 1U] = temp;
    }
}

static uint16_t TempToY(int16_t temp)
{
    int32_t clamped = temp;

    if (clamped < CHART_MIN_TEMP)
    {
        clamped = CHART_MIN_TEMP;
    }
    if (clamped > CHART_MAX_TEMP)
    {
        clamped = CHART_MAX_TEMP;
    }

    return (uint16_t)((int32_t)CHART_BOTTOM -
                      ((clamped - CHART_MIN_TEMP) * (int32_t)(CHART_H - 4U)) /
                      (CHART_MAX_TEMP - CHART_MIN_TEMP));
}

static void DrawChartGrid(void)
{
    uint8_t i;
    uint16_t y;

    ST7789_FillRect(CHART_BOX_X, CHART_BOX_Y, CHART_BOX_W, CHART_BOX_H, UI_PANEL_DARK);
    DrawRect(CHART_BOX_X, CHART_BOX_Y, CHART_BOX_W, CHART_BOX_H, UI_BORDER);

    ST7789_DrawString(12, 128, "C", UI_TEXT_DIM, UI_PANEL_DARK, 1);
    ST7789_DrawString(34, 176, "-60S", UI_TEXT_DIM, UI_PANEL_DARK, 1);
    ST7789_DrawString(199, 176, "0S", UI_CYAN, UI_PANEL_DARK, 1);

    for (i = 0U; i < 3U; i++)
    {
        y = (uint16_t)(CHART_Y + 8U + i * 12U);
        ST7789_DrawLine(CHART_X, y, CHART_RIGHT, y, UI_GRID);
    }
    ST7789_DrawLine(CHART_X, CHART_BOTTOM, CHART_RIGHT, CHART_BOTTOM, UI_TEXT_DIM);
    ST7789_DrawLine(CHART_X, CHART_Y, CHART_X, CHART_BOTTOM, UI_TEXT_DIM);
}

static void DrawDashedTarget(uint16_t y)
{
    uint16_t x;

    for (x = CHART_X; x < CHART_RIGHT; x = (uint16_t)(x + 10U))
    {
        ST7789_DrawLine(x, y, (uint16_t)(x + 5U), y, UI_ORANGE);
    }
}

static void DrawCurve(const AppState_t *state)
{
    uint8_t i;
    uint16_t x0;
    uint16_t y0;
    uint16_t x1;
    uint16_t y1;
    uint16_t targetY;

    PushTemp(state->ironTemp);

    /* 曲线区只刷新绘图内部，不再重画坐标文字和外框。 */
    ST7789_FillRect(CHART_X, CHART_Y, CHART_W, CHART_H, UI_PANEL_DARK);
    for (i = 0U; i < 3U; i++)
    {
        y0 = (uint16_t)(CHART_Y + 8U + i * 12U);
        ST7789_DrawLine(CHART_X, y0, CHART_RIGHT, y0, UI_GRID);
    }
    ST7789_DrawLine(CHART_X, CHART_BOTTOM, CHART_RIGHT, CHART_BOTTOM, UI_TEXT_DIM);
    ST7789_DrawLine(CHART_X, CHART_Y, CHART_X, CHART_BOTTOM, UI_TEXT_DIM);

    targetY = TempToY(GetTargetTemp(state));
    DrawDashedTarget(targetY);

    if (tempCount < 2U)
    {
        return;
    }

    for (i = 1U; i < tempCount; i++)
    {
        x0 = (uint16_t)(CHART_X + (uint16_t)(i - 1U) * (CHART_W - 4U) / (TEMP_HISTORY_SIZE - 1U));
        x1 = (uint16_t)(CHART_X + (uint16_t)i * (CHART_W - 4U) / (TEMP_HISTORY_SIZE - 1U));
        y0 = TempToY(tempHistory[i - 1U]);
        y1 = TempToY(tempHistory[i]);
        DrawThickLine(x0, y0, x1, y1, UI_CYAN);
    }
}

static void DrawRelayCard(uint8_t idx, const char *name, uint8_t on)
{
    uint16_t x = (idx == 0U) ? 8U : ((idx == 1U) ? 84U : 160U);
    uint16_t border = on ? UI_GREEN : UI_OFF;
    uint16_t fill = on ? 0x0400 : UI_PANEL_DARK;

    if ((idx < LOAD_COUNT) && (relayCache[idx] == on))
    {
        return;
    }
    if (idx < LOAD_COUNT)
    {
        relayCache[idx] = on;
    }

    ST7789_FillRect(x, 196, 72, 34, fill);
    DrawRect(x, 196, 72, 34, border);
    ST7789_DrawString((uint16_t)(x + 7U), 201, name, UI_TEXT, fill, 1);
    ST7789_DrawString((uint16_t)(x + 42U), 201, on ? "ON" : "OFF", on ? UI_GREEN : UI_OFF, fill, 1);
    if (idx == 0U)
    {
        UiAssets_DrawText16((uint16_t)(x + 20U), 211, UI_TEXT_ZH_LIGHT, on ? UI_GREEN : UI_OFF, fill);
    }
    else if (idx == 1U)
    {
        UiAssets_DrawText16((uint16_t)(x + 20U), 211, UI_TEXT_ZH_FAN, on ? UI_GREEN : UI_OFF, fill);
    }
}

static void DrawStaticFrame(void)
{
    ST7789_FillScreen(UI_BG);
    ST7789_FillRect(0, 0, 240, 24, UI_BAR);

    ST7789_FillRect(4, 28, 232, 208, UI_PANEL);
    DrawRect(4, 28, 232, 208, UI_BORDER);
    ST7789_DrawString(12, 32, "CH1", UI_CYAN, UI_PANEL, 1);
    ST7789_DrawLine(10, 92, 230, 92, UI_BORDER);
    ST7789_DrawLine(10, 124, 230, 124, UI_BORDER);
    ST7789_DrawLine(10, 190, 230, 190, UI_BORDER);
    DrawChartGrid();
}

static void DrawWaitingFrame(void)
{
    ST7789_FillScreen(UI_BG);
    ST7789_FillRect(0, 0, 240, 24, UI_BAR);

    ST7789_FillRect(8, 42, 224, 112, UI_PANEL);
    DrawRect(8, 42, 224, 112, UI_BORDER);
    ST7789_DrawString(30, 54, "WEB OFFLINE", UI_ORANGE, UI_PANEL, 2);
    ST7789_DrawString(28, 88, "IP", UI_TEXT_DIM, UI_PANEL, 2);
    ST7789_DrawString(28, 128, "WAIT UPPER LINK", UI_TEXT_DIM, UI_PANEL, 1);

    ST7789_DrawLine(10, 190, 230, 190, UI_BORDER);
}

static void DrawWaitingInfo(const AppState_t *state, uint32_t nowMs)
{
    char timeText[6];
    const char *ipText = ((state->ip[0] != '\0') ? state->ip : "0.0.0.0");
    uint32_t totalSecond = nowMs / 1000UL;
    uint32_t minute = (totalSecond / 60UL) % 100UL;
    uint32_t second = totalSecond % 60UL;

    snprintf(timeText, sizeof(timeText), "%02lu:%02lu", (unsigned long)minute, (unsigned long)second);
    DrawCachedString(176, 5, timeText, timeCache, 5U, UI_TEXT, UI_BAR, 2);

    if (strncmp(ipCache, ipText, sizeof(ipCache)) != 0)
    {
        strncpy(ipCache, ipText, sizeof(ipCache) - 1U);
        ipCache[sizeof(ipCache) - 1U] = '\0';
        ST7789_FillRect(58, 88, 164, 18, UI_PANEL);
        ST7789_DrawString(58, 88, ipCache, UI_CYAN, UI_PANEL, 2);
    }

    if ((state->espOnline != last.espOnline) || (state->webOnline != last.webOnline))
    {
        ST7789_FillRect(28, 112, 184, 10, UI_PANEL);
        ST7789_DrawString(28, 112, state->espOnline ? "ESP OK" : "ESP OFF",
                          state->espOnline ? UI_CYAN : UI_OFF,
                          UI_PANEL,
                          1);
        ST7789_DrawString(116, 112, state->webOnline ? "WEB OK" : "WEB WAIT",
                          state->webOnline ? UI_GREEN : UI_OFF,
                          UI_PANEL,
                          1);
    }
}

void DisplayUi_Init(const AppState_t *state)
{
    (void)state;

    ST7789_Init();
    ResetDrawCaches();
    pageCache = 0xFFU;
    initialized = 1U;
}

void DisplayUi_Task(const AppState_t *state, uint32_t nowMs)
{
    uint32_t topSecond = nowMs / 1000UL;
    uint8_t dashboardPage = (state->espOnline && state->webOnline) ? 1U : 0U;

    if (!initialized)
    {
        return;
    }

    ST7789_Task();

    if (pageCache != dashboardPage)
    {
        pageCache = dashboardPage;
        ResetDrawCaches();
        if (dashboardPage)
        {
            DrawStaticFrame();
        }
        else
        {
            DrawWaitingFrame();
        }
    }

    if (!dashboardPage)
    {
        if ((topSecond != lastTopSecond) ||
            (state->espOnline != last.espOnline) ||
            (state->webOnline != last.webOnline))
        {
            lastTopSecond = topSecond;
            DrawWaitingInfo(state, nowMs);
        }

        DrawRelayCard(0U, "R1", state->load[LOAD_LIGHT]);
        DrawRelayCard(1U, "R2", state->load[LOAD_FAN]);
        DrawRelayCard(2U, "R3", state->load[LOAD_AUX]);

        last = *state;
        return;
    }

    if ((topSecond != lastTopSecond) ||
        (state->espOnline != last.espOnline) ||
        (state->lowerOnline != last.lowerOnline) ||
        (state->webOnline != last.webOnline) ||
        (state->mode != last.mode))
    {
        lastTopSecond = topSecond;
        DrawStatusBar(state, nowMs);
    }

    if ((state->ironTemp != last.ironTemp) ||
        (state->lowerOnline != last.lowerOnline) ||
        (state->setTemp != last.setTemp))
    {
        DrawMainTemperature(state);
        DrawInfoRow(state);
        DrawCurve(state);
    }

    DrawRelayCard(0U, "R1", state->load[LOAD_LIGHT]);
    DrawRelayCard(1U, "R2", state->load[LOAD_FAN]);
    DrawRelayCard(2U, "R3", state->load[LOAD_AUX]);

    last = *state;
}
