#ifndef __BOARD_IO_H
#define __BOARD_IO_H

#include "stm32f10x.h"

void BoardIo_Init(void);
void BoardIo_Task(uint32_t nowMs, uint8_t espOnline);

#endif
