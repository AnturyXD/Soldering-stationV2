#include "lower_uart.h"
#include "board_config.h"

static volatile char rxQueue[LOWER_UART_QUEUE_DEPTH][LOWER_UART_LINE_MAX];
static volatile char build[LOWER_UART_LINE_MAX];
static volatile uint8_t writeIdx;
static volatile uint8_t readIdx;
static volatile uint8_t count;
static volatile uint16_t buildIdx;
static volatile uint8_t frameActive;
static volatile uint32_t rxByteCount;
static volatile uint32_t overflowCount;
static volatile uint32_t overrunCount;
static volatile uint32_t frameErrorCount;
static volatile uint32_t noiseErrorCount;
static volatile uint32_t parityErrorCount;
static volatile uint8_t recentBytes[LOWER_UART_RECENT_BYTES];
static volatile uint8_t recentWriteIdx;
static volatile uint8_t recentCount;

static uint8_t IsFrameChar(uint8_t data)
{
    return ((data >= ' ') && (data <= '~')) ? 1U : 0U;
}

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
    /* PB11 是下位机状态串口输入。使用上拉输入可以让空闲电平更稳定，
     * 减少复位、插拔或分压点高阻时的误触发。
     */
    gpio.GPIO_Mode = GPIO_Mode_IPU;
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
    nvic.NVIC_IRQChannelSubPriority = 0;
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

uint32_t LowerUart_GetOverrunCount(void)
{
    uint32_t value;

    __disable_irq();
    value = overrunCount;
    __enable_irq();
    return value;
}

uint32_t LowerUart_GetFrameErrorCount(void)
{
    uint32_t value;

    __disable_irq();
    value = frameErrorCount;
    __enable_irq();
    return value;
}

uint32_t LowerUart_GetNoiseErrorCount(void)
{
    uint32_t value;

    __disable_irq();
    value = noiseErrorCount;
    __enable_irq();
    return value;
}

uint32_t LowerUart_GetParityErrorCount(void)
{
    uint32_t value;

    __disable_irq();
    value = parityErrorCount;
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
    uint16_t status;
    uint8_t data;
    uint8_t hasRx;
    uint8_t hasHwError;

    status = USART3->SR;
    hasRx = ((status & USART_SR_RXNE) != 0U) ? 1U : 0U;
    hasHwError = ((status & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) != 0U) ? 1U : 0U;

    if (hasRx || hasHwError)
    {
        data = (uint8_t)USART3->DR;

        if (hasRx)
        {
            rxByteCount++;
            recentBytes[recentWriteIdx] = data;
            recentWriteIdx = (uint8_t)((recentWriteIdx + 1U) % LOWER_UART_RECENT_BYTES);
            if (recentCount < LOWER_UART_RECENT_BYTES)
            {
                recentCount++;
            }
        }

        if ((status & USART_SR_ORE) != 0U)
        {
            overrunCount++;
        }
        if ((status & USART_SR_FE) != 0U)
        {
            frameErrorCount++;
        }
        if ((status & USART_SR_NE) != 0U)
        {
            noiseErrorCount++;
        }
        if ((status & USART_SR_PE) != 0U)
        {
            parityErrorCount++;
        }

        if (hasHwError)
        {
            /* USART硬件已经报告该字节异常，当前帧不可信，重新等待 '$' 同步。 */
            frameActive = 0U;
            buildIdx = 0U;
            return;
        }

        /* 下位机协议使用 '$' 起帧；未同步前的噪声字节全部忽略。 */
        if (data == '$')
        {
            frameActive = 1U;
            buildIdx = 0U;
            build[buildIdx++] = (char)data;
            return;
        }

        if (!frameActive)
        {
            return;
        }

        if (data == '\n')
        {
            QueuePushFromIrq();
            frameActive = 0U;
        }
        else if (!IsFrameChar(data))
        {
            /* 帧内出现非 ASCII 可打印字符，说明本帧已经损坏，直接丢弃等待下一个 '$'。 */
            frameActive = 0U;
            buildIdx = 0U;
            overflowCount++;
        }
        else if (buildIdx < (LOWER_UART_FRAME_MAX - 1U))
        {
            build[buildIdx++] = (char)data;
        }
        else
        {
            /* 最大帧长 32 字节，包含结尾 LF；超长帧不进入解析层。 */
            frameActive = 0U;
            buildIdx = 0U;
            overflowCount++;
        }
    }
}
