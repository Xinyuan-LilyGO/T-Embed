#include "MyFFT.h"
#include "arduinoFFT.h"
#include "ADCSampler.h"

double     vReal[ADC_SAMPLE_COUNT]; // 实部
double     vImag[ADC_SAMPLE_COUNT]; // 虚部

arduinoFFT FFT = arduinoFFT(vReal, vImag, ADC_SAMPLE_COUNT, ADC_SAMPLE_RATE);
ADCSampler g_adcSampler(ADC_UNIT_1, ADC_CHANNEL_NUM);

bool       g_fftDataIsOk = false;
bool       g_fftIsInstalled = false;

uint16_t   _FFT_GetAmplitude(int i)
{
    if (i == 0)
    {
        return (
            uint16_t)((sqrt(sq(vReal[0]) + sq(vImag[0])) / ADC_SAMPLE_COUNT));
    }
    else
    {
        return (uint16_t)(sqrt(sq(vReal[i]) + sq(vImag[i])) * 2 /
                          ADC_SAMPLE_COUNT);
    }
}

void FFT_Calc()
{
    // 计算FFT
    FFT.Windowing(FFT_WIN_TYP_RECTANGLE, FFT_FORWARD);
    FFT.Compute(FFT_FORWARD);

    // 因为具有对称性,只计算一半
    vReal[0] = _FFT_GetAmplitude(0);
    for (int i = 1; i < ADC_SAMPLE_COUNT / 2; i++)
    {
        vReal[i] = vReal[ADC_SAMPLE_COUNT - i] = _FFT_GetAmplitude(i);
    }
}

uint16_t FFT_GetAmplitude(int i) { return vReal[i]; }


//将ADC_SAMPLE_COUNT和ADC_SAMPLE_RATE改成一样数值,好查看数据
void     FFT_Test()
{
    // 构造测试信号
    for (int i = 0; i < ADC_SAMPLE_COUNT; i++)
    {
        double f1 = 50;                   //0Hz,幅值50
        double f2 = fn(1024, 50, 0, i);   //50Hz,幅值1024,相位0度
        double f3 = fn(3096, 100, 30, i); //100Hz,幅值3096,相位30度

        vReal[i] = f1 + f2 + f3;
        vImag[i] = 0;
    }

    // 计算FFT
    FFT.Windowing(FFT_WIN_TYP_RECTANGLE, FFT_FORWARD);
    FFT.Compute(FFT_FORWARD);

    // 计算幅值
    vReal[0] = sqrt(sq(vReal[0]) + sq(vImag[0])) / ADC_SAMPLE_COUNT;
    for (int i = 1; i < ADC_SAMPLE_COUNT / 2;
         i++) //因为数据具有对称性,所以计算一般就可以了
    {
        vReal[i] = sqrt(sq(vReal[i]) + sq(vImag[i])) * 2 / ADC_SAMPLE_COUNT;
    }

    int idx = 0;
    for (int i = 0; i < (ADC_SAMPLE_COUNT) >> 4; i++)
    {
        for (int j = 0; j < 1 << 4; j++)
        {
            Serial.printf("(%dHz)%.0f  ", IDX_TO_FREQ(idx), vReal[idx]);
            idx++;
        }
        Serial.println("");
    }
    Serial.println("");
    Serial.println("");
}

void FFT_adcWriterTask(void *param)
{
    I2SSampler      *sampler = (I2SSampler *)param;
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(100);
    while (true)
    {
        // wait for some samples to save
        uint32_t ulNotificationValue = ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        if (ulNotificationValue > 0)
        {
            if (g_fftDataIsOk == false)
            {
                int16_t *pData = sampler->getCapturedAudioBuffer();
                //Serial.println(millis());
                //Serial.println(*pData);

                for (int i = 0; i < ADC_SAMPLE_COUNT; i++, pData++)
                {
                    vReal[i] = *pData - 2048; //构造上下一半
                    vImag[i] = 0;
                }
                g_fftDataIsOk = true; // 数据准备完毕
            }
        }
    }
}

void FFT_Install()
{
    g_fftDataIsOk = false;

    if (g_fftIsInstalled == true) return;

    g_fftIsInstalled = true;

    i2s_config_t adcI2SConfig = {
        .mode =
            (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX ),
        .sample_rate = ADC_SAMPLE_RATE,                  // 设置采样率
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,    // 设置采样深度
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,     // 设置左右声道
        .communication_format = I2S_COMM_FORMAT_I2S, // 设置交流格式
        .intr_alloc_flags = 0,           // 设置用来分配中断
        .dma_buf_count = 2,              // 设置 DMA Buffer 计数
        .dma_buf_len = ADC_SAMPLE_COUNT, // 设置 DMA Buffer 长度
        .use_apll = false,               // 设置是否获得精确时钟
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0};

    // set up the adc sample writer task
    TaskHandle_t adcWriterTaskHandle;
    xTaskCreatePinnedToCore(FFT_adcWriterTask,
                            "ADC Writer Task",
                            4096,
                            &g_adcSampler,
                            0,
                            &adcWriterTaskHandle,
                            1);
    g_adcSampler.start(I2S_NUM_0,
                       adcI2SConfig,
                       ADC_SAMPLE_COUNT * 2,
                       adcWriterTaskHandle);
}

bool FFT_GetDataFlag() { return g_fftDataIsOk; }

void FFT_ClrDataFlag() { g_fftDataIsOk = false; }
