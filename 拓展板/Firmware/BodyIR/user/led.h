/**
* @par Copyright (C): 2022-, Shenzhen Yahboom Tech
* @file         // rgbd.h
* @author       //wu
* @version      // 1.0
* @date         // 2022.12.14
* @brief        // rgb灯头文件
* @details      // 定义rgb灯的管脚、时钟和函数声明
* @par History  // 
*               //   
* version:		wu		2022.12.14		
*/



#ifndef __LED_H_
#define __LED_H_

#include "stm32f10x.h"

#define LED_PORT  GPIOC
#define LED_GPIO_CLK RCC_APB2Periph_GPIOC//GPIOB时钟
#define LED_GPIO_PIN GPIO_Pin_13//PC13
void LED(uint16_t t);
void LED_Init(void);

	
#endif
