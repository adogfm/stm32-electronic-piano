#ifndef __LED_H
#define __LED_H

#include "stdint.h"

void LED_Init_A(uint16_t pins);
void LED_Init_B(uint16_t pins);
void LED_ON_A(uint16_t pins);
void LED_OFF_A(uint16_t pins);
void LED_ON_B(uint16_t pins);
void LED_OFF_B(uint16_t pins);

#endif
