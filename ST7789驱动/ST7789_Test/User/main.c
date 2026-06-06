#include "stm32f10x.h"
#include "generic.h"		//常用的头文件		
#include "ST7789.h"
#include "Picture.h"    //照片放在这里
#include "DMA.h"

/*
例程默认接线方式:使用硬件SPI1
SCL = A5
SDA = A7
RES = B11
DC  = B10
BLK 不接
DC 不接(或接地)
*/

int main(void)
{
  uint8_t flag = 0;
  uint8_t xposition = 0;
  uint8_t yposition = 0;
  ST7789_Init(WHITE,RED);
  ST7789_Clear(WHITE);
	POINT_COLOR = RED;
	while(1)
	{
    //刷屏检测
    if(flag == 0){
      for(int i =20;i>0;i--){
        ST7789_Clear_DMA(RED);
        Delay_ms(10*i);
        ST7789_Clear_DMA(GREEN);
        Delay_ms(10*i);
        ST7789_Clear_DMA(BLUE);
        Delay_ms(10*i);
      }
      for(int i = 40;i>0;i--){
        ST7789_Clear_DMA(RED);
        ST7789_Clear_DMA(GREEN);
        ST7789_Clear_DMA(BLUE);
      }
      //重新设置背景颜色
      ST7789_Clear(WHITE);
      //DMA显示图片
      ST7789_ShowImage_DMA(50, 0,gImage_test_pic,sizeof(gImage_test_pic));
      ST7789_SlowPrint(20,140,"hello,everyone i am benjiamin");
      ST7789_SlowPrint(0,154,"welcom to use our ST7789");       
      ST7789_SlowPrint(0,166,"Driver.if you suffer some ");
      ST7789_SlowPrint(0,178,"problems,please check our");
      ST7789_SlowPrint(0,190,"solution.txt"); 
      flag = 1;
    }
    //显示跳跃的小人
    ST7789_ShowImage_DMA(xposition, yposition,gImage_pp1,sizeof(gImage_pp1));
    Delay_ms(30);
    ST7789_ShowImage_DMA(xposition, yposition,gImage_pp2,sizeof(gImage_pp2));
    Delay_ms(30);
    ST7789_ShowImage_DMA(xposition, yposition,gImage_pp3,sizeof(gImage_pp3));
    Delay_ms(30);
    ST7789_ShowImage_DMA(xposition, yposition,gImage_pp4,sizeof(gImage_pp4));
    Delay_ms(30);
    xposition++;
    yposition++;
    if(xposition>=200)xposition=0;
    if(yposition>=200)yposition=0;
    //ST7789_ShowImage_Flash(0,0,gImage_test_pic);
	}
}
