#include "lower_uart.h"
#include "board_config.h"

static volatile char rxQueue[LOWER_UART_QUEUE_DEPTH][LOWER_UART_LINE_MAX];
static volatile char build[LOWER_UART_LINE_MAX];
static volatile uint8_t writeIdx;
static volatile uint8_t readIdx;
static volatile uint8_t count;
static volatile uint16_t buildIdx;
static volatile uint8_t discard;
static volatile uint32_t rxByteCount;
static volatile uint32_t overflowCount;
static volatile uint8_t recentBytes[LOWER_UART_RECENT_BYTES];
static volatile uint8_t recentWriteIdx;
static volatile uint8_t recentCount;

static void QueuePushFromIrq(void)
{
    uint8_t slot = writeIdx;
    uint8_t i;

    build[buildIdx] = '\0';
    for (i = 0; i < LOWER_UART_LINE_MAX; i++)
    {
        rxQueue[slot][i] = build[i];
        if (build[i] == '\0')
        {
            break;
        }
    }

    writeIdx = (uint8_t)((writeIdx + 1U) % LOWER_UART_QUEUE_DEPTH);
    if (count < LOWER_UART_QUEUE_DEPTH)
    {
        count++;
    }
    else
    {
        readIdx = (uint8_t)((readIdx + 1U) % LOWER_UART_QUEUE_DEPTH);
        overflowCount++;
    }
    buildIdx = 0U;
}

void LowerUart_Init(void)
{
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    NVIC_InitTypeDef nvic;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_11;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    usart.USART_BaudRate = LOWER_UART_BAUD;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx;
    USART_Init(USART3, &usart);

    nvic.NVIC_IRQChannel = USART3_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 2;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART3, ENABLE);
}

uint32_t LowerUart_GetRxByteCount(void)
{
    uint32_t value;

    __disable_irq();
    value = rxByteCount;
    __enable_irq();
    return value;
}

uint32_t LowerUart_GetOverflowCount(void)
{
    uint32_t value;

    __disable_irq();
    value = overflowCount;
    __enable_irq();
    return value;
}

void LowerUart_GetRecentHex(char *buf, uint16_t maxLen)
{
    static const char hex[] = "0123456789ABCDEF";
    uint8_t bytes[LOWER_UART_RECENT_BYTES];
    uint8_t countCopy;
    uint8_t start;
    uint8_t i;
    uint16_t out = 0U;

    if ((buf == 0) || (maxLen == 0U))
    {
        return;
    }

    __disable_irq();
    countCopy = recentCount;
    start = (uint8_t)((recentWriteIdx + LOWER_UART_RECENT_BYTES - countCopy) % LOWER_UART_RECENT_BYTES);
    for (i = 0U; i < countCopy; i++)
    {
        bytes[i] = recentBytes[(uint8_t)((start + i) % LOWER_UART_RECENT_BYTES)];
    }
    __enable_irq();

    for (i = 0U; i < countCopy; i++)
    {
        if ((out + 3U) >= maxLen)
        {
            break;
        }
        buf[out++] = hex[(bytes[i] >> 4) & 0x0FU];
        buf[out++] = hex[bytes[i] & 0x0FU];
        if (i + 1U < countCopy)
        {
            buf[out++] = ' ';
        }
    }
    buf[out] = '\0';
}

uint8_t LowerUart_GetLine(char *buf, uint16_t maxLen)
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

    for (i = 0; (i < (maxLen - 1U)) && (i < LOWER_UART_LINE_MAX); i++)
    {
        buf[i] = (char)rxQueue[readIdx][i];
        if (buf[i] == '\0')
        {
            break;
        }
    }
    buf[maxLen - 1U] = '\0';
    readIdx = (uint8_t)((readIdx + 1U) % LOWER_UART_QUEUE_DEPTH);
    count--;
    __enable_irq();

    return 1U;
}

void USART3_IRQHandler(void)
{
    uint8_t data;

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
        data = (uint8_t)USART_ReceiveData(USART3);
        rxByteCount++;
        recentBytes[recentWriteIdx] = data;
        recentWriteIdx = (uint8_t)((recentWriteIdx + 1U) % LOWER_UART_RECENT_BYTES);
        if (recentCount < LOWER_UART_RECENT_BYTES)
        {
            recentCount++;
        }

        if (discard)
        {
            if (data == '\n')
            {
                discard = 0U;
                buildIdx = 0U;
            }
            return;
        }

        if (data == '\r')
        {
            return;
        }

        if (data == '\n')
        {
            if (buildIdx > 0U)
            {
                QueuePushFromIrq();
            }
        }
        else if (buildIdx < (LOWER_UART_LINE_MAX - 1U))
        {
            build[buildIdx++] = (char)data;
        }
        else
        {
            buildIdx = 0U;
            discard = 1U;
            overflowCount++;
        }
    }
}
