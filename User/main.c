/* ============================================================
 *  Piano v11 — 单 main 文件，结构干净
 *  4 模式: Live / Rec / Play / Demo
 *  触摸: TTP229 (PB6 SCL, PB7 SDO)
 *  声音: TIM2_CH1(PA0) PWM 音响 + PA7 低电平触发有源蜂鸣器
 *  修正：录音记录所有状态变化，播放以10ms推进，去除开机画面
 * ============================================================ */
#include "stm32f10x.h"
#include "Delay.h"
#include "LED.h"
#include "Key.h"
#include "Buzzer.h"
#include "OLED.h"
#include "Encoder.h"
#include "JoyStick.h"
#include "OLED_Font.h"
#include "TouchKey.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

/* ==================== 引脚定义 ==================== */
#define Key1 GPIO_Pin_10
#define Key2 GPIO_Pin_15
#define Key3 GPIO_Pin_14
#define Key4 GPIO_Pin_9
#define LED1 GPIO_Pin_6
#define LED2 GPIO_Pin_4
#define LED3 GPIO_Pin_2
#define LED1_PIN  GPIO_Pin_6
#define LED2_PIN  GPIO_Pin_4
#define LED3_PIN  GPIO_Pin_2
#define LED_PORT  GPIOA
#define BTN1_PIN  GPIO_Pin_5
#define BTN2_PIN  GPIO_Pin_8
#define BTN3_PIN  GPIO_Pin_10

#define MAX_RECORD_SIZE 16
#define BUZZER_PIN   GPIO_Pin_7
#define BUZZER_PORT  GPIOA

/* ==================== OLED 行地址 ==================== */
#define LINE0  0
#define LINE1  2
#define LINE2  4
#define LINE3  6

/* ==================== 模式 ==================== */
#define MODE_LIVE  0
#define MODE_REC   1
#define MODE_PB    2
#define MODE_DEMO  3
#define MODE_COUNT 4

/* ==================== 节拍/录音参数 ==================== */
#define MAX_RECORD  128
#define KEY_SCAN_MS 10
#define BEAT_PERIOD 15            /* 一拍 150ms @ KEY_SCAN_MS=10 */
#define BEAT_FLASH  5
#define DEMO_GAP    1             /* 音符间空 1 拍 */

#define LED_BEAT  0x01
#define LED_KEY   0x02
#define LED_MODE  0x04

/* ==================== 全局变量 ==================== */
uint8_t key_record[MAX_RECORD_SIZE];
uint8_t key_flag = 0;
uint16_t record_index = 0;
uint8_t is_recording = 0;

uint8_t  voice = 1;      // 音色编号（1~6）
uint8_t  knob  = 1;
uint8_t  tsp   = 5;      // 移调参数 0~11
uint8_t  sft   = 3;      // 键盘平移 0~6
uint8_t  oct   = 1;      // 八度：0低、1正常、2高
uint8_t  sharp = 0;      // 升半音：0关、1开
uint8_t  mode  = MODE_LIVE;
uint8_t  style = 1;
uint16_t tempo = 120;
uint8_t  chordType = 0;
uint8_t  isPlaying = 0;
uint8_t  track = 0;
uint8_t  section = 0;
uint8_t  arpVoice = 1;

/* 显示用，不影响频率 */
uint8_t  tone       = 0;
uint8_t  scale_var  = 0;

/* ----- C 大调频率表 (按键1~12 = C4~G5) ----- */
/* idx: 0=C4  1=D4  2=E4  3=F4  4=G4  5=A4  6=B4  7=C5  8=D5  9=E5  10=F5  11=G5  12=A5 */
uint16_t scale[] = {262, 294, 330, 349, 392, 440, 494, 523, 587, 659, 698, 784, 880};
float transpose[] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
/* 音色参数: tone 0~3 对应不同占空比 + 颤音 */
const uint8_t tone_duty[4]    = {25, 12, 50, 30};   /* Piano/Flute/Organ/Bell */
const uint8_t tone_vibrato[4] = {3,  6,  0,  4};    /* 颤音幅度 % */
uint8_t voiceDuties[5] = {25, 25, 25, 25, 25};      /* 兼容旧代码 */

