#ifndef __UI_ASSETS_H
#define __UI_ASSETS_H

#include "stm32f10x.h"

#define UI_ZH_FONT_W 16U
#define UI_ZH_FONT_H 16U

typedef enum
{
    UI_TEXT_ZH_WORKING = 0,
    UI_TEXT_ZH_ONLINE,
    UI_TEXT_ZH_OFFLINE,
    UI_TEXT_ZH_MANUAL,
    UI_TEXT_ZH_AUTO,
    UI_TEXT_ZH_LIGHT,
    UI_TEXT_ZH_FAN
} UiTextZh_t;

uint16_t UiAssets_TextWidth16(UiTextZh_t text);
void UiAssets_DrawText16(uint16_t x, uint16_t y, UiTextZh_t text, uint16_t fg, uint16_t bg);

#endif
