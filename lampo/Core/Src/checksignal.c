#include "checksignal.h"

void check_adc_data(SignalInfo *s, volatile uint16_t *adc, uint16_t count)
{
	if (count == 0)
	{
		return;
	}
	s->zeros = 0;
	s->ovfls = 0;
	s->minCode = adc[0];
	s->maxCode = adc[0];
	s->average = 0;
	for (uint16_t i = 0; i < count; i++)
	{
		if (adc[i] == 0)
		{
			s->zeros++;
		}
		if (adc[i] == 4095)
		{
			s->ovfls++;
		}
		if (adc[i] > s->maxCode)
		{
			s->maxCode = adc[i];
		}
		if (adc[i] < s->minCode)
		{
			s->minCode = adc[i];
		}
	}
	s->range = s->maxCode - s->minCode;
	s->normalized = 0;
	s->decimated = 0;
	s->total_samples = count;
	s->show_samples = 0;
}

void normalize_simple(SignalInfo *s, volatile uint16_t *adc, uint16_t count)
{
	for (uint16_t i = 0; i < count; i++)
	{
		adc[i] = adc[i] - s->minCode;
	}
}

void normalize_scale(SignalInfo *s, volatile uint16_t *adc, uint16_t count,
		uint16_t k_bits)
{
	for (uint16_t i = 0; i < count; i++)
	{
		adc[i] = (adc[i] - s->minCode) >> k_bits;
	}
}

void normalize_for_display(SignalInfo *s, volatile uint16_t *adc,
		uint16_t count)
{
	uint16_t DISP_H = 64;
	if (s->range < DISP_H)
	{
		normalize_simple(s, adc, count);
	}
	else
	{

		if (s->range < 256)
		{
			normalize_scale(s, adc, count, 2);
		}
		else
		{
			if (s->range < 1024)
			{
				normalize_scale(s, adc, count, 4);
			}
			else
			{
				normalize_scale(s, adc, count, 6);
			}
		}
	}
	s->normalized = 1;
}

void decimate_for_display(SignalInfo *s, volatile uint16_t *adc, uint16_t count)
{
	uint16_t DISP_W = 128;
	uint16_t step = count / DISP_W;
	uint16_t index_adc = 0;
	uint16_t index_dec = 0;
	for (uint16_t i = 0; i < DISP_W; i++)
	{
		adc[index_dec] = adc[index_adc];
		index_dec++;
		index_adc += step;
		if (index_adc >= count)
			break;
	}
	s->total_samples = index_dec;
	for (uint16_t i = index_dec; i < DISP_W; i++)
	{
		adc[index_dec] = 0;
	}
	s->decimated = 1;
}

void process_adc(SignalInfo *s, volatile uint16_t *adc, uint16_t count)
{
	check_adc_data(s, adc, count);
	normalize_for_display(s, adc, count);
	decimate_for_display(s, adc, count);
}

