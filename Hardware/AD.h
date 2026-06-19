#ifndef __AD_H
#define __AD_H

#include "stdint.h"

void AD_Init(uint16_t pins);
uint16_t AD_GetValue(uint8_t ADC_Channel);

#endif
