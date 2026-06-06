#include "presence.h"
#include "board_config.h"

void Presence_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
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
    uint8_t raw = Presence_ReadRaw();
    return PRESENCE_ACTIVE_HIGH ? raw : (uint8_t)!raw;
#else
    return 0U;
#endif
}
