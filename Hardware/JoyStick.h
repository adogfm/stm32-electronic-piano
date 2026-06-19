#ifndef __JOYSTICK_H
#define __JOYSTICK_H

#include "stdint.h"

void JoyStick_Init(void);
uint8_t JoyStick_GetSW(void);
uint16_t JoyStick_GetX(void);
uint16_t JoyStick_GetY(void);

#endif