char* voiceNames[6]   = {"Clarinet", "Pipe Organ", "Accordion", "Oboe", "Trumpet", "Drums"};
char* styleNames[6]   = {"8Beat", "16Beat", "Slow Rock", "Swing", "March", "Waltz"};
char* trackStatus[3]  = {"Dr&Ap", "Drums", "Arpgo"};
char* sectionStatus[4]= {"In&Ed", "Intro", "Endng", " OFF "};
char* chordNames[8]   = {"C ", "Dm", "Em", "E7", "F ", "G ", "G7", "Am"};
char* noteNames[7]    = {"DO", "RA", "MI", "FA", "SO", "LA", "XI"};
char* drumsNames[7]   = {"bass", "snare", "hihatC", "hihatO", "Htonggu", "Mtonggu", "Ltonggu"};

static const uint8_t next_mode[4] = {MODE_PB, MODE_DEMO, MODE_REC, MODE_LIVE};

/* ==================== 结构体 ==================== */
typedef struct { uint16_t keys; uint16_t dur; } RecNote;
typedef struct { uint8_t sum; uint8_t beats; } DemoNote;

RecNote recording[MAX_RECORD];
uint8_t  rec_len    = 0;
uint8_t  rec_idx    = 0;
uint8_t  rec_play   = 0;
uint16_t rec_timer  = 0;

uint16_t last_keys    = 0xFFFF;  // 初始化为不可能值，确保第一次记录
uint16_t cur_freq     = 0;
uint8_t  current_duty = 50;
uint8_t  last_rgb_sum = 0;

uint16_t beat_tick  = 0;
uint8_t  beat_flash = 0;
uint8_t  beat_flag  = 0;

uint8_t  demo_idx       = 0;
uint8_t  demo_beat_cnt  = 0;
uint8_t  demo_gap_cnt   = 0;
uint8_t  demo_pause     = 0;

uint8_t btn1_cnt = 0, btn2_cnt = 0, btn3_cnt = 0;
uint8_t btn1_deb = 0, btn2_deb = 0, btn3_deb = 0;
uint8_t btn1_last = 0, btn2_last = 0, btn3_last = 0;
uint8_t startup_guard = 20;

uint8_t last_key = 0;
uint8_t debounce_cnt = 0;

/* ==================== Demo 曲谱 ==================== */
const DemoNote demo_song[] = {
    {1,1},{1,1},{5,1},{5,1},{6,1},{6,1},{5,2},
    {4,1},{4,1},{3,1},{3,1},{2,1},{2,1},{1,2},
    {5,1},{5,1},{4,1},{4,1},{3,1},{3,1},{2,2},
    {5,1},{5,1},{4,1},{4,1},{3,1},{3,1},{2,2},
    {1,1},{1,1},{5,1},{5,1},{6,1},{6,1},{5,2},
    {4,1},{4,1},{3,1},{3,1},{2,1},{2,1},{1,2},
};
#define DEMO_LEN (sizeof(demo_song)/sizeof(demo_song[0]))

const char* mode_name[4]  = {"Live", "Rec", "Play", "Demo"};
const char* tone_name[4]  = {"Piano", "Flute", "Organ", "Bell"};
const char* scale_name[4] = {"Nrm", "Up", "Down", "Hi"};

/* ==================== 函数声明 ==================== */
void SystemClock_Config(void);
void My_GPIO_Init(void);
void LED_Show(uint8_t value);
void OLED_ShowChar_Simple(uint8_t x, uint8_t y, uint8_t chr);
void OLED_ShowString_Simple(uint8_t x, uint8_t y, char *str);
void OLED_ShowNum_Simple(uint8_t x, uint8_t y, uint32_t num);
uint16_t GetFreq(uint8_t sum);
void SoundNote(uint16_t freq, uint8_t duty);
void SoundStop(void);

