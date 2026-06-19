#include "stm32f10x.h"                  // Device header
#include "AD.h"

void JoyStick_Init(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	AD_Init(GPIO_Pin_3|GPIO_Pin_5);
}

uint8_t JoyStick_GetSW(void){
	if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1)){
		return 0;
	}
	return 1;
}

uint16_t JoyStick_GetX(void){
	return AD_GetValue(ADC_Channel_5);
}

uint16_t JoyStick_GetY(void){
	return AD_GetValue(ADC_Channel_3);
}
