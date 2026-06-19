#include "stm32f10x.h"                  // Device header

void LED_Init_A(uint16_t pins){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = pins;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOA, pins);
}

void LED_Init_B(uint16_t pins){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = pins;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, pins);
}

void LED_ON_A(uint16_t pins){
	GPIO_ResetBits(GPIOA, pins);
}

void LED_OFF_A(uint16_t pins){
	GPIO_SetBits(GPIOA, pins);
}

void LED_ON_B(uint16_t pins){
	GPIO_ResetBits(GPIOB, pins);
}

void LED_OFF_B(uint16_t pins){
	GPIO_SetBits(GPIOB, pins);
}
