#ifndef __KEY_H
#define __KEY_H

#include "stdint.h"

void Key_Init_A(uint16_t pins);
void Key_Init_B(uint16_t pins);
uint8_t Key_Read_A(uint16_t pins);
uint8_t Key_Read_B(uint16_t pins);
int16_t Key_Var_Plus(int16_t var, int8_t step, int16_t max, int16_t set);

#endif
