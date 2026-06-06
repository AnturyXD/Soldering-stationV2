#include "relay.h"
#include "board_config.h"

static uint8_t relayState[LOAD_COUNT];

static uint16_t Relay_Pin(LoadChannel_t channel)
{
    /* 继电器顺序按V0.3硬件规范固定：
     * 第一路PB13=照明灯，第二路PB12=排风扇，第三路PB14=预留。
     */
    switch (channel)
    {
    case LOAD_LIGHT:
        return GPIO_Pin_13; /* relay 1 */
    case LOAD_FAN:
        return GPIO_Pin_12; /* relay 2 */
    case LOAD_AUX:
        return GPIO_Pin_14; /* relay 3 */
    default:
        return 0;
    }
}

void Relay_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    Relay_AllOff();
}

void Relay_Set(LoadChannel_t channel, uint8_t on)
{
    uint16_t pin = Relay_Pin(channel);

    if ((pin == 0U) || (channel >= LOAD_COUNT))
    {
        return;
    }

    relayState[channel] = on ? 1U : 0U;

#if RELAY_ACTIVE_HIGH
    /* 当前继电器驱动按高电平触发设计，若硬件改版只需要改board_config.h。 */
    if (on)
    {
        GPIO_SetBits(GPIOB, pin);
    }
    else
    {
        GPIO_ResetBits(GPIOB, pin);
    }
#else
    if (on)
    {
        GPIO_ResetBits(GPIOB, pin);
    }
    else
    {
        GPIO_SetBits(GPIOB, pin);
    }
#endif
}

uint8_t Relay_Get(LoadChannel_t channel)
{
    return (channel < LOAD_COUNT) ? relayState[channel] : 0U;
}

void Relay_ApplyAll(const uint8_t load[LOAD_COUNT])
{
    Relay_Set(LOAD_LIGHT, load[LOAD_LIGHT]);
    Relay_Set(LOAD_FAN, load[LOAD_FAN]);
    Relay_Set(LOAD_AUX, load[LOAD_AUX]);
}

void Relay_AllOff(void)
{
    Relay_Set(LOAD_LIGHT, 0U);
    Relay_Set(LOAD_FAN, 0U);
    Relay_Set(LOAD_AUX, 0U);
}
