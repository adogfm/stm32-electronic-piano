#ifndef __TOUCHKEY_H
#define __TOUCHKEY_H

#include "stm32f10x.h"

void TouchKey_Init(void);
uint8_t TouchKey_Scan(void);

#endif