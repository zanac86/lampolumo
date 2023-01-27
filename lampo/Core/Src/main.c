/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "checksignal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

/* USER CODE BEGIN PV */

SignalInfo signalInfo;

uint16_t adc_count = 0;
volatile uint16_t adc_stopped = 0;
volatile uint16_t new_measure_do = 0;

volatile uint16_t tick1 = 0;

uint16_t decimates[] =
{ 1, 2, 4, 8, 16, 32, 64, 128 };

const uint16_t decimate_max_index = 8;
uint16_t decimate_index = 0;

// 128*32
#define MAX_SAMPLES_COUNT 4096
//#define MAX_SAMPLES_COUNT 3200
//#define MAX_SAMPLES_COUNT 2048
// 3200

// 5120

// Отсчеты АЦП 12 бит 0..4095
// 0-3.3 В

#pragma pack(push, 2)
volatile uint16_t samples[MAX_SAMPLES_COUNT] =
{ 0, };
#pragma pack(pop)

volatile uint16_t sample_index = 0;

volatile uint16_t button_pressed = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	// обработчик завершения преобразования ацп
	if (adc_stopped)
	{
		// сейчас пауза на обработку записанного сигнала
		return;
	}

	uint16_t a = HAL_ADC_GetValue(&hadc1);
	if (sample_index > 0)
	{
		// фильтрация=сглаживание
		// (предыдущий+текущий)/2 -> в новый отсчет
		a += samples[sample_index - 1];
		samples[sample_index] = a >> 1;
	}
	else
	{
		samples[sample_index] = a;
	}

	sample_index++;

	if (sample_index == MAX_SAMPLES_COUNT)
	{
		sample_index = 0;
		adc_stopped = 1;
	}
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim->Instance == htim2.Instance)
	{
		// Таймер 2 настроен на 1 сек
		tick1++;
		new_measure_do = 1;
	}
	if (htim->Instance == htim3.Instance)
	{
		// для таймера 3 настроено
		// делители 144-1, 50-1  - 10000 кГц - 100.0 мкс
		// делители 225-1, 100-1 -  3200 кГц - 312.5 мкс
		// -1 в настройках таймера - считает с 0 до N-1 и выдает прерывание
		// и запустим новое преобразование ацп
		HAL_ADC_Start_IT(&hadc1);
	}
}

void init_my_devices()
{
	HAL_Delay(100);
	OLED_Init(&hi2c1);
	OLED_FontSet(Font_MSX_6x8_rus1251);
	OLED_SetContrast(4);
	OLED_Clear(0);
	OLED_DrawStr("STM32F103C8T6", 0, 0, 1);
	OLED_DrawStr(__DATE__, 0, 8, 1);
	OLED_DrawStr(__TIME__, 0, 16, 1);
	char str[20];
	sprintf(str, "S=%ld", HAL_RCC_GetSysClockFreq());
	OLED_DrawStr(str, 0, 24, 1);
	sprintf(str, "H=%ld", HAL_RCC_GetHCLKFreq());
	OLED_DrawStr(str, 64, 24, 1);
	sprintf(str, "1=%ld", HAL_RCC_GetPCLK1Freq());
	OLED_DrawStr(str, 0, 32, 1);
	sprintf(str, "2=%ld", HAL_RCC_GetPCLK2Freq());
	OLED_DrawStr(str, 64, 32, 1);
	OLED_UpdateScreen();
	HAL_Delay(800);
	OLED_Clear(0);
	OLED_UpdateScreen();
}

void print_val(uint8_t line, uint32_t v)
{
	char str[15];
	sprintf(str, "%06lu", v);

	switch (line)
	{
	case 0:
		str[0] = 'A';
		break;
	case 1:
		str[0] = 'X';
		break;
	case 2:
		str[0] = 'N';
		break;
	case 3:
		str[0] = 0x98;
		break;
	case 4:
		str[0] = 'F';
		break;
	case 5:
		str[0] = 'D';
		break;
	case 6:
		str[0] = 'K';
		break;
	case 7:
		str[0] = 'k';
		break;
	}

	OLED_DrawStr(str, OLED_WIDTH - 6 * 6, line * 8, 1);
}

void draw_button_state()
{
	char str[10];
	// если compressed - то узкий символ прямоугольник
	str[0] = button_pressed ? 0x86 : 0x9b;
	str[1] = 0;
	OLED_DrawStr(str, 0, 0, 1);
}

void print_signal(SignalInfo *s)
{
	print_val(0, s->average);
	print_val(1, s->maxCode);
	print_val(2, s->minCode);
	print_val(3, s->range);
	print_val(4, s->freq);
	print_val(5, s->decimated);
	print_val(6, s->kp1);
	print_val(7, s->kp2);

}

void print_ticks()
{
	char str[15];
	sprintf(str, "%06u", tick1);
	OLED_DrawStr(str, 0, 56, 1);
}

