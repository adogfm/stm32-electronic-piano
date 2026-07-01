#include "TouchKey.h"
#include "Delay.h"

#define TTP229_SCL_PIN   GPIO_Pin_6
#define TTP229_SCL_PORT  GPIOB
#define TTP229_SDO_PIN   GPIO_Pin_7
#define TTP229_SDO_PORT  GPIOB

void TouchKey_Init(void) {
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = TTP229_SCL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(TTP229_SCL_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = TTP229_SDO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(TTP229_SDO_PORT, &GPIO_InitStructure);

    GPIO_SetBits(TTP229_SCL_PORT, TTP229_SCL_PIN);
}

uint8_t TouchKey_Scan(void) {
    uint16_t key_data = 0;
    uint8_t i;

    // 起始脉冲：高→低2.6ms→高
    GPIO_SetBits(TTP229_SCL_PORT, TTP229_SCL_PIN);
    Delay_us(10);
    GPIO_ResetBits(TTP229_SCL_PORT, TTP229_SCL_PIN);
    Delay_us(2600);
    GPIO_SetBits(TTP229_SCL_PORT, TTP229_SCL_PIN);
    Delay_us(10);

    // 读16位（低电平有效）
    for (i = 0; i < 16; i++) {
        GPIO_ResetBits(TTP229_SCL_PORT, TTP229_SCL_PIN);
        Delay_us(10);
        if (GPIO_ReadInputDataBit(TTP229_SDO_PORT, TTP229_SDO_PIN) == Bit_RESET) {
            key_data |= (1 << i);
        }
        GPIO_SetBits(TTP229_SCL_PORT, TTP229_SCL_PIN);
        Delay_us(10);
    }

    // 结束脉冲
    GPIO_ResetBits(TTP229_SCL_PORT, TTP229_SCL_PIN);
    Delay_us(2600);
    GPIO_SetBits(TTP229_SCL_PORT, TTP229_SCL_PIN);

    // 只扫描前12个键（0~11）
    for (i = 0; i < 12; i++) {
        if (key_data & (1 << i)) {
            return i + 1;
        }
    }
    return 0;
}