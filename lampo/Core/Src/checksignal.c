#include "checksignal.h"

void check_adc_data(SignalInfo *s, volatile uint16_t *adc, uint16_t count)
{
	if (count == 0)
	{
		return;
	}
	s->period=0;
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
	sum = ((sum * 1000) / count);
	s->average = (uint16_t) (sum / 1000);

/*
#define GAP_MIN 50
#define GAP_MAX (4095-GAP_MIN)
	if (s->minCode>GAP_MIN)
	{
		s->minCode-=GAP_MIN;
	}
	if (s->maxCode<GAP_MAX)
	{
		s->maxCode+=GAP_MIN;
	}
*/
	s->range = s->maxCode - s->minCode;
	s->normalized = 0;

//	s->decimated = 0;

	s->total_samples = count;
	s->show_samples = 0;

}

void normalize_simple(SignalInfo *s, volatile uint16_t *adc)
{
	for (uint16_t i = 0; i < s->show_samples; i++)
	{
		adc[i] = adc[i] - s->minCode;
	}
}

void normalize_scale(SignalInfo *s, volatile uint16_t *adc, uint16_t k_bits)
{
	for (uint16_t i = 0; i < s->show_samples; i++)
	{
		adc[i] = (adc[i] - s->minCode) >> k_bits;
	}
}

void normalize_real(SignalInfo *s, volatile uint16_t *adc)
{
	for (uint16_t i = 0; i < s->show_samples; i++)
	{
		uint32_t tmp = (adc[i] - s->minCode);
		tmp = (tmp * 64) << 10;
		tmp = (tmp / (s->range + 1));
		tmp = (tmp >> 10);
		adc[i] = (uint16_t) tmp;
	}

}

void normalize_for_display(SignalInfo *s, volatile uint16_t *adc)
{
	normalize_real(s, adc);
	s->normalized = 1;
	return;

	uint16_t DISP_H = 64;
	if (s->range < DISP_H)
	{
		normalize_simple(s, adc);
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
	uint16_t DISP_W = 128;
	uint16_t count = s->total_samples;
	if (count <= DISP_W)
	{
		s->show_samples = s->total_samples;
		s->decimated = 1;
		return;
	}
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

	for (uint16_t i = index_dec; i < DISP_W; i++)
	{
		adc[index_dec] = 0;
	}
	s->show_samples = index_dec;
	s->decimated = 1;
	s->compressed=1;
}

void decimate_for_display_real_first(SignalInfo *s, volatile uint16_t *adc)
{
	uint16_t DISP_W = 128;
	uint16_t count = s->total_samples;
	s->compressed=0;
	if (count <= DISP_W)
	{
		s->show_samples = s->total_samples;
		s->decimated = 1;
	}else
	{
		s->show_samples = DISP_W;
		s->decimated = 1;
	}
}

void calc_env_e(SignalInfo *s, volatile uint16_t *adc, uint16_t count)
{
	// посчитаем сумму энергии
	//
	s->env_E=0;
	int32_t env_E_sum = 0;
	for(uint16_t i=0;i<s->total_samples;i++)
	{
		env_E_sum+=adc[i];
	}
	s->env_E=env_E_sum*16;
	s->env_E=s->env_E/s->total_samples;
	s->env_E=s->env_E/16;
}

void calc_period_max(SignalInfo *s, volatile uint16_t *adc)
{
	s->period=0;
	uint32_t periods=0;
	uint16_t prev_index_max=0;
	uint16_t prev_index_min=0;
	uint32_t t_sum=0;

	uint16_t min_index=0;
	uint16_t max_index=0;
	int16_t min_val=adc[0];
	int16_t max_val=adc[0];
	int16_t delta=4;
	uint16_t look_for_max=1;

	for(uint16_t i=0;i<s->total_samples;i++)
	{
		int16_t a=adc[i];
		if (a>max_val)
		{
			max_index=i;
			max_val=a;
		}
		if (a<min_val)
		{
			min_index=i;
			min_val=a;
		}

		if (look_for_max)
		{
			if (a<(max_val-delta))
			{
				if (prev_index_max>0)
				{
					t_sum+=(max_index-prev_index_max);
					periods++;
				}
				prev_index_max=max_index;
				min_index=i;
				min_val=a;
				look_for_max=0;
			}
		}else
		{
			if (a>(min_val+delta))
			{
				if (prev_index_min>0)
				{
					t_sum+=(min_index-prev_index_min);
					periods++;
				}
				prev_index_min=min_index;
				max_index=i;
				max_val=a;
				look_for_max=1;

			}
		}
	}

	s->period=(uint16_t)(((t_sum*4096)/periods)/4096);
	uint32_t tmp=((10000*1024)/((uint32_t)(s->period)))/1024;
	s->freq=tmp;

	// период в семплах
	// freq_hz=1/(period*0.0001)

//	s->period=periods;


//
//    mni, mn = 0, signal[1][0]
//    mxi, mx = 0, signal[1][0]
//    look_for_max = True
//    maxtab = []
//    mintab = []
//    for i in range(len(signal[0])):
//        a = signal[1][i]
//        if a > mx:
//            mxi, mx = i, a
//        if a < mn:
//            mni, mn = i, a
//
//        if look_for_max:
//            if a < (mx-delta):
//                maxtab.append(mxi)
//                mni, mn = i, a
//                look_for_max = False
//        else:
//            if a > (mn+delta):
//                mintab.append(mni)
//                mxi, mx = i, a
//                look_for_max = True
//    return mintab, maxtab
//
//

}

void calc_gost_pulsation(SignalInfo *s, volatile uint16_t *adc)
{
	s->kp=0;
	int32_t sum_E = 0;
	for(uint16_t i=0;i<s->total_samples;i++)
	{
	      sum_E += adc[i] - s->env_E;

	}

//	int32_t Emax = 0;
//	int32_t Emin = 0;
//
//	int32_t Emax_sum = 0;
//	int32_t Emin_sum = 0;
//	int32_t sum_E_sum = 0;

}

void process_adc(SignalInfo *s, volatile uint16_t *adc, uint16_t count)
{
	check_adc_data(s, adc, count); // set total_samples here
	calc_period_max(s, adc);
	if (s->decimated)
	{
		decimate_for_display_real_first(s, adc); // set show_samples_here
	}else
	{
		decimate_for_display(s, adc); // set show_samples_here
	}
	normalize_for_display(s, adc);

	// need skip first time or N for calculate store env_E like calibration
	// before real calculation of lightness pulsation
}
