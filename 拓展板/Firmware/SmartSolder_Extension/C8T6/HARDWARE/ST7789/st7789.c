#include "st7789.h"
#include "delay.h"

#define ST7789_SCL_PIN    GPIO_Pin_5
#define ST7789_SDA_PIN    GPIO_Pin_7
#define ST7789_BLK_PIN    GPIO_Pin_8
#define ST7789_RES_PORT   GPIOB
#define ST7789_RES_PIN    GPIO_Pin_0
#define ST7789_DC_PORT    GPIOB
#define ST7789_DC_PIN     GPIO_Pin_1

#define RES_HIGH() GPIO_SetBits(ST7789_RES_PORT, ST7789_RES_PIN)
#define RES_LOW()  GPIO_ResetBits(ST7789_RES_PORT, ST7789_RES_PIN)
#define DC_HIGH()  GPIO_SetBits(ST7789_DC_PORT, ST7789_DC_PIN)
#define DC_LOW()   GPIO_ResetBits(ST7789_DC_PORT, ST7789_DC_PIN)

static uint8_t dmaRowBuf[ST7789_WIDTH * 2U];
static volatile uint8_t dmaActive;
static volatile uint8_t fillActive;
static uint16_t fillRowsTotal;
static uint16_t fillRowsDone;
static uint16_t fillRowBytes;

/* 屏幕大块区域刷新采用“主循环推进 + DMA搬运”的方式：
 * DMA中断只标记本行完成，不在中断里继续刷下一行，避免影响串口接收。
 */
static uint8_t SpiBusy(void)
{
    return (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) ? 1U : 0U;
}

static void ST7789_WaitReady(void)
{
    while (ST7789_IsBusy())
    {
        ST7789_Task();
    }
}

static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x00,0x00,0x5F,0x00,0x00}, /* ! */
    {0x00,0x07,0x00,0x07,0x00}, /* " */
    {0x14,0x7F,0x14,0x7F,0x14}, /* # */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* $ */
    {0x23,0x13,0x08,0x64,0x62}, /* % */
    {0x36,0x49,0x55,0x22,0x50}, /* & */
    {0x00,0x05,0x03,0x00,0x00}, /* ' */
    {0x00,0x1C,0x22,0x41,0x00}, /* ( */
    {0x00,0x41,0x22,0x1C,0x00}, /* ) */
    {0x14,0x08,0x3E,0x08,0x14}, /* * */
    {0x08,0x08,0x3E,0x08,0x08}, /* + */
    {0x00,0x50,0x30,0x00,0x00}, /* , */
    {0x08,0x08,0x08,0x08,0x08}, /* - */
    {0x00,0x60,0x60,0x00,0x00}, /* . */
    {0x20,0x10,0x08,0x04,0x02}, /* / */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 0 */
    {0x00,0x42,0x7F,0x40,0x00}, /* 1 */
    {0x42,0x61,0x51,0x49,0x46}, /* 2 */
    {0x21,0x41,0x45,0x4B,0x31}, /* 3 */
    {0x18,0x14,0x12,0x7F,0x10}, /* 4 */
    {0x27,0x45,0x45,0x45,0x39}, /* 5 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 6 */
    {0x01,0x71,0x09,0x05,0x03}, /* 7 */
    {0x36,0x49,0x49,0x49,0x36}, /* 8 */
    {0x06,0x49,0x49,0x29,0x1E}, /* 9 */
    {0x00,0x36,0x36,0x00,0x00}, /* : */
    {0x00,0x56,0x36,0x00,0x00}, /* ; */
    {0x08,0x14,0x22,0x41,0x00}, /* < */
    {0x14,0x14,0x14,0x14,0x14}, /* = */
    {0x00,0x41,0x22,0x14,0x08}, /* > */
    {0x02,0x01,0x51,0x09,0x06}, /* ? */
    {0x32,0x49,0x79,0x41,0x3E}, /* @ */
    {0x7E,0x11,0x11,0x11,0x7E}, /* A */
    {0x7F,0x49,0x49,0x49,0x36}, /* B */
    {0x3E,0x41,0x41,0x41,0x22}, /* C */
    {0x7F,0x41,0x41,0x22,0x1C}, /* D */
    {0x7F,0x49,0x49,0x49,0x41}, /* E */
    {0x7F,0x09,0x09,0x09,0x01}, /* F */
    {0x3E,0x41,0x49,0x49,0x7A}, /* G */
    {0x7F,0x08,0x08,0x08,0x7F}, /* H */
    {0x00,0x41,0x7F,0x41,0x00}, /* I */
    {0x20,0x40,0x41,0x3F,0x01}, /* J */
    {0x7F,0x08,0x14,0x22,0x41}, /* K */
    {0x7F,0x40,0x40,0x40,0x40}, /* L */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* M */
    {0x7F,0x04,0x08,0x10,0x7F}, /* N */
    {0x3E,0x41,0x41,0x41,0x3E}, /* O */
    {0x7F,0x09,0x09,0x09,0x06}, /* P */
    {0x3E,0x41,0x51,0x21,0x5E}, /* Q */
    {0x7F,0x09,0x19,0x29,0x46}, /* R */
    {0x46,0x49,0x49,0x49,0x31}, /* S */
    {0x01,0x01,0x7F,0x01,0x01}, /* T */
    {0x3F,0x40,0x40,0x40,0x3F}, /* U */
    {0x1F,0x20,0x40,0x20,0x1F}, /* V */
    {0x3F,0x40,0x38,0x40,0x3F}, /* W */
    {0x63,0x14,0x08,0x14,0x63}, /* X */
    {0x07,0x08,0x70,0x08,0x07}, /* Y */
    {0x61,0x51,0x49,0x45,0x43}  /* Z */
};