/* 直接操作 TIM2 的辅助函数 */
static void PWM_SetFreq(uint16_t freq)
{
    if (freq == 0) { TIM2->CCR1 = 0; return; }
    uint32_t arr = 1000000UL / freq - 1;
    if (arr > 0xFFFF) arr = 0xFFFF;
    TIM2->ARR = (uint16_t)arr;
    /* 改频率后必须重算 CCR1,否则占空比错乱 */
    uint8_t duty = 25;
    if (tone < 4) duty = tone_duty[tone];
    uint32_t pulse = (uint32_t)(TIM2->ARR + 1) * duty / 100;
    TIM2->CCR1 = (uint16_t)pulse;
    TIM_GenerateEvent(TIM2, TIM_EventSource_Update);
}

static void PWM_SetDuty(uint8_t duty_percent)
{
    if (duty_percent > 100) duty_percent = 100;
    uint32_t pulse = (uint32_t)(TIM2->ARR + 1) * duty_percent / 100;
    TIM2->CCR1 = (uint16_t)pulse;
    TIM_GenerateEvent(TIM2, TIM_EventSource_Update);
}

/* PA7 蜂鸣器（低电平触发）*/
static void Buzzer_Set(uint8_t on)
{
    if (on) GPIO_ResetBits(GPIOA, GPIO_Pin_7);
    else    GPIO_SetBits(GPIOA, GPIO_Pin_7);
}

void StartRecording(void);
void StopRecording(void);
void RecordTick(uint16_t keys);
void StartPlayback(void);
void StopPlayback(void);
uint8_t PlaybackTick(void);
void DemoTick(void);
void ShowDemoScreen(uint8_t b1, uint8_t b2, uint8_t b3);
void ShowScreen(uint16_t keys, uint8_t sum, uint8_t b1, uint8_t b2, uint8_t b3);

/* 旧版辅助函数（保留） */
int chord(char number);
void playNote(uint8_t noteNum);
void stopNote(void);
void show_note(uint8_t note, uint8_t type);

/* ==================== 系统时钟 ==================== */
void SystemClock_Config(void) {
    RCC_HSEConfig(RCC_HSE_ON);
    uint32_t timeout = 10000;
    while (!RCC_WaitForHSEStartUp() && timeout > 0) timeout--;
    if (timeout == 0) {
        RCC_HSEConfig(RCC_HSE_OFF);
        RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div2);
        RCC_PCLK2Config(RCC_HCLK_Div1);
        return;
    }
    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
    RCC_PLLCmd(ENABLE);
    while (!RCC_GetFlagStatus(RCC_FLAG_PLLRDY));
    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
    while (RCC_GetSYSCLKSource() != 0x08);
    RCC_HCLKConfig(RCC_SYSCLK_Div1);
    RCC_PCLK1Config(RCC_HCLK_Div2);
    RCC_PCLK2Config(RCC_HCLK_Div1);
}

/* ==================== GPIO 初始化 ==================== */
void My_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* LED */
    GPIO_InitStruct.GPIO_Pin   = LED1_PIN | LED2_PIN | LED3_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_SetBits(GPIOA, LED1_PIN | LED2_PIN | LED3_PIN);

    /* 按钮上拉输入 */
    GPIO_InitStruct.GPIO_Pin   = BTN1_PIN | BTN2_PIN | BTN3_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 蜂鸣器 PA7 推挽（由 Buzzer 库管理） */
    GPIO_InitStruct.GPIO_Pin   = BUZZER_PIN;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUZZER_PORT, &GPIO_InitStruct);
    GPIO_SetBits(BUZZER_PORT, BUZZER_PIN);

    /* TTP229: PB6 输出(SCL), PB7 输入上拉(SDO) */
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    GPIO_SetBits(GPIOB, GPIO_Pin_6);

    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_7;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* ==================== 频率计算 (C大调 + scale_var八度切换) ==================== */
uint16_t GetFreq(uint8_t sum) {
    if (sum == 0 || sum > 13) return 0;
    int idx = sum - 1;               /* 1~12 直接映射 scale[0]~scale[11] */
    if (idx < 0 || idx >= 13) return 0;
    uint16_t f = scale[idx];
    /* scale_var: 0=Nrm 1=Up(+1oct) 2=Down(-1oct) 3=Hi(+2oct) */
    if (scale_var == 1)      f = f * 2;
    else if (scale_var == 2) f = f / 2;
    else if (scale_var == 3) f = f * 4;
    if (f < 20) f = 20;
    if (f > 20000) f = 20000;
    return f;
}

