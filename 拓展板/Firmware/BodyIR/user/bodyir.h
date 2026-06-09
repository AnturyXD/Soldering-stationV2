#ifndef __BODYIR_H_
#define __BODYIR_H_

#include "stm32f10x.h"

#define BDYIR_PORT  GPIOB
#define BDYIR_GPIO_CLK RCC_APB2Periph_GPIOB//GPIOB时钟
#define BDYIR_GPIO_PIN GPIO_Pin_9//PB9

void Bdir_init(void);

#endif