static void ST7789_WriteByte(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
    {
    }
    SPI_I2S_SendData(SPI1, data);
    while (SpiBusy())
    {
    }
}

static void ST7789_WriteCommand(uint8_t cmd)
{
    DC_LOW();
    ST7789_WriteByte(cmd);
}

static void ST7789_WriteData8(uint8_t data)
{
    DC_HIGH();
    ST7789_WriteByte(data);
}

static void ST7789_WriteData16(uint16_t data)
{
    DC_HIGH();
    ST7789_WriteByte((uint8_t)(data >> 8));
    ST7789_WriteByte((uint8_t)data);
}

static void ST7789_SetAddressWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t x2 = (uint16_t)(x + w - 1U);
    uint16_t y2 = (uint16_t)(y + h - 1U);

    ST7789_WriteCommand(0x2A);
    ST7789_WriteData16(x);
    ST7789_WriteData16(x2);

    ST7789_WriteCommand(0x2B);
    ST7789_WriteData16(y);
    ST7789_WriteData16(y2);

    ST7789_WriteCommand(0x2C);
}

static void ST7789_StartDma(uint16_t len)
{
    if ((len == 0U) || dmaActive || SpiBusy())
    {
        return;
    }

    DC_HIGH();
    dmaActive = 1U;
    DMA_Cmd(DMA1_Channel3, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel3, len);
    DMA1_Channel3->CMAR = (uint32_t)dmaRowBuf;
    DMA_ClearFlag(DMA1_FLAG_GL3 | DMA1_FLAG_TC3 | DMA1_FLAG_TE3 | DMA1_FLAG_HT3);
    DMA_Cmd(DMA1_Channel3, ENABLE);
}

