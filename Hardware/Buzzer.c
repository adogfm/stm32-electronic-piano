#include "stm32f10x.h"
#include "Delay.h"

/* ============================================================
   ?????????:??? PWM.c
   - PA0 ?? PWM ??(8002 ????,??? TIM2_CH1)
   - PA7 ?? GPIO ??(??????????)
   ============================================================ */

static uint16_t current_freq = 0;
static uint8_t  current_duty = 0;

// ????
static void PWM_SetDuty(uint8_t duty_percent);
static void PWM_SetFreq(uint16_t freq);

// ?? PWM ???
static void PWM_Init_Audio(void)
{
    // ?? TIM2 ? GPIOA ??
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // PA0 ??? TIM2_CH1
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ???????:???? 71,?? 1MHz ????,ARR ?? 65535
    // ??????:1MHz / 65535 ˜ 15Hz,?? 1MHz(?????????)
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_TimeBaseStruct.TIM_Period = 65535;   // ???,????????
    TIM_TimeBaseStruct.TIM_Prescaler = 71;   // 72MHz / (71+1) = 1MHz
    TIM_TimeBaseStruct.TIM_ClockDivision = 0;
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStruct);

    // ??1 PWM1 ??,?????
    TIM_OCInitTypeDef TIM_OCStruct;
    TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCStruct.TIM_Pulse = 0;
    TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2, &TIM_OCStruct);

    // ?? TIM2
    TIM_Cmd(TIM2, ENABLE);
}

// ?? PWM ??
static void PWM_SetFreq(uint16_t freq)
{
    if (freq == 0) {
        PWM_SetDuty(0);
        return;
    }
    // ????? 1MHz,ARR = 1000000 / freq - 1
    uint32_t arr = 1000000UL / freq - 1;
    if (arr > 0xFFFF) arr = 0xFFFF;
    TIM2->ARR = (uint16_t)arr;
    // ??????????(????????)
    uint16_t duty_percent = current_duty;
    if (duty_percent > 100) duty_percent = 100;
    uint32_t pulse = (uint32_t)(TIM2->ARR + 1) * duty_percent / 100;
    TIM2->CCR1 = (uint16_t)pulse;
    TIM_GenerateEvent(TIM2, TIM_EventSource_Update);
}

// ?? PWM ???(??? 0~100)
static void PWM_SetDuty(uint8_t duty_percent)
{
    if (duty_percent > 100) duty_percent = 100;
    current_duty = duty_percent;
    uint32_t pulse = (uint32_t)(TIM2->ARR + 1) * duty_percent / 100;
    TIM2->CCR1 = (uint16_t)pulse;
}

// ??? GPIO ??(?????)
static void Buzzer_GPIO_Set(uint8_t on)
{
    if (on) {
        GPIO_ResetBits(GPIOA, GPIO_Pin_7);   // ????
    } else {
        GPIO_SetBits(GPIOA, GPIO_Pin_7);     // ????
    }
}

/* ==================== ???? ==================== */

void Buzzer_Init(void)
{
    // ??? PWM
    PWM_Init_Audio();
    // ??? PA7 ?????,?????(??)
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_SetBits(GPIOA, GPIO_Pin_7);
}

void Buzzer_OFF(uint8_t channel)
{
    // channel ??,???? PWM ????
    PWM_SetDuty(0);
    Buzzer_GPIO_Set(0);
}

void Buzzer_ON(uint16_t freq, uint8_t tone, uint8_t channel)
{
    if (freq == 0) {
        Buzzer_OFF(channel);
        return;
    }
    // channel ??,???? PWM ????
    PWM_SetFreq(freq);
    PWM_SetDuty(tone);
    Buzzer_GPIO_Set(1);
}

void Buzzer_Timing(uint16_t freq, uint16_t timing, uint8_t tone, uint8_t channel)
{
    Buzzer_ON(freq, tone, channel);
    Delay_ms(timing);
    Buzzer_OFF(channel);
}

void Buzzer_Reset(uint8_t channel)
{
    // ????:?? PWM ???????
    Buzzer_OFF(channel);
    Delay_ms(5);
}

void Buzzer_Drum(uint16_t type)
{
    // ??????(??? PWM ??)
    if (type != 0) Buzzer_Reset(2);
    switch(type) {
        case 0: Delay_ms(30); break;
        case 1: Buzzer_Timing(80, 30, 50, 2); break;
        case 2: for(int i=0;i<15;i++){ Buzzer_ON(600,50,2); Delay_ms(1); Buzzer_ON(1200,30,2); Delay_ms(1); } Buzzer_OFF(2); break;
        case 3: Buzzer_Timing(3000,30,10,2); break;
        case 4: Buzzer_ON(3000,10,2); Delay_ms(30); break;
        case 5: for(int i=500;i>470;i--){ Buzzer_ON(i,50,2); Delay_ms(1); } Buzzer_OFF(2); break;
        case 6: for(int i=400;i>370;i--){ Buzzer_ON(i,50,2); Delay_ms(1); } Buzzer_OFF(2); break;
        case 7: for(int i=300;i>270;i--){ Buzzer_ON(i,50,2); Delay_ms(1); } Buzzer_OFF(2); break;
        case 8: Buzzer_Timing(900,30,10,2); break;
        case 9: Buzzer_Timing(600,30,10,2); break;
        default: break;
    }
}