/* ==================== 发声/停止 (按 tone 切换音色) ====================
 * tone 0 (Piano): 25% 占空比, 颤音 ±3%
 * tone 1 (Flute): 12% 占空比, 颤音 ±6% (最柔和)
 * tone 2 (Organ): 50% 占空比, 无颤音 (丰满方波)
 * tone 3 (Bell):  30% 占空比, 颤音 ±4%, 频率×1.5 (清脆)
 */
static uint8_t vib_phase = 0;
void SoundNote(uint16_t freq, uint8_t duty) {
    if (freq == 0) { SoundStop(); return; }
    uint8_t t = tone;
    if (t > 3) t = 0;
    /* Bell 模式: 升五度(接近高八度) */
    if (t == 3 && freq < 1000) freq = freq * 3 / 2;
    /* 颤音: 周期性 ±vibrato% 微扰频率 */
    if (tone_vibrato[t] > 0) {
        vib_phase++;
        uint8_t v = tone_vibrato[t];
        int16_t offset = (vib_phase & 0x0F) < 8 ? v : -v;
        int32_t f2 = (int32_t)freq + (int32_t)(freq * offset / 100);
        if (f2 < 20) f2 = 20;
        freq = (uint16_t)f2;
    }
    /* 直接操作 TIM2 寄存器,不走 Buzzer_ON(避免 PA7 蜂鸣器响) */
    uint8_t real_duty = tone_duty[t];
    uint32_t arr = 1000000UL / freq - 1;
    if (arr > 0xFFFF) arr = 0xFFFF;
    TIM2->ARR = (uint16_t)arr;
    uint32_t pulse = (uint32_t)(arr + 1) * real_duty / 100;
    TIM2->CCR1 = (uint16_t)pulse;
    TIM_GenerateEvent(TIM2, TIM_EventSource_Update);
    /* Drums/Bell 模式: PA7 蜂鸣器短促敲击 */
    if (t == 3) {
        GPIO_ResetBits(GPIOA, GPIO_Pin_7);
        Delay_us(80);
        GPIO_SetBits(GPIOA, GPIO_Pin_7);
    }
}

void SoundStop(void) {
    TIM2->CCR1 = 0;       /* 占空比归零 = 静音 */
    GPIO_SetBits(GPIOA, GPIO_Pin_7);  /* PA7 蜂鸣器关 */
}

/* ==================== 录音/播放 (修正版) ==================== */
void StartRecording(void) {
    rec_len = 0;
    rec_idx = 0;
    rec_timer = 0;
    last_keys = 0xFFFF;   // 确保第一次记录
}

void RecordTick(uint16_t keys) {
    if (rec_len >= MAX_RECORD) {
        Buzzer_ON(800, 50, 2); Delay_ms(200); Buzzer_OFF(2);
        StopRecording();
        mode = MODE_LIVE;
        return;
    }
    // 首次记录
    if (rec_len == 0) {
        recording[0].keys = keys;
        recording[0].dur = 1;
        rec_len = 1;
        last_keys = keys;
        return;
    }
    // 状态与上次相同，延长持续时间
    if (keys == last_keys) {
        recording[rec_len - 1].dur++;
    } else {
        // 状态变化，新建条目
        recording[rec_len].keys = keys;
        recording[rec_len].dur = 1;
        rec_len++;
        last_keys = keys;
    }
}

void StopRecording(void) {
    if (rec_len > 0) {
        Buzzer_ON(1000, 50, 2); Delay_ms(60); Buzzer_OFF(2); Delay_ms(80);
        Buzzer_ON(1200, 50, 2); Delay_ms(60); Buzzer_OFF(2); Delay_ms(80);
        Buzzer_ON(1400, 50, 2); Delay_ms(60); Buzzer_OFF(2);
    }
}

void StartPlayback(void) {
    rec_idx = 0;
    rec_timer = 0;
    rec_play = 1;
}

void StopPlayback(void) {
    rec_play = 0;
    SoundStop();
}

