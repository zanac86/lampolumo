#ifndef CHECKSIGNAL_H
#define CHECKSIGNAL_H

#include <stdint.h>

typedef struct
{

    // минимальный код в массиве
    uint16_t minCode;
    // максимальный код в массиве
    uint16_t maxCode;
    // среднее
    uint16_t average;
    // количество 0
    uint16_t zeros;
    // количество 4095
    uint16_t ovfls;
    // диапазон maxCode-minCode
    uint16_t range;
    // признак масштабирования по высоту экрана
    uint16_t normalized;
    // шаг прореживания
    uint16_t decimated;
    uint16_t compressed;
    uint16_t total_samples;
    uint16_t show_samples;

    // расчитанный период периодического сигнала
    uint16_t period;
    // частота периодического сигнала
    uint16_t freq;

    // Коэффициент пульсаций света
    uint32_t Emax;
    uint32_t Emin;
    uint32_t Eav;
    uint32_t kp1;
    uint32_t kp2;

} SignalInfo;

//void check_adc_data(SignalInfo *s, uint16_t *adc, uint16_t count);
//void normalize_simple(SignalInfo *s, uint16_t *adc, uint16_t count);
//void normalize_scale(SignalInfo *s, uint16_t *adc, uint16_t count, uint16_t k_bits);
//void normalize_for_display(SignalInfo *s, uint16_t *adc, uint16_t count);
//void decimate_for_display(SignalInfo *s, uint16_t *adc, uint16_t count);

void process_adc(SignalInfo* s, volatile uint16_t* adc, uint16_t count);

#endif