static void ST7789_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;
    DMA_InitTypeDef dma;
    TIM_TimeBaseInitTypeDef tim;
    TIM_OCInitTypeDef oc;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_SPI1 | RCC_APB2Periph_TIM1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    gpio.GPIO_Pin = ST7789_SCL_PIN | ST7789_SDA_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = ST7789_BLK_PIN;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Pin = ST7789_RES_PIN | ST7789_DC_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &gpio);

    DC_HIGH();
    RES_HIGH();

    spi.SPI_Direction = SPI_Direction_1Line_Tx;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_High;
    spi.SPI_CPHA = SPI_CPHA_2Edge;
    spi.SPI_NSS = SPI_NSS_Soft;
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
    spi.SPI_FirstBit = SPI_FirstBit_MSB;
    spi.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &spi);
    SPI_Cmd(SPI1, ENABLE);

    DMA_DeInit(DMA1_Channel3);
    dma.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;
    dma.DMA_MemoryBaseAddr = (uint32_t)dmaRowBuf;
    dma.DMA_DIR = DMA_DIR_PeripheralDST;
    dma.DMA_BufferSize = 1;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    dma.DMA_Mode = DMA_Mode_Normal;
    dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel3, &dma);
    DMA_ITConfig(DMA1_Channel3, DMA_IT_TC | DMA_IT_TE, ENABLE);
    SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

    nvic.NVIC_IRQChannel = DMA1_Channel3_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 1;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    tim.TIM_Period = 1000 - 1;
    tim.TIM_Prescaler = 72 - 1;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &tim);

    oc.TIM_OCMode = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_OutputNState = TIM_OutputNState_Disable;
    oc.TIM_Pulse = 1000 - 1;
    oc.TIM_OCPolarity = TIM_OCPolarity_High;
    oc.TIM_OCNPolarity = TIM_OCNPolarity_High;
    oc.TIM_OCIdleState = TIM_OCIdleState_Reset;
    oc.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
    TIM_OC1Init(TIM1, &oc);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
    ST7789_SetBacklight(85U);
}

void ST7789_Init(void)
{
    ST7789_GPIO_Init();

    RES_LOW();
    delay_ms(20);
    RES_HIGH();
    delay_ms(120);

    ST7789_WriteCommand(0x36);
    ST7789_WriteData8(0x00);

    ST7789_WriteCommand(0x3A);
    ST7789_WriteData8(0x05);

    ST7789_WriteCommand(0xB2);
    ST7789_WriteData8(0x0C);
    ST7789_WriteData8(0x0C);
    ST7789_WriteData8(0x00);
    ST7789_WriteData8(0x33);
    ST7789_WriteData8(0x33);

    ST7789_WriteCommand(0xB7);
    ST7789_WriteData8(0x35);

    ST7789_WriteCommand(0xBB);
    ST7789_WriteData8(0x19);

    ST7789_WriteCommand(0xC0);
    ST7789_WriteData8(0x2C);

    ST7789_WriteCommand(0xC2);
    ST7789_WriteData8(0x01);

    ST7789_WriteCommand(0xC3);
    ST7789_WriteData8(0x12);

    ST7789_WriteCommand(0xC4);
    ST7789_WriteData8(0x20);

    ST7789_WriteCommand(0xC6);
    ST7789_WriteData8(0x0F);

    ST7789_WriteCommand(0xD0);
    ST7789_WriteData8(0xA4);
    ST7789_WriteData8(0xA1);

    ST7789_WriteCommand(0x11);
    delay_ms(120);

    ST7789_WriteCommand(0x29);
    delay_ms(20);

    ST7789_FillScreen(ST7789_COLOR_BLACK);
}

void ST7789_Task(void)
{
    if (!fillActive || dmaActive || SpiBusy())
    {
        return;
    }

    if (fillRowsDone >= fillRowsTotal)
    {
        fillActive = 0U;
        return;
    }

    ST7789_StartDma(fillRowBytes);
    fillRowsDone++;
}

uint8_t ST7789_IsBusy(void)
{
    return (uint8_t)(fillActive || dmaActive || SpiBusy());
}

void ST7789_SetBacklight(uint8_t percent)
{
    uint32_t pulse;

    if (percent > 100U)
    {
        percent = 100U;
    }

    pulse = ((uint32_t)percent * 999UL) / 100UL;
    TIM_SetCompare1(TIM1, (uint16_t)pulse);
}

void ST7789_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint32_t count;

    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT) || (w == 0U) || (h == 0U))
    {
        return;
    }

    ST7789_WaitReady();

    if ((x + w) > ST7789_WIDTH)
    {
        w = (uint16_t)(ST7789_WIDTH - x);
    }
    if ((y + h) > ST7789_HEIGHT)
    {
        h = (uint16_t)(ST7789_HEIGHT - y);
    }

    ST7789_SetAddressWindow(x, y, w, h);
    for (count = 0; count < ((uint32_t)w * (uint32_t)h); count++)
    {
        ST7789_WriteData16(color);
    }
}