/* 播放：每次主循环调用，以 10ms 为单位推进，保证速度与录制一致 */
uint8_t PlaybackTick(void) {
    if (!rec_play || rec_len == 0) {
        SoundStop();
        return 0;
    }
    RecNote n = recording[rec_idx];
    uint8_t s = 0;
    for (uint8_t i = 0; i < 16; i++) {
        if (n.keys & (1 << i)) { s = i + 1; break; }
    }
    uint16_t f = GetFreq(s);
    uint8_t na = 0;
    if (f != 0) {
        SoundNote(f, 25);
        na = 1;
    } else {
        SoundStop();
    }
    rec_timer++;
    if (rec_timer >= n.dur) {
        rec_timer = 0;
        rec_idx++;
        if (rec_idx >= rec_len) {
            StopPlayback();
            mode = MODE_LIVE;
        }
    }
    return na;
}

/* ==================== Demo ==================== */
void DemoTick(void) {
    if (!beat_flag) return;
    beat_flag = 0;
    demo_beat_cnt++;
    if (demo_beat_cnt >= demo_song[demo_idx].beats) {
        demo_beat_cnt = 0; demo_idx++;
        if (demo_idx >= DEMO_LEN) demo_idx = 0;
        demo_gap_cnt = DEMO_GAP;
    }
}

/* ==================== LED 显示 ==================== */
void LED_Show(uint8_t value) {
    GPIO_WriteBit(LED_PORT, LED1_PIN, (value & LED_BEAT) ? Bit_RESET : Bit_SET);
    GPIO_WriteBit(LED_PORT, LED2_PIN, (value & LED_KEY)  ? Bit_RESET : Bit_SET);
    GPIO_WriteBit(LED_PORT, LED3_PIN, (value & LED_MODE) ? Bit_RESET : Bit_SET);
}

/* ==================== OLED 简易显示 ==================== */
void OLED_ShowChar_Simple(uint8_t x, uint8_t y, uint8_t chr) {
    if (chr < 32 || chr > 126 || x >= 128 || y >= 7) return;
    const uint8_t *p = OLED_F8x16[chr - 32];
    for (uint8_t i = 0; i < 8; i++) {
        OLED_GRAM[y][x + i]     = p[i];
        OLED_GRAM[y + 1][x + i] = p[i + 8];
    }
}

void OLED_ShowString_Simple(uint8_t x, uint8_t y, char *str) {
    while (*str) {
        OLED_ShowChar_Simple(x, y, (uint8_t)(*str));
        x += 8; if (x > 120) { x = 0; y += 2; }
        str++;
    }
}

void OLED_ShowNum_Simple(uint8_t x, uint8_t y, uint32_t num) {
    char buf[12]; char *p = buf + 11; *p = '\0';
    if (num == 0) { *(--p) = '0'; }
    else { while (num > 0) { *(--p) = '0' + (num % 10); num /= 10; } }
    OLED_ShowString_Simple(x, y, p);
}

