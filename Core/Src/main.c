// Header
#include "main.h"

// Variabel privat
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

// Status kecepatan LED dan variabel terkait debouncing
uint8_t led1_speed = 0;
uint8_t led2_speed = 0;
uint32_t last_press_time_pb0 = 0;
uint32_t last_press_time_pb1 = 0;
const uint32_t debounce_delay = 50; // Delay debouncing dalam milidetik
const uint32_t led1_periods[] = {199, 499, 999};  // cepat → sedang → lambat
const uint32_t led2_periods[] = {999, 1499, 1999};  // lambat → lebih lambat → sangat lambat

// Prototipe fungsi privat
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);

// Memperbarui periode pada timer tertentu
void update_timer_period(TIM_HandleTypeDef *htim, uint32_t period);

// Memperbarui periode timer secara aman: stop, inisialisasi ulang, lalu mulai lagi
void update_timer_period(TIM_HandleTypeDef *htim, uint32_t period)
{
  __HAL_TIM_DISABLE(htim);
  htim->Init.Period = period;
  HAL_TIM_Base_Init(htim);
  __HAL_TIM_SET_COUNTER(htim, 0);
  __HAL_TIM_ENABLE(htim);
}

// Callback saat timer overflow: toggle LED sesuai timer
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM2)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
  }
  else if(htim->Instance == TIM3)
  {
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_6);
  }
}

// Callback interrupt eksternal: memperbarui kecepatan LED berdasarkan tombol yang ditekan
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  uint32_t now = HAL_GetTick();

  if(GPIO_Pin == GPIO_PIN_0 && (now - last_press_time_pb0 > debounce_delay))
  {
    last_press_time_pb0 = now;
    led1_speed = (led1_speed + 1) % 3;
    update_timer_period(&htim2, led1_periods[led1_speed]);
  }
  else if(GPIO_Pin == GPIO_PIN_1 && (now - last_press_time_pb1 > debounce_delay))
  {
    last_press_time_pb1 = now;
    led2_speed = (led2_speed + 1) % 3;
    update_timer_period(&htim3, led2_periods[led2_speed]);
  }
}

// Titik masuk utama aplikasi
int main(void)
{
  HAL_Init(); // Reset semua peripheral dan inisialisasi sistem
  SystemClock_Config(); // Konfigurasi clock sistem
  MX_GPIO_Init(); // Inisialisasi GPIO
  MX_TIM2_Init(); // Inisialisasi Timer 2
  MX_TIM3_Init(); // Inisialisasi Timer 3

  // Mulai kedua timer dengan kecepatan awal
  update_timer_period(&htim2, led1_periods[led1_speed]);
  update_timer_period(&htim3, led2_periods[led2_speed]);

  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start_IT(&htim3);

  while (1)
  {
    // Loop utama kosong; semua fungsi berbasis interrupt
  }
}

// Konfigurasi clock sistem
void SystemClock_Config(void)
{
  // Konfigurasi standar clock
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

// Inisialisasi Timer 2
static void MX_TIM2_Init(void)
{
  // Timer 2 dikonfigurasi dengan prescaler dan periode default
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8399;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
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
}

// Inisialisasi Timer 3
static void MX_TIM3_Init(void)
{
  // Timer 3 dikonfigurasi dengan prescaler dan periode default
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 8399;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 999;
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
}

// Inisialisasi GPIO
static void MX_GPIO_Init(void)
{
  // Aktifkan clock GPIO dan konfigurasi pin untuk LED dan tombol
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_RESET);

  // Konfigurasi pin PC13 sebagai output
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  // Konfigurasi pin PA0 dan PA1 sebagai input dengan interrupt naik (rising edge)
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Konfigurasi pin PA5 dan PA6 sebagai output untuk LED
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // Inisialisasi interrupt EXTI
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

// Penanganan kesalahan: nonaktifkan interrupt dan hentikan sistem
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
// Penanganan error assert (untuk debugging)
void assert_failed(uint8_t *file, uint32_t line)
{
  // Cetak informasi error jika diperlukan
}
#endif
