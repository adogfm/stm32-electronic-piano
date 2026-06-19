#ifndef __ENCODER_H
#define __ENCODER_H

#include "stdint.h"

void Encoder_Init(void);
int8_t Encoder_Get(void);
void Encoder_Clear(void);
int16_t Encoder_Var(int16_t var, int8_t step, int16_t min, int16_t max, int16_t setL, int16_t setH);

#endif
