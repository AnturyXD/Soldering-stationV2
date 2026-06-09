#include "stm32f10x.h"
#include "bodyir.h"

void Bdir_init(void)//人体红外模块管脚配置
{
	GPIO_InitTypeDef GPIO_InitStructure; //定义结构体
	
	RCC_APB2PeriphClockCmd(BDYIR_GPIO_CLK,ENABLE);//开启时钟
	
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;//设置浮空输入
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;//设置时钟频率
	GPIO_InitStructure.GPIO_Pin=BDYIR_GPIO_PIN;//定义管脚
	
	GPIO_Init(BDYIR_PORT,&GPIO_InitStructure);//初始化

	
}

