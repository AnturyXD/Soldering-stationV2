#include "stm32f10x.h"                  // Device header
#include "DMA.h"

//（DMA通道,外设地址，内存地址，缓冲区大小）
uint8_t SendBuff[480];
void MYDMA_Init(DMA_Channel_TypeDef* DMA_CHx,uint32_t Add_Periph,uint32_t Add_Memory,uint16_t BufferSize)
{
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输
		DMA_InitTypeDef DMA_InitStructure;
		DMA_InitStructure.DMA_PeripheralBaseAddr = Add_Periph;
		DMA_InitStructure.DMA_MemoryBaseAddr = Add_Memory;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  //数据传输方向，从内存读取发送到外设
		DMA_InitStructure.DMA_BufferSize = BufferSize;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;//once
		DMA_InitStructure.DMA_Priority = DMA_Priority_High;
		DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
		DMA_Init(DMA_CHx, &DMA_InitStructure);
}
 
void DMA_Transfer(DMA_Channel_TypeDef*DMA_CHx,uint16_t times)
{
	DMA_Cmd(DMA_CHx, DISABLE);
	DMA_SetCurrDataCounter(DMA_CHx, times);
	DMA_Cmd(DMA_CHx, ENABLE);

	if(DMA_CHx==DMA1_Channel1){
		while(DMA_GetFlagStatus(DMA1_FLAG_TC1)==RESET);
		DMA_ClearFlag(DMA1_FLAG_TC1);
	}
	else if(DMA_CHx==DMA1_Channel2){
		while(DMA_GetFlagStatus(DMA1_FLAG_TC2)==RESET);
		DMA_ClearFlag(DMA1_FLAG_TC2);
	}
	else if(DMA_CHx==DMA1_Channel3){
		while(DMA_GetFlagStatus(DMA1_FLAG_TC3)==RESET);
		DMA_ClearFlag(DMA1_FLAG_TC3);
	}
	else if(DMA_CHx==DMA1_Channel4){
		while(DMA_GetFlagStatus(DMA1_FLAG_TC4)==RESET);
		DMA_ClearFlag(DMA1_FLAG_TC4);
	}
	else if(DMA_CHx==DMA1_Channel5){
		while(DMA_GetFlagStatus(DMA1_FLAG_TC5)==RESET);
		DMA_ClearFlag(DMA1_FLAG_TC5);
	}
}
