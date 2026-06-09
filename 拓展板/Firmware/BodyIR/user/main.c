/***源码***/

#include "stm32f10x.h"
#include "bodyir.h"
#include "led.h"

int main(void)
{
	
		LED_Init();//rgb灯初始化
		Bdir_init();//人体红外初始化
		
	
	while(1)
		{
			u8 value;
			value=GPIO_ReadInputDataBit(BDYIR_PORT, BDYIR_GPIO_PIN); //读取PB9引脚
			if(value == 0)
			{
				   LED(0); //无人关灯
			}
			else
			{
				LED(1);  //有人亮灯    
			}
				
		}
	
}
