#include "board_io.h"

static uint32_t lastBlinkMs;
static uint8_t heartbeatOn;

void BoardIo_Init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_6;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_13;
    GPIO_Init(GPIOC, &gpio);

    GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    GPIO_SetBits(GPIOA, GPIO_Pin_6);
    GPIO_SetBits(GPIOC, GPIO_Pin_13);
}

void BoardIo_Task(uint32_t nowMs, uint8_t espOnline)
{
    if ((nowMs - lastBlinkMs) >= 500UL)
    {
        lastBlinkMs = nowMs;
        heartbeatOn = heartbeatOn ? 0U : 1U;
        if (heartbeatOn)
        {
            GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        }
        else
        {
            GPIO_SetBits(GPIOC, GPIO_Pin_13);
        }
    }

    if (espOnline)
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_4);
    }
    else
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_4);
    }
}
