#include "presence.h"
#include "board_config.h"

static uint8_t stableDetected;
static uint8_t highActive;
static uint8_t lowActive;
static uint32_t highSinceMs;
static uint32_t lowSinceMs;
static uint32_t lastPresenceMs;
static uint32_t lastSampleMs;

static uint8_t Presence_ConvertRaw(uint8_t raw)
{
    return PRESENCE_ACTIVE_HIGH ? raw : (uint8_t)!raw;
}

void Presence_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /* PA1 使用内部下拉，传感器未连接时保持“无人”，避免浮空误触发继电器。 */
    GPIO_ResetBits(GPIOA, GPIO_Pin_1);
    gpio.GPIO_Pin = GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_IPD;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    stableDetected = 0U;
    highActive = 0U;
    lowActive = 0U;
    highSinceMs = 0U;
    lowSinceMs = 0U;
    lastPresenceMs = 0U;
    lastSampleMs = 0U;
}

void Presence_Task(uint32_t nowMs)
{
#if PRESENCE_FEATURE_ENABLE
    uint8_t rawDetected;

    if ((nowMs - lastSampleMs) < PRESENCE_SAMPLE_MS)
    {
        return;
    }
    lastSampleMs = nowMs;
    rawDetected = Presence_ConvertRaw(Presence_ReadRaw());

    if (rawDetected)
    {
        if (!highActive)
        {
            highActive = 1U;
            highSinceMs = nowMs;
        }
        lowActive = 0U;

        if ((nowMs - highSinceMs) >= PRESENCE_ON_DEBOUNCE_MS)
        {
            stableDetected = 1U;
            lastPresenceMs = nowMs;
        }
        return;
    }

    highActive = 0U;
    if (!lowActive)
    {
        lowActive = 1U;
        lowSinceMs = nowMs;
    }

    if (stableDetected &&
        ((nowMs - lastPresenceMs) >= PRESENCE_HOLD_MS) &&
        ((nowMs - lowSinceMs) >= PRESENCE_OFF_DELAY_MS))
    {
        stableDetected = 0U;
    }
#else
    (void)nowMs;
#endif
}

uint8_t Presence_ReadRaw(void)
{
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) ? 1U : 0U;
}

uint8_t Presence_IsEnabled(void)
{
    return PRESENCE_FEATURE_ENABLE ? 1U : 0U;
}

uint8_t Presence_IsDetected(void)
{
#if PRESENCE_FEATURE_ENABLE
    return stableDetected;
#else
    return 0U;
#endif
}
