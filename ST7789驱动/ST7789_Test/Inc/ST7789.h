#ifndef __ST7789_H
#define __ST7789_H			  	 
#include "stm32f10x.h"
#include "stdint.h"
#include "generic.h"
#define ST7789_W 240
#define ST7789_H 240

/*
bilibili 小努班 UID:437280309
@time时间: 2025.4.4
@version版本:V1_2
@attention : 如果你在使用ST7789的时候遇到了一些困难，请看我给你的解决办法txt，这里会帮助你解决问题，或者直接私信我
If you encounter some difficulties in using ST7789, please check the txt documentation I gave you(solution.txt), or send me a private message directly
更新:修复文字无法显示的问题,以及有些人无法正常使用SPI2
bug:屏幕旋转出现问题
if you can't display Chinese correctly,please check your encoding mode
*/

/*-------------STM32F103_HSPI_GPIO_PIN------------
SCK1：PA5    SCK2：PB13
MISO1：PA6   MISO2：PB14
MOSI1：PA7   MOSI2：PB15
*/

//-------------------模式选择---------------------
#define ST7789_HSPI SPI1     //选择SPI1或2,choose Hardware SPI2 or SPI1
#define SoftWare_SPI 0       //是否使用软件SPI如果使用则设置为1,do ou want to use softerware SPI?if you do,set here to 1
#define Fast_Mode 0          //是否使用快速模式如果使用则设置为1,do ou want to use FastMode?if you do,set here to 1
#define ENABLE_CS 0          //如果你有片选端，请勾选这里,if there is a CS pin in your ST7789 ,set here to 1(Actually, you don't have to)

//-----------------OLED端口定义---------------- //注意这里除了DC其它都是定义软件SPI的引脚
#define OLED_SCL(x) GPIO_WriteBit(GPIOB,GPIO_Pin_13,(BitAction)x)    //软件SCL 
#define OLED_SDA(x) GPIO_WriteBit(GPIOB,GPIO_Pin_15,(BitAction)(x))  //软件SDA
#define OLED_RST(x) GPIO_WriteBit(GPIOB,GPIO_Pin_11,(BitAction)x)    //RES
#define OLED_CS(x) GPIO_WriteBit(GPIOB,GPIO_Pin_10,(BitAction)x)      //DC

#define OLED_DC(x) GPIO_WriteBit(GPIOB,GPIO_Pin_10,(BitAction)x)     //DC
#define DC_Pin(x) 0x01<<x//寄存器操作，提升速度
//BLK可以不接
//CS可以不接,大部分ST7789默认为0,即默认为选中状态
//you can ignored your CS and BLK use defualt value

//画笔颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE         	 0x001F  
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 
#define BRRED 			 0XFC07 
#define GRAY  			 0X8430 

extern uint16_t BACK_COLOR, POINT_COLOR;//背景色，画笔色
//默认Deault: White Black
////////////////////////////////User////////////////////////////////////////////
void ST7789_Init(uint16_t Back_color,uint16_t Pen_color);
void ST7789_SetRotation(uint8_t Diraction);                         //bug
void ST7789_Clear(uint16_t Color);
void ST7789_Clear_DMA(uint16_t Color);
void ST7789_Cursor(unsigned int x1,unsigned int y1,unsigned int x2,unsigned int y2);
void ST7789_Fill(uint16_t xsta,uint16_t ysta,uint16_t xend,uint16_t yend,uint16_t color);
void ST7789_Printf(uint16_t X, uint16_t Y,const char* format, ...);
void ST7789_SlowPrint(uint16_t x,uint16_t y,const char* string);
void ST7789_PrintChinese(unsigned int x,unsigned int y,unsigned char index);
void ST7789_ShowImage_Flash(uint16_t startX, uint16_t startY,const uint8_t *Picture_ptr);
void ST7789_ShowImage_DMA(uint16_t startX, uint16_t startY,const uint8_t *Picture_ptr,uint32_t sizof_pic);

//Add 增添的不常用基础操作
void ST7789_DrawPoint(uint16_t x,uint16_t y);//画点
void ST7789_DrawPoint_big(uint16_t x,uint16_t y);//画一个大点
uint16_t  ST7789_ReadPoint(uint16_t x,uint16_t y);//读点
void ST7789_Draw_Circle(uint16_t x0,uint16_t y0,uint8_t r);//画圆
void ST7789_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);//画线
void ST7789_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);//画矩形
void ST7789_ShowChar(uint16_t x,uint16_t y,uint8_t num,uint8_t mode);//char
void ST7789_ShowNum(uint16_t x,uint16_t y,uint32_t num,uint8_t len);//Num
void ST7789_ShowString(uint16_t x,uint16_t y,char *p);//String
					  		 
#endif  
	 
	 



