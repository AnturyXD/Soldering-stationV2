#include "esp_uart.h"
#include "board_config.h"

static volatile char rxQueue[ESP_UART_QUEUE_DEPTH][ESP_UART_LINE_MAX];
static volatile char build[ESP_UART_LINE_MAX];
static volatile uint8_t writeIdx;
static volatile uint8_t readIdx;
static volatile uint8_t count;
static volatile uint16_t buildIdx;
static volatile uint8_t discard;

#define ESP_TX_BUF_SIZE 1024U
static volatile uint16_t txHead;
static volatile uint16_t txTail;
static uint8_t txBuf[ESP_TX_BUF_SIZE];

static void QueuePushFromIrq(void)
{
    uint8_t slot = writeIdx;
    uint8_t i;

    build[buildIdx] = '\0';
    for (i = 0; i < ESP_UART_LINE_MAX; i++)
    {
        rxQueue[slot][i] = build[i];
        if (build[i] == '\0')
        {
            break;
        }
    }

    writeIdx = (uint8_t)((writeIdx + 1U) % ESP_UART_QUEUE_DEPTH);
    if (count < ESP_UART_QUEUE_DEPTH)
    {
        count++;
    }
    else
    {
        readIdx = (uint8_t)((readIdx + 1U) % ESP_UART_QUEUE_DEPTH);
    }
    buildIdx = 0U;
}

void EspUart_Init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_2;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_3;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);

    usart.USART_BaudRate = ESP_UART_BAUD;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &usart);

    nvic.NVIC_IRQChannel = USART2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
}

void EspUart_SendString(const char *str)
{
    /* ESP上报不允许阻塞主循环：只写入发送队列，真正发送由TXE中断完成。 */
    while ((str != 0) && (*str != '\0'))
    {
        uint16_t next;

        __disable_irq();
        next = (uint16_t)((txHead + 1U) % ESP_TX_BUF_SIZE);
        if (next == txTail)
        {
            __enable_irq();
            return;
        }
        txBuf[txHead] = (uint8_t)*str++;
        txHead = next;
        USART_ITConfig(USART2, USART_IT_TXE, ENABLE);
        __enable_irq();
    }
}

uint8_t EspUart_GetLine(char *buf, uint16_t maxLen)
{
    uint16_t i;

    if ((buf == 0) || (maxLen == 0U))
    {
        return 0U;
    }

    __disable_irq();
    if (count == 0U)
    {
        __enable_irq();
        return 0U;
    }

    for (i = 0; (i < (maxLen - 1U)) && (i < ESP_UART_LINE_MAX); i++)
    {
        buf[i] = (char)rxQueue[readIdx][i];
        if (buf[i] == '\0')
        {
            break;
        }
    }
    buf[maxLen - 1U] = '\0';
    readIdx = (uint8_t)((readIdx + 1U) % ESP_UART_QUEUE_DEPTH);
    count--;
    __enable_irq();

    return 1U;
}

void USART2_IRQHandler(void)
{
    uint8_t data;

    if (USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
    {
        if (txTail != txHead)
        {
            USART_SendData(USART2, txBuf[txTail]);
            txTail = (uint16_t)((txTail + 1U) % ESP_TX_BUF_SIZE);
        }
        else
        {
            USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
        }
    }

    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
        data = (uint8_t)USART_ReceiveData(USART2);

        if (discard)
        {
            if ((data == '\r') || (data == '\n'))
            {
                discard = 0U;
                buildIdx = 0U;
            }
            return;
        }

        if ((data == '\r') || (data == '\n'))
        {
            if (buildIdx > 0U)
            {
                QueuePushFromIrq();
            }
        }
        else if (buildIdx < (ESP_UART_LINE_MAX - 1U))
        {
            build[buildIdx++] = (char)data;
        }
        else
        {
            buildIdx = 0U;
            discard = 1U;
        }
    }
}