void draw_waveform()
{
//	decimate_index = (tick1 / 2) % 6;
	signalInfo.decimated = decimates[decimate_index];
	process_adc(&signalInfo, samples, MAX_SAMPLES_COUNT);
	OLED_Clear(0);
	for (uint16_t i = 0; i < signalInfo.show_samples; i++)
	{
		OLED_DrawPixel(i, OLED_HEIGHT - samples[i] - 1);
	}

	print_signal(&signalInfo);
	print_ticks();
	OLED_UpdateScreen();
}

void print_val_zoom(char c, uint32_t v, uint8_t x, uint8_t y, uint8_t zoom)
{
	char str[15];
	sprintf(str, "%4lu", v);
	str[0] = c;

	OLED_DrawStrZoom(str, x, y, 1, zoom);
}

void draw_big_numbers()
{
	signalInfo.decimated = decimates[decimate_index];
	process_adc(&signalInfo, samples, MAX_SAMPLES_COUNT);
	OLED_Clear(0);

	char str[15];
	sprintf(str, "Kp %3u %%", signalInfo.kp1);
	OLED_DrawStrZoom(str, 4, 3, 1, 2);

	sprintf(str, "Fr %4u Hz", signalInfo.freq);
	OLED_DrawStrZoom(str, 4, 24, 1, 2);

	OLED_DrawRectangle(0, 0, 127, 21);
	OLED_DrawRectangle(0, 21, 127, 42);
	OLED_DrawRectangle(0, 42, 127, 63);

	sprintf(str, "%4u Min", signalInfo.minCode);
	OLED_DrawStrZoom(str, 4, 45, 1, 1);
	sprintf(str, "%4u Max", signalInfo.maxCode);
	OLED_DrawStrZoom(str, 4, 54, 1, 1);

	sprintf(str, "%4u Av", signalInfo.average);
	OLED_DrawStrZoom(str, 70, 45, 1, 1);
	sprintf(str, "%4u \x98", signalInfo.range);
	OLED_DrawStrZoom(str, 70, 54, 1, 1);

	OLED_UpdateScreen();
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_I2C1_Init();
	MX_ADC1_Init();
	MX_TIM3_Init();
	MX_TIM2_Init();
	/* USER CODE BEGIN 2 */

	init_my_devices();

	HAL_ADCEx_Calibration_Start(&hadc1);

	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_Base_Start_IT(&htim3);

	HAL_Delay(100);

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
//		HAL_Delay(1);
		if (new_measure_do == 1)
		{
			new_measure_do = 0;
		}
		if (adc_stopped)
		{
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
			if (decimate_index == (decimate_max_index - 1))
			{
				draw_big_numbers();
			}
			else
			{
				draw_waveform();
			}
			sample_index = 0;
			adc_stopped = 0;
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
		}

		if (button_pressed)
		{
			draw_button_state();
			OLED_UpdateScreen();
			button_pressed = 0;
			decimate_index = (decimate_index + 1) % decimate_max_index;
		}

		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct =
	{ 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct =
	{ 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit =
	{ 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV8;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void)
{

	/* USER CODE BEGIN ADC1_Init 0 */

	/* USER CODE END ADC1_Init 0 */

	ADC_ChannelConfTypeDef sConfig =
	{ 0 };

	/* USER CODE BEGIN ADC1_Init 1 */

	/* USER CODE END ADC1_Init 1 */

	/** Common config
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	/** Configure Regular Channel
	 */
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_41CYCLES_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN ADC1_Init 2 */

	/* USER CODE END ADC1_Init 2 */

}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 400000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief TIM2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM2_Init(void)
{

	/* USER CODE BEGIN TIM2_Init 0 */

	/* USER CODE END TIM2_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig =
	{ 0 };
	TIM_MasterConfigTypeDef sMasterConfig =
	{ 0 };

	/* USER CODE BEGIN TIM2_Init 1 */

	/* USER CODE END TIM2_Init 1 */
	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 7200 - 1;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 10000 - 1;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM2_Init 2 */

	/* USER CODE END TIM2_Init 2 */

}

/**
 * @brief TIM3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM3_Init(void)
{

	/* USER CODE BEGIN TIM3_Init 0 */

	/* USER CODE END TIM3_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig =
	{ 0 };
	TIM_MasterConfigTypeDef sMasterConfig =
	{ 0 };

	/* USER CODE BEGIN TIM3_Init 1 */

	/* USER CODE END TIM3_Init 1 */
	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 144 - 1;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 100 - 1;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
	{
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
	/* USER CODE BEGIN TIM3_Init 2 */

	/* USER CODE END TIM3_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct =
	{ 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

	/*Configure GPIO pin : PC13 */
	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : PB15 */
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == GPIO_PIN_15)
	{
		button_pressed = 1;
	}
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