/* ==================== 屏幕显示 ==================== */
void ShowScreen(uint16_t keys, uint8_t sum, uint8_t b1, uint8_t b2, uint8_t b3) {
    for (uint8_t p = 0; p < 8; p++)
        for (uint16_t c = 0; c < 128; c++)
            OLED_GRAM[p][c] = 0;

    char line0[32];
    if (mode == MODE_DEMO && demo_pause) sprintf(line0, "DEMO|PAUSED|%s", scale_name[scale_var]);
    else sprintf(line0, "%s|%s|%s", mode_name[mode], tone_name[tone], scale_name[scale_var]);
    OLED_ShowString_Simple(0, LINE0, line0);

    if (mode == MODE_DEMO) { ShowDemoScreen(b1, b2, b3); return; }
    else if (mode == MODE_REC) {
        char line1[32]; sprintf(line1, "REC %d/%-3d", rec_len, MAX_RECORD);
        OLED_ShowString_Simple(0, LINE1, line1);
    } else if (mode == MODE_PB) {
        char line1[32];
        if (rec_play) sprintf(line1, "PLAY %d/%-3d", rec_idx + 1, rec_len);
        else sprintf(line1, "PLAY --/%-3d", rec_len);
        OLED_ShowString_Simple(0, LINE1, line1);
    } else {
        if (keys == 0) {
            OLED_ShowString_Simple(0, LINE1, "Keys: None");
        } else {
            OLED_ShowString_Simple(0, LINE1, "Keys:");
            uint8_t x = 40;
            for (uint8_t i = 0; i < 16; i++) {
                if (keys & (1 << i)) {
                    if (x > 112) break;
                    OLED_ShowChar_Simple(x, LINE1, 'K'); x += 8;
                    if (i + 1 >= 10) {
                        OLED_ShowChar_Simple(x, LINE1, '0' + (i+1)/10); x += 8;
                        OLED_ShowChar_Simple(x, LINE1, '0' + (i+1)%10); x += 8;
                    } else {
                        OLED_ShowChar_Simple(x, LINE1, '0' + (i+1)); x += 8;
                    }
                }
            }
        }
    }

    OLED_ShowString_Simple(0, LINE2, "Sum:"); OLED_ShowNum_Simple(32, LINE2, sum);
    if (cur_freq != 0) {
        OLED_ShowString_Simple(64, LINE2, "F:");
        OLED_ShowNum_Simple(80, LINE2, cur_freq);
        OLED_ShowChar_Simple(112, LINE2, '!');
    }
    char kmsg[20]; sprintf(kmsg, "K:%04X B:%d%d%d", keys, b1, b2, b3);
    OLED_ShowString_Simple(0, LINE3, kmsg);
}

void ShowDemoScreen(uint8_t b1, uint8_t b2, uint8_t b3) {
    char line[32];
    if (demo_pause) {
        sprintf(line, "Paused %d/%d", demo_idx + 1, DEMO_LEN);
        OLED_ShowString_Simple(0, LINE1, line);
        OLED_ShowString_Simple(0, LINE2, ">> PAUSED <<");
        OLED_ShowString_Simple(0, LINE3, "B1:Resume");
        return;
    }
    sprintf(line, "Demo %d/%d B:%d%d%d", demo_idx + 1, DEMO_LEN, b1, b2, b3);
    OLED_ShowString_Simple(0, LINE1, line);
    uint8_t pos = 0;
    line[pos++] = '>';
    uint8_t cnt = 0;
    for (uint8_t i = demo_idx; i < DEMO_LEN && cnt < 6; i++) {
        pos += sprintf(line + pos, "%d ", demo_song[i].sum);
        cnt++;
    }
    line[pos] = 0;
    OLED_ShowString_Simple(0, LINE2, line);
    pos = 0; cnt = 0;
    for (uint8_t i = demo_idx + 6; i < DEMO_LEN && cnt < 7; i++) {
        pos += sprintf(line + pos, "%d ", demo_song[i].sum);
        cnt++;
    }
    if (pos == 0) OLED_ShowString_Simple(0, LINE3, "End");
    else { line[pos] = 0; OLED_ShowString_Simple(0, LINE3, line); }
}

/* ==================== 旧版辅助函数 ==================== */
int chord(char number) {
    uint16_t freq = 0;
    if (number == 0) return 0;
    uint16_t major[] = {131, 165, 196, 262};
    uint16_t minor[] = {131, 156, 196, 262};
    uint16_t seventh[] = {131, 165, 196, 233};
    number = number - 1;
    switch (chordType) {
        case 0: freq = major[number % 4] * (uint16_t)pow(2, number / 4); break;
        case 1: freq = (uint16_t)(minor[number % 4] * pow(2, number / 4) * transpose[7]); break;
        case 2: freq = (uint16_t)(minor[number % 4] * pow(2, number / 4) * transpose[9]); break;
        case 3: freq = (uint16_t)(seventh[number % 4] * pow(2, number / 4) * transpose[9]); break;
        case 4: freq = (uint16_t)(major[number % 4] * pow(2, number / 4) * transpose[10]); break;
        case 5: freq = (uint16_t)(major[number % 4] * pow(2, number / 4) * transpose[0]); break;
        case 6: freq = (uint16_t)(seventh[number % 4] * pow(2, number / 4) * transpose[0]); break;
        case 7: freq = (uint16_t)(minor[number % 4] * pow(2, number / 4) * transpose[2]); break;
    }
    return (int)(freq * transpose[tsp]);
}