uint8_t ST7789_StartFillRectAsync(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t i;

    if (ST7789_IsBusy() || (x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT) || (w == 0U) || (h == 0U))
    {
        return 0U;
    }

    if ((x + w) > ST7789_WIDTH)
    {
        w = (uint16_t)(ST7789_WIDTH - x);
    }
    if ((y + h) > ST7789_HEIGHT)
    {
        h = (uint16_t)(ST7789_HEIGHT - y);
    }

    for (i = 0; i < w; i++)
    {
        dmaRowBuf[i * 2U] = (uint8_t)(color >> 8);
        dmaRowBuf[i * 2U + 1U] = (uint8_t)color;
    }

    ST7789_SetAddressWindow(x, y, w, h);
    fillRowBytes = (uint16_t)(w * 2U);
    fillRowsTotal = h;
    fillRowsDone = 0U;
    fillActive = 1U;
    ST7789_Task();

    return 1U;
}

void ST7789_FillScreen(uint16_t color)
{
    ST7789_FillRect(0, 0, ST7789_WIDTH, ST7789_HEIGHT, color);
}

void ST7789_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x >= ST7789_WIDTH) || (y >= ST7789_HEIGHT))
    {
        return;
    }

    ST7789_WaitReady();
    ST7789_SetAddressWindow(x, y, 1, 1);
    ST7789_WriteData16(color);
}

void ST7789_DrawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
    int16_t dx = (x0 > x1) ? (int16_t)(x0 - x1) : (int16_t)(x1 - x0);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = (y0 > y1) ? (int16_t)(y0 - y1) : (int16_t)(y1 - y0);
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = (dx > dy ? dx : -dy) / 2;
    int16_t e2;

    for (;;)
    {
        ST7789_DrawPixel(x0, y0, color);
        if ((x0 == x1) && (y0 == y1))
        {
            break;
        }
        e2 = err;
        if (e2 > -dx)
        {
            err -= dy;
            x0 = (uint16_t)((int16_t)x0 + sx);
        }
        if (e2 < dy)
        {
            err += dx;
            y0 = (uint16_t)((int16_t)y0 + sy);
        }
    }
}

void ST7789_DrawChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg, uint8_t scale)
{
    uint8_t c;
    uint8_t col;
    uint8_t row;
    uint8_t bits;

    if (scale == 0U)
    {
        scale = 1U;
    }

    if ((ch >= 'a') && (ch <= 'z'))
    {
        ch = (char)(ch - 32);
    }

    if ((ch < ' ') || (ch > 'Z'))
    {
        ch = '?';
    }

    c = (uint8_t)(ch - ' ');

    ST7789_FillRect(x, y, (uint16_t)(6U * scale), (uint16_t)(8U * scale), bg);

    for (col = 0; col < 6U; col++)
    {
        bits = (col < 5U) ? font5x7[c][col] : 0x00U;
        for (row = 0; row < 8U; row++)
        {
            if (bits & (1U << row))
            {
                ST7789_FillRect((uint16_t)(x + col * scale),
                                (uint16_t)(y + row * scale),
                                scale,
                                scale,
                                color);
            }
        }
    }
}

void DMA1_Channel3_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC3) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC3);
        DMA_Cmd(DMA1_Channel3, DISABLE);
        dmaActive = 0U;
    }

    if (DMA_GetITStatus(DMA1_IT_TE3) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TE3);
        DMA_Cmd(DMA1_Channel3, DISABLE);
        dmaActive = 0U;
        fillActive = 0U;
    }
}

void ST7789_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg, uint8_t scale)
{
    while ((str != 0) && (*str != '\0'))
    {
        ST7789_DrawChar(x, y, *str, color, bg, scale);
        x = (uint16_t)(x + 6U * scale);
        str++;
        if (x >= (ST7789_WIDTH - 6U * scale))
        {
            break;
        }
    }
}
