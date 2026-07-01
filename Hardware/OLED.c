#include "stm32f10x.h"
#include "Delay.h"
#include "OLED_Font.h"

// ========== 显存定义 ==========
uint8_t OLED_GRAM[8][128];

// ========== 引脚定义 ==========
#define OLED_CS    GPIO_Pin_11
#define OLED_DC    GPIO_Pin_12
#define OLED_RES   GPIO_Pin_14
#define OLED_SCK   GPIO_Pin_13
#define OLED_MOSI  GPIO_Pin_15
#define OLED_PORT  GPIOB

// ========== 底层宏（与条纹代码完全一致） ==========
#define OLED_CS_L()   GPIO_ResetBits(OLED_PORT, OLED_CS)
#define OLED_CS_H()   GPIO_SetBits(OLED_PORT, OLED_CS)
#define OLED_DC_L()   GPIO_ResetBits(OLED_PORT, OLED_DC)
#define OLED_DC_H()   GPIO_SetBits(OLED_PORT, OLED_DC)
#define OLED_RES_L()  GPIO_ResetBits(OLED_PORT, OLED_RES)
#define OLED_RES_H()  GPIO_SetBits(OLED_PORT, OLED_RES)
#define OLED_SCK_L()  GPIO_ResetBits(OLED_PORT, OLED_SCK)
#define OLED_SCK_H()  GPIO_SetBits(OLED_PORT, OLED_SCK)
#define OLED_MOSI_L() GPIO_ResetBits(OLED_PORT, OLED_MOSI)
#define OLED_MOSI_H() GPIO_SetBits(OLED_PORT, OLED_MOSI)

// ========== SPI 发送字节（与条纹代码完全一致） ==========
static void OLED_SPI_SendByte(uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        OLED_SCK_L();
        Delay_us(1);
        if (byte & 0x80) OLED_MOSI_H();
        else             OLED_MOSI_L();
        byte <<= 1;
        OLED_SCK_H();
        Delay_us(1);
    }
    OLED_SCK_L();
}

// ========== 写命令/数据（与条纹代码完全一致） ==========
void OLED_WR_Byte(uint8_t dat, uint8_t cmd) {
    if (cmd) OLED_DC_H();   // 数据
    else     OLED_DC_L();   // 命令
    OLED_CS_L();
    OLED_SPI_SendByte(dat);
    OLED_CS_H();
}

// ========== 初始化（与条纹代码完全一致） ==========
void OLED_Init(void) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = OLED_CS | OLED_DC | OLED_RES | OLED_SCK | OLED_MOSI;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(OLED_PORT, &GPIO_InitStructure);
    
    OLED_RES_L();
    Delay_ms(20);
    OLED_RES_H();
    Delay_ms(20);
    
    OLED_WR_Byte(0xAE, 0);
    OLED_WR_Byte(0xD5, 0); OLED_WR_Byte(0x80, 0);
    OLED_WR_Byte(0xA8, 0); OLED_WR_Byte(0x3F, 0);
    OLED_WR_Byte(0xD3, 0); OLED_WR_Byte(0x00, 0);
    OLED_WR_Byte(0x40, 0);
    OLED_WR_Byte(0x8D, 0); OLED_WR_Byte(0x14, 0);
    OLED_WR_Byte(0x20, 0); OLED_WR_Byte(0x02, 0);
    // 不加 0x21/0x22
    OLED_WR_Byte(0xA1, 0);
    OLED_WR_Byte(0xC8, 0);
    OLED_WR_Byte(0xDA, 0); OLED_WR_Byte(0x12, 0);
    OLED_WR_Byte(0x81, 0); OLED_WR_Byte(0x7F, 0);
    OLED_WR_Byte(0xD9, 0); OLED_WR_Byte(0xF1, 0);
    OLED_WR_Byte(0xDB, 0); OLED_WR_Byte(0x40, 0);
    OLED_WR_Byte(0xA4, 0);
    OLED_WR_Byte(0xA6, 0);
    OLED_WR_Byte(0xAF, 0);
}

// ========== 刷新显存到屏幕（直接使用裸机写屏方式） ==========
void OLED_Refresh(void) {
    for (uint8_t page = 0; page < 8; page++) {
        OLED_WR_Byte(0xB0 + page, 0);
        OLED_WR_Byte(0x00, 0);
        OLED_WR_Byte(0x10, 0);
        for (uint16_t col = 0; col < 128; col++) {
            OLED_WR_Byte(OLED_GRAM[page][col], 1);  // 数据
        }
    }
}

// ========== 清屏（清显存并刷新） ==========
void OLED_Clear(void) {
    for (uint8_t page = 0; page < 8; page++) {
        for (uint16_t col = 0; col < 128; col++) {
            OLED_GRAM[page][col] = 0x00;
        }
    }
    OLED_Refresh();
}

// ========== 画点（操作显存） ==========
void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t t) {
    if (x >= 128 || y >= 64) return;
    uint8_t page = y / 8;
    uint8_t bit = y % 8;
    if (t)  OLED_GRAM[page][x] |=  (1 << bit);
    else    OLED_GRAM[page][x] &= ~(1 << bit);
}

// ========== 显示字符（8x16） ==========
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t chr, uint8_t size1, uint8_t mode) {
    if (size1 != 8) return;  // 仅支持 8x16
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t temp = OLED_F8x16[chr - ' '][i];
        for (uint8_t j = 0; j < 8; j++) {
            if (temp & 0x80) OLED_DrawPoint(x + j, y + i, 1);
            else if (mode == 0) OLED_DrawPoint(x + j, y + i, 0);
            temp <<= 1;
        }
    }
}

// ========== 显示字符串 ==========
void OLED_ShowString(uint8_t x, uint8_t y, uint8_t *chr, uint8_t size1, uint8_t mode) {
    while (*chr) {
        OLED_ShowChar(x, y, *chr, size1, mode);
        x += size1 / 2;
        chr++;
    }
}

// ========== 数字显示（简单） ==========
static uint32_t OLED_Pow(uint32_t m, uint32_t n) {
    uint32_t result = 1;
    while (n--) result *= m;
    return result;
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode) {
    for (uint8_t i = 0; i < len; i++) {
        uint8_t digit = (num / OLED_Pow(10, len - i - 1)) % 10;
        OLED_ShowChar(x + (size1/2) * i, y, '0' + digit, size1, mode);
    }
}

// ========== 其他函数占位 ==========
void OLED_WR_BP(uint8_t x, uint8_t y) {}
void OLED_On(void) {}
void OLED_ColorTurn(uint8_t i) {}
void OLED_DisplayTurn(uint8_t i) {}
void OLED_ShowSignedNum(uint8_t x, uint8_t y, int32_t num, uint8_t len, uint8_t size1, uint8_t mode) {}
void OLED_ShowHexNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode) {}
void OLED_ShowBinNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size1, uint8_t mode) {}
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t prec, uint8_t size1, uint8_t mode) {}
void OLED_ShowChinese(uint8_t x, uint8_t y, uint8_t num, uint8_t size1, uint8_t mode) {}
void OLED_ScrollDisplay(uint8_t num, uint8_t space, uint8_t mode) {}
void OLED_ShowPicture(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t BMP[], uint8_t mode) {}
void OLED_ShowWatch(void) {}