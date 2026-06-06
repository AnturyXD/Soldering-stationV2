#include "debug_uart.h"
#include "board_config.h"
#include <stdarg.h>
#include <stdio.h>

#define DBG_TX_BUF_SIZE 1024U

static volatile uint16_t txHead;
static volatile uint16_t txTail;
static volatile uint16_t dmaLen;
static volatile uint8_t dmaActive;
static uint8_t txBuf[DBG_TX_BUF_SIZE];

static void DebugUart_KickLocked(void)
{
    uint16_t len;

    if (dmaActive || (txHead == txTail))
    {
        return;
    }

    len = (txHead > txTail) ? (uint16_t)(txHead - txTail) : (uint16_t)(DBG_TX_BUF_SIZE - txTail);
    if (len == 0U)
    {
        return;
    }

    dmaLen = len;
    dmaActive = 1U;
    DMA_Cmd(DMA1_Channel4, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel4, len);
    DMA1_Channel4->CMAR = (uint32_t)&txBuf[txTail];
    DMA_ClearFlag(DMA1_FLAG_GL4 | DMA1_FLAG_TC4 | DMA1_FLAG_TE4 | DMA1_FLAG_HT4);
    DMA_Cmd(DMA1_Channel4, ENABLE);
}

static uint8_t DebugUart_PushByte(uint8_t data)
{
    uint16_t next;
    uint8_t ok = 0U;

    __disable_irq();
    next = (uint16_t)((txHead + 1U) % DBG_TX_BUF_SIZE);
    if (next != txTail)
    {
        txBuf[txHead] = data;
        txHead = next;
        DebugUart_KickLocked();
        ok = 1U;
    }
    __enable_irq();

    return ok;
}

void DebugUart_Init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    DMA_InitTypeDef dma;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = DEBUG_UART_BAUD;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &usart);

    DMA_DeInit(DMA1_Channel4);
    dma.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
    dma.DMA_MemoryBaseAddr = (uint32_t)txBuf;
    dma.DMA_DIR = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize = 1;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_Medium;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &dma);
    DMA_ITConfig(DMA1_Channel4, DMA_IT_TC | DMA_IT_TE, ENABLE);

    nvic.NVIC_IRQChannel = DMA1_Channel4_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
    USART_Cmd(USART1, ENABLE);
}

uint16_t DebugUart_Write(const char *str)
{
    uint16_t written = 0U;

    while ((str != 0) && (*str != '\0'))
    {
        written += DebugUart_PushByte((uint8_t)*str);
        str++;
    }

    return written;
}

uint16_t DebugUart_Printf(const char *fmt, ...)
{
    char buf[160];
    va_list ap;
    int len;
    int i;
    uint16_t written = 0U;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (len < 0)
    {
        return 0U;
    }
    if (len >= (int)sizeof(buf))
    {
        len = (int)sizeof(buf) - 1;
    }

    for (i = 0; i < len; i++)
    {
        written += DebugUart_PushByte((uint8_t)buf[i]);
    }

    return written;
}

int fputc(int ch, FILE *f)
{
    (void)f;
    DebugUart_PushByte((uint8_t)ch);
    return ch;
}

void DMA1_Channel4_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC4) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC4);
        DMA_Cmd(DMA1_Channel4, DISABLE);
        txTail = (uint16_t)((txTail + dmaLen) % DBG_TX_BUF_SIZE);
        dmaLen = 0U;
        dmaActive = 0U;
        DebugUart_KickLocked();
    }

    if (DMA_GetITStatus(DMA1_IT_TE4) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TE4);
        DMA_Cmd(DMA1_Channel4, DISABLE);
        dmaLen = 0U;
        dmaActive = 0U;
    }
}
