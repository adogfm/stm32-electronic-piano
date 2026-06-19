#ifndef __PWM_H
#define __PWM_H

#include "stdint.h"

void PWM_Init_1(void);
void PWM_Init_2(void);
void PWM_SetDuty_1(uint16_t duty);
void PWM_SetDuty_2(uint16_t duty);
void PWM_SetFreq_1(uint16_t freq);
void PWM_SetFreq_2(uint16_t freq);

#endif
