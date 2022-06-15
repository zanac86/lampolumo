#ifndef CHECKSIGNAL_H
#define CHECKSIGNAL_H

#include <stdint.h>

typedef struct
{

	uint16_t minCode;
	uint16_t maxCode;
	uint16_t average;
	uint16_t zeros;
	uint16_t ovfls;
	uint16_t range;
	uint16_t normalized;
	uint16_t decimated;
	uint16_t total_samples;
	uint16_t show_samples;

	// environment average lightness at startup
	int32_t env_E;
	int32_t kp;

} SignalInfo;

//void check_adc_data(SignalInfo *s, uint16_t *adc, uint16_t count);
//void normalize_simple(SignalInfo *s, uint16_t *adc, uint16_t count);
//void normalize_scale(SignalInfo *s, uint16_t *adc, uint16_t count, uint16_t k_bits);
//void normalize_for_display(SignalInfo *s, uint16_t *adc, uint16_t count);
//void decimate_for_display(SignalInfo *s, uint16_t *adc, uint16_t count);


void process_adc(SignalInfo *s, volatile uint16_t *adc, uint16_t count);

#endif

