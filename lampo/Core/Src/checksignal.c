#include "checksignal.h"
#include "OLED.h"

void check_adc_data(SignalInfo *s, volatile uint16_t *adc, uint16_t count)
{
	if (count == 0)
	{
		return;
	}
	s->total_samples = count;
	s->show_samples = 0;
	s->normalized = 0;
	s->period = 0;
	s->zeros = 0;
	s->ovfls = 0;
	s->minCode = adc[0];
	s->maxCode = adc[0];
	s->average = 0;
	uint32_t sum = 0;
	for (uint16_t i = 0; i < count; i++)
	{
		sum += adc[i];
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
	sum = ((sum << 7) / (uint32_t) count);
	s->average = (uint16_t) (sum >> 7);

	s->range = s->maxCode - s->minCode;

}

void normalize_simple(SignalInfo *s, volatile uint16_t *adc, uint16_t max_value)
{
	uint16_t d = (max_value - s->range) / 2;
	for (uint16_t i = 0; i < s->show_samples; i++)
	{
		adc[i] = adc[i] - s->minCode + d;
	}
}

void normalize_scale(SignalInfo *s, volatile uint16_t *adc, uint16_t k_bits)
{
	for (uint16_t i = 0; i < s->show_samples; i++)
	{
		adc[i] = (adc[i] - s->minCode) >> k_bits;
	}
}

void normalize_real_range(SignalInfo *s, volatile uint16_t *adc)
{
	for (uint16_t i = 0; i < s->show_samples; i++)
	{
		uint32_t tmp = (adc[i] - s->minCode);
		tmp = (tmp * OLED_HEIGHT) << 8;
		tmp = (tmp / (s->range + 1));
		tmp = (tmp >> 8);
		adc[i] = (uint16_t) tmp;
	}

}

void normalize_real_full(SignalInfo *s, volatile uint16_t *adc)
{
	uint16_t range = s->maxCode;
	for (uint16_t i = 0; i < s->show_samples; i++)
	{
		uint32_t tmp = (adc[i]);
		tmp = (tmp * OLED_HEIGHT) << 8;
		tmp = (tmp / (range + 1));
		tmp = (tmp >> 8);
		adc[i] = (uint16_t) tmp;
	}

}

void normalize_for_display(SignalInfo *s, volatile uint16_t *adc)
{
	/*
	 <64            как есть
	 64... 256      /4
	 256...1024     /16
	 >1024          /64
	 */

	normalize_real_full(s, adc);
	s->normalized = 1;
	return;

	// если влезает по высоте, то центрируем
	if (s->range < OLED_HEIGHT)
	{
		normalize_simple(s, adc, OLED_HEIGHT);
	}
	else
	{
		// по честному масштабируем в экран по высоте
		normalize_real_full(s, adc);
	}
	s->normalized = 1;

	return;

	//////////////////////////////////////////////////////

	if (s->range < OLED_HEIGHT)
	{
		normalize_simple(s, adc, OLED_HEIGHT);
	}
	else
	{
		if (s->range < 256)
		{
			normalize_scale(s, adc, 2);
		}
		else
		{
			if (s->range < 1024)
			{
				normalize_scale(s, adc, 4);
			}
			else
			{
				normalize_scale(s, adc, 6);
			}
		}
	}
	s->normalized = 1;
}

void decimate_for_display(SignalInfo *s, volatile uint16_t *adc)
{
	if (s->total_samples <= OLED_WIDTH)
	{
		s->show_samples = s->total_samples;
		s->compressed = 0;
		return;
	}
	uint16_t step = s->decimated;
	uint16_t index_adc = 0;
	uint16_t index_dec = 0;
	uint16_t count = s->total_samples;
	for (uint16_t i = 0; i < OLED_WIDTH; i++)
	{
		adc[index_dec] = adc[index_adc];
		index_dec++;
		index_adc += step;
		if (index_adc >= count)
		{
			break;
		}
	}

	for (uint16_t i = index_dec; i < OLED_WIDTH; i++)
	{
		adc[index_dec] = 0;
	}
	s->show_samples = index_dec;
	s->compressed = 1;
}

void decimate_for_display_real_first(SignalInfo *s, volatile uint16_t *adc)
{
	s->compressed = 0;
	if (s->total_samples <= OLED_WIDTH)
	{
		s->show_samples = s->total_samples;
	}
	else
	{
		s->show_samples = OLED_WIDTH;
	}
}

void calc_period_max(SignalInfo *s, volatile uint16_t *adc)
{
	// поиск в сигнале локальных максимумов и минимумов по кривой
	// в t_sum собираю дельты между максимумами и минимумами
	// потом вычислю период и частоту
	s->period = 0;
	uint32_t periods_count = 0;
	uint16_t prev_index_max = 0;
	uint16_t prev_index_min = 0;
	uint32_t t_sum = 0;

	uint16_t min_index = 0;
	uint16_t max_index = 0;
	int16_t min_val = adc[0];
	int16_t max_val = adc[0];
	int16_t delta = 2;
	uint16_t look_for_max = 1;

	uint32_t e_min = 0;
	uint32_t e_max = 0;
	uint16_t n_min = 0;
	uint16_t n_max = 0;

	for (uint16_t i = 0; i < s->total_samples; i++)
	{
		int16_t a = adc[i];
		if (a > max_val)
		{
			max_index = i;
			max_val = a;
		}
		if (a < min_val)
		{
			min_index = i;
			min_val = a;
		}

		if (look_for_max)
		{
			if (a < (max_val - delta))
			{
				if (prev_index_max > 0)
				{
					t_sum += (max_index - prev_index_max);
					periods_count++;
				}
				prev_index_max = max_index;
				min_index = i;
				min_val = a;
				look_for_max = 0;
				e_max += adc[max_index];
				n_max++;
			}
		}
		else
		{
			if (a > (min_val + delta))
			{
				if (prev_index_min > 0)
				{
					t_sum += (min_index - prev_index_min);
					periods_count++;
				}
				prev_index_min = min_index;
				max_index = i;
				max_val = a;
				look_for_max = 1;
				e_min += adc[min_index];
				n_min++;

			}
		}
	}

	s->period = 0;
	s->freq = 0;

	if (periods_count > 0)
	{
		// в t_sum попали интервалы между соседними максимуми и соседними минимуми
//		s->period = (uint16_t) (((t_sum * 4096) / periods_count) / 4096);
//		uint32_t tmp = ((5000 * 1024) / ((uint32_t) (s->period))) / 1024;
		s->period = (uint16_t) (t_sum / periods_count);
		s->freq = (uint32_t) ((5000 * periods_count) / (t_sum));
	}

	s->Emin = 0;
	s->Emax = 0;
	s->Eav = 0;
	if ((n_min > 0) && (n_max > 0))
	{
		s->Emin = ((e_min << 6) / n_min) >> 6;
		s->Emax = ((e_max << 6) / n_max) >> 6;
	}

	if ((s->average > 0) && ((s->Emax + s->Emin) > 0))
	{
		s->Eav = s->average;
		s->kp1 = (((s->Emax - s->Emin) * 1000) / (s->Emax + s->Emin)) / 10;	//
		s->kp2 = (((s->Emax - s->Emin) * 1000) / (2 * s->Eav)) / 10;
	}
}

void process_adc(SignalInfo *s, volatile uint16_t *adc, uint16_t count)
{
	check_adc_data(s, adc, count); // set total_samples here
	calc_period_max(s, adc);
	if (s->decimated == 0)
	{
		// для рисования по ширине экрана отсчет в пиксель
		// рисовать только первые 128 отсчетов
		decimate_for_display_real_first(s, adc);
	}
	else
	{
		// честно прореживаем для количества на экране
		decimate_for_display(s, adc);
	}

	normalize_for_display(s, adc);

	// need skip first time or N for calculate store env_E like calibration
	// before real calculation of lightness pulsation
}
