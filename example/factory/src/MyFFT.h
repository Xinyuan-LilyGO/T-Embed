#ifndef MY_FFT_H
#define MY_FFT_H

#include "Arduino.h"
// FFT大致了解
// https://www.pianshen.com/article/5965692032/
// https://zhuanlan.zhihu.com/p/39328336

// 程序参考
// https://arduino.nxez.com/2019/03/28/arduino-32-frequency-audio-spectrum-display.html
// https://blog.csdn.net/mc_li/article/details/81364766

// esp32 高速采样adc
// https://www.cnblogs.com/kerwincui/p/13751746.html
// https://github.com/atomic14/esp32_audio/tree/master/i2s_sampling

// 周期 = 1/Fs = ADC_SAMPLE_COUNT / ADC_SAMPLE_RATE
#define ADC_SAMPLE_COUNT 	(512)			//N 采样个数 必须2的N次方
#define ADC_SAMPLE_RATE 	(16*1000) 		//Fs 采样频率
#define ADC_CHANNEL_NUM     ADC1_CHANNEL_6 //只能是ADC1

#define PI2 6.28318530717959
#define _FREQ_TO_IDX(freq) ((freq)*ADC_SAMPLE_COUNT/ADC_SAMPLE_RATE)
#define FREQ_TO_IDX(freq) ((freq) > ADC_SAMPLE_RATE ? (ADC_SAMPLE_COUNT-1) : _FREQ_TO_IDX(freq))
#define IDX_TO_FREQ(idx)  ((idx)*ADC_SAMPLE_RATE/ADC_SAMPLE_COUNT)
#define fn(a,f,p,i) (a) * cos(PI2 * (f) * ((double)(i)/ADC_SAMPLE_RATE) + (p) * PI2 / 360) 

void FFT_Install();
void FFT_Calc();
uint16_t FFT_GetAmplitude(int i);
bool FFT_GetDataFlag();
void FFT_ClrDataFlag();
void FFT_Test();

#endif