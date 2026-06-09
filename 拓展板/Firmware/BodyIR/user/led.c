/**
* @par Copyright (C): 2022-, Shenzhen Yahboom Tech
* @file         // rgbd.c
* @author       // wu
* @version      // 1.0
* @date         // 2022.12.14
* @brief        // rgb灯的初始化、rgb灯状态定义
* @details      // void RGBD_Init(void)： rgb都能管脚初始化
* @par History  // 
*               // 
* version:		wu		2022.12.14		
*/


#include "stm32f10x.h"
#include "led.h"

void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; //定义结构体
	
	RCC_APB2PeriphClockCmd(LED_GPIO_CLK,ENABLE);//开启时钟
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;//设置推挽输出
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//设置时钟频率
	
	GPIO_InitStructure.GPIO_Pin=LED_GPIO_PIN;//设置led灯引脚
	GPIO_Init(LED_PORT,&GPIO_InitStructure);
	
	
	GPIO_SetBits(LED_PORT,LED_GPIO_PIN);//关闭led灯

	
}

void LED(uint16_t t)//设置0灯灭、1灯亮
{
	if(t==0)
	{
		GPIO_SetBits(LED_PORT,LED_GPIO_PIN);
	}
	else 
	{
		GPIO_ResetBits(LED_PORT,LED_GPIO_PIN);
	}
}

