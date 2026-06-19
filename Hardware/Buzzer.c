#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "PWM.h"

void Buzzer_Init(void){
	PWM_Init_1();
	PWM_Init_2();
}

/**
  * @brief  蜂鸣器关闭函数
  * @param  channel：要关闭的通道编号，旋律通道选1，伴奏通道选2，同时关闭选0
  * @retval 无
  */
void Buzzer_OFF(uint8_t channel){
	if(channel == 0){       // 通道1&2
		PWM_SetDuty_1(0);
		PWM_SetDuty_2(0);
	}
	else if(channel == 1){  // 通道1
		PWM_SetDuty_1(0);
	}
	else if(channel == 2){  // 通道2
		PWM_SetDuty_2(0);
	}
}

/**
  * @brief  蜂鸣器发声函数，调用后会持续发声直到关闭蜂鸣器
  * @param  freq：发声的频率
  * @param  tone：发声的音色，用占空比表示
  * @param  channel：要发声的通道编号，旋律通道选1，伴奏通道选2，同时发声选0
  * @retval 无
  */
void Buzzer_ON(uint16_t freq, uint8_t tone, uint8_t channel){
	if(freq != 0){
		if(channel == 0){       // 通道1&2
			PWM_SetFreq_1(freq);
			PWM_SetFreq_2(freq);
			PWM_SetDuty_1(tone);
			PWM_SetDuty_2(tone);
		}
		else if(channel == 1){  // 通道1
			PWM_SetFreq_1(freq);
			PWM_SetDuty_1(tone);
		}
		else if(channel == 2){  // 通道2
			PWM_SetFreq_2(freq);
			PWM_SetDuty_2(tone);
		}
	}
	else{
		Buzzer_OFF(channel);
	}
}

/**
  * @brief  蜂鸣器发声函数，可设置发声的时长
  * @param  freq：发声的频率
  * @param  timing：发声的时长
  * @param  tone：发声的音色，用占空比表示
  * @param  channel：要发声的通道编号，旋律通道选1，伴奏通道选2，同时发声选0
  * @retval 无
  */
void Buzzer_Timing(uint16_t freq, uint16_t timing, uint8_t tone, uint8_t channel){
	Buzzer_ON(freq, tone, channel);
	Delay_ms(timing);
	Buzzer_OFF(channel);
}

/**
  * @brief  蜂鸣器重置函数，可在发声前调用使音头更平滑
  * @param  channel：要重置的通道编号，旋律通道选1，伴奏通道选2，同时重置选0
  * @retval 无
  */
void Buzzer_Reset(uint8_t channel){
	if(channel == 0){       // 通道1&2
		PWM_SetFreq_1(1);
		PWM_SetFreq_2(1);
		TIM_SetCounter(TIM2, 51);
		TIM_SetCounter(TIM3, 51);
	}
	else if(channel == 1){  // 通道1
		PWM_SetFreq_1(1);
		TIM_SetCounter(TIM2, 51);
	}
	else if(channel == 2){  // 通道2
		PWM_SetFreq_2(1);
		TIM_SetCounter(TIM3, 51);
	}
}

/**
  * @brief  蜂鸣器打击乐函数
  * @param  type：打击乐器的类型，0表示休止符
  * @retval 无
  */
void Buzzer_Drum(uint16_t type){
		if(type != 0){
			Buzzer_Reset(2);
		}
	  switch(type){
    case 0:  // 休止符
			Delay_ms(30);
			break;
    case 1:  // 低音鼓
			Buzzer_Timing(80,30,50,2);
			break;
    case 2:  // 小军鼓
			for(int i=0;i<15;i++){
				Buzzer_ON(600,50,2);
				Delay_ms(1);
				Buzzer_ON(1200,30,2);
				Delay_ms(1);
			}
			Buzzer_OFF(2);
			break;
    case 3:  // 踩镲（闭合）
			Buzzer_Timing(3000,30,10,2);
			break;
    case 4:  // 踩镲（打开）
			Buzzer_ON(3000,10,2);
			Delay_ms(30);
			break;
    case 5:  // 通鼓（高）
			for(int i=500;i>470;i--){
				Buzzer_ON(i,50,2);
				Delay_ms(1);
			}
			Buzzer_OFF(2);
			break;
    case 6:  // 通鼓（中）
			for(int i=400;i>370;i--){
				Buzzer_ON(i,50,2);
				Delay_ms(1);
			}
			Buzzer_OFF(2);
			break;
    case 7:  // 通鼓（低）
			for(int i=300;i>270;i--){
				Buzzer_ON(i,50,2);
				Delay_ms(1);
			}
			Buzzer_OFF(2);
			break;
    case 8:  // Agogo（高）
			Buzzer_Timing(900,30,10,2);
			break;
    case 9:  // Agogo（低）
			Buzzer_Timing(600,30,10,2);
			break;
  }
}
