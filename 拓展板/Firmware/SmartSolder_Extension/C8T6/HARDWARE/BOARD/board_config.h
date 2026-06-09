#ifndef __BOARD_CONFIG_H
#define __BOARD_CONFIG_H

#include "stm32f10x.h"

#define FW_VERSION_TEXT               "V0.3"

#define APP_STATUS_REPORT_MS          500UL
#define APP_DEBUG_REPORT_MS           1000UL
#define APP_DISPLAY_TICK_MS           40UL
#define APP_RELAY_APPLY_MS            20UL
#define APP_TEMP_DEMO_ENABLE          0
#define APP_TEMP_DEMO_MS              1000UL

#define DEBUG_UART_BAUD               115200UL
#define ESP_UART_BAUD                 115200UL
#define LOWER_UART_BAUD               9600UL

#define LOWER_ONLINE_TIMEOUT_MS       3000UL
#define ESP_ONLINE_TIMEOUT_MS         10000UL

#define PRESENCE_FEATURE_ENABLE       1
#define PRESENCE_ACTIVE_HIGH          1
#define PRESENCE_SAMPLE_MS            20UL
#define PRESENCE_ON_DEBOUNCE_MS       200UL
#define PRESENCE_OFF_DELAY_MS         15000UL
#define PRESENCE_HOLD_MS              30000UL

#define RELAY_ACTIVE_HIGH             1

#define TFT_WIDTH                     240U
#define TFT_HEIGHT                    240U

#endif