void playNote(uint8_t noteNum) {
    if (voice == 6) {
        Buzzer_Drum(noteNum);
        Delay_ms(70);
    } else {
        Buzzer_Reset(1);
        float freq = scale[noteNum - 1 + sft] * transpose[tsp] * pow(2, oct) * pow(1.06, sharp);
        Buzzer_ON((uint16_t)freq, voiceDuties[voice - 1], 1);
    }
}

void stopNote(void) { Buzzer_OFF(1); }

void show_note(uint8_t note, uint8_t type) {
    (void)note; (void)type;
}

/* ==================== 主函数 ==================== */
int main(void) {
    SystemClock_Config();
    My_GPIO_Init();
    TouchKey_Init();
    Buzzer_Init();          // 初始化 Buzzer 库（TIM2 + PA7）
    OLED_Init();
    OLED_Clear();

    /* 开机仅保留蜂鸣器测试，去掉所有屏幕文字显示 */
    Buzzer_ON(440, 50, 1);
    Delay_ms(300);
    Buzzer_OFF(1);
    Delay_ms(150);

    // 直接进入主循环，不显示任何开机提示
    for (uint8_t i = 0; i < 6; i++) {
        GPIO_WriteBit(GPIOA, LED1_PIN, (BitAction)(i % 2));
        Delay_ms(80);
    }

    while (1) {
        /* ---------- 触摸读取 + 去抖 ---------- */
        uint8_t raw = TouchKey_Scan();
        uint8_t valid_key = (raw >= 1 && raw <= 12) ? raw : 0;
        uint8_t sum_touch = 0;
        if (valid_key != 0 && valid_key == last_key) {
            debounce_cnt++;
            if (debounce_cnt >= 2) {
                sum_touch = valid_key;
            }
        } else {
            debounce_cnt = 0;
        }
        last_key = valid_key;
        uint16_t keys = (sum_touch > 0) ? (uint16_t)(1u << (sum_touch - 1)) : 0;

        /* ---------- 按钮处理 ---------- */
        uint8_t b1_raw = (GPIO_ReadInputDataBit(GPIOA, BTN1_PIN) == 0);
        uint8_t b2_raw = (GPIO_ReadInputDataBit(GPIOA, BTN2_PIN) == 0);
        uint8_t b3_raw = (GPIO_ReadInputDataBit(GPIOA, BTN3_PIN) == 0);

        if (startup_guard > 0) {
            startup_guard--;
            btn1_cnt = 0; btn2_cnt = 0; btn3_cnt = 0;
            btn1_deb = 0; btn2_deb = 0; btn3_deb = 0;
        } else {
            if (b1_raw) { if (btn1_cnt < 2) btn1_cnt++; } else { if (btn1_cnt > 0) btn1_cnt--; }
            if (b2_raw) { if (btn2_cnt < 2) btn2_cnt++; } else { if (btn2_cnt > 0) btn2_cnt--; }
            if (b3_raw) { if (btn3_cnt < 2) btn3_cnt++; } else { if (btn3_cnt > 0) btn3_cnt--; }

            btn1_deb = (btn1_cnt >= 2);
            btn2_deb = (btn2_cnt >= 2);
            btn3_deb = (btn3_cnt >= 2);

            if (btn1_deb && !btn1_last) {
                if (mode == MODE_DEMO) {
                    demo_pause = !demo_pause;
                    if (demo_pause) SoundStop();
                } else {
                    tone = (tone + 1) % 4;
                    if (mode == MODE_REC) StopRecording();
                }
            }
            btn1_last = btn1_deb;

            if (btn2_deb && !btn2_last) {
                scale_var = (scale_var + 1) % 4;
                if (mode == MODE_REC) StopRecording();
            }
            btn2_last = btn2_deb;

            if (btn3_deb && !btn3_last) {
                if (mode == MODE_REC)  StopRecording();
                if (mode == MODE_PB)   StopPlayback();
                if (mode == MODE_DEMO) {
                    SoundStop();
                    demo_idx = 0; demo_beat_cnt = 0; demo_gap_cnt = 0; demo_pause = 0;
                }
                mode = next_mode[mode];
                if (mode == MODE_PB && rec_len > 0) StartPlayback();
                if (mode == MODE_REC) StartRecording();
                if (mode == MODE_DEMO) {
                    demo_idx = 0; demo_beat_cnt = 0; demo_gap_cnt = 0; demo_pause = 0;
                    beat_tick = 0; beat_flag = 0;
                }
            }
            btn3_last = btn3_deb;
        }

        /* ---------- 模式处理 ---------- */
        uint8_t note_active = 0;
        uint8_t rgb_sum = 0;
        cur_freq = 0;

        if (mode == MODE_DEMO) {
            if (demo_pause) {
                SoundStop();
                note_active = 0; rgb_sum = 0;
            } else {
                DemoTick();
                if (demo_idx < DEMO_LEN) {
                    if (demo_gap_cnt > 0) {
                        demo_gap_cnt--; SoundStop();
                        note_active = 0; rgb_sum = 0;
                    } else {
                        rgb_sum = demo_song[demo_idx].sum;
                        cur_freq = GetFreq(rgb_sum);
                        if (cur_freq != 0) {
                            SoundNote(cur_freq, 25);
                            note_active = 1;
                        }
                    }
                }
            }
        } else {
            /* LIVE / REC / PB */
            if (keys != 0) cur_freq = GetFreq(sum_touch);
            rgb_sum = sum_touch;

            if (mode == MODE_LIVE) {
                if (cur_freq != 0) {
                    SoundNote(cur_freq, 25);
                    note_active = 1;
                } else {
                    SoundStop();
                }
            } else if (mode == MODE_REC) {
                if (cur_freq != 0) {
                    SoundNote(cur_freq, 25);
                    note_active = 1;
                } else {
                    SoundStop();
                }
                RecordTick(keys);   // 每次循环都记录，不再依赖 beat_flag
            } else if (mode == MODE_PB) {
                note_active = PlaybackTick();  // 每次循环推进，不再依赖 beat_flag
            }
        }

        /* ---------- LED 指示 ---------- */
        uint8_t led = 0;
        if (note_active)         led |= LED_KEY;
        if (mode != MODE_LIVE)   led |= LED_MODE;
        LED_Show(led);

        /* ---------- OLED 显示 ---------- */
        ShowScreen(keys, sum_touch, b1_raw, b2_raw, b3_raw);
        OLED_Refresh();

        /* ---------- 节拍定时器（仅用于 Demo 和旧逻辑，录音播放已独立） ---------- */
        beat_tick++;
        if (beat_tick >= BEAT_PERIOD) {
            beat_tick = 0;
            beat_flag = 1;
        }

        Delay_ms(KEY_SCAN_MS);
    }
}

