#ifndef __BUZZER_H
#define __BUZZER_H

#include "stdint.h"

void Buzzer_Init(void);
void Buzzer_OFF(uint8_t channel);
void Buzzer_ON(uint16_t freq, uint8_t tone, uint8_t channel);
void Buzzer_Timing(uint16_t freq, uint16_t timing, uint8_t tone, uint8_t channel);
void Buzzer_Reset(uint8_t channel);
void Buzzer_Drum(uint16_t type);

#endif