/* ==================== 备用空 ISR ==================== */
void EXTI3_IRQHandler(void)       { EXTI_ClearITPendingBit(EXTI_Line3); }
void EXTI4_IRQHandler(void)       { EXTI_ClearITPendingBit(EXTI_Line4); }
void EXTI9_5_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line8) == SET) EXTI_ClearITPendingBit(EXTI_Line8);
    if (EXTI_GetITStatus(EXTI_Line9) == SET) EXTI_ClearITPendingBit(EXTI_Line9);
}
void EXTI15_10_IRQHandler(void) {
    if (EXTI_GetITStatus(EXTI_Line10) == SET) EXTI_ClearITPendingBit(EXTI_Line10);
    if (EXTI_GetITStatus(EXTI_Line11) == SET) EXTI_ClearITPendingBit(EXTI_Line11);
    if (EXTI_GetITStatus(EXTI_Line12) == SET) EXTI_ClearITPendingBit(EXTI_Line12);
    if (EXTI_GetITStatus(EXTI_Line13) == SET) EXTI_ClearITPendingBit(EXTI_Line13);
    if (EXTI_GetITStatus(EXTI_Line14) == SET) EXTI_ClearITPendingBit(EXTI_Line14);
    if (EXTI_GetITStatus(EXTI_Line15) == SET) EXTI_ClearITPendingBit(EXTI_Line15);
}