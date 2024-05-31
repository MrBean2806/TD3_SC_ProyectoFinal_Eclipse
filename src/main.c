/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention

///////////////////////////////////////////////////////////////
///     Proyecto Final Tecnicas III - Sistemas de Control   ///
///     UTN - FRC 2022                                      ///
///     Agustin Bean - Manuel Arias                         ///
///////////////////////////////////////////////////////////////

  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
union dato{
	int dato_int;
    float dato_float;
    char dato_char[4];
};

TIM_HandleTypeDef htim2;	//Base de tiempos para encoder
TIM_HandleTypeDef htim3;	//Base de tiempos para muestreo
TIM_HandleTypeDef htim4;	//Generacion de PWM

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */
  union dato vel_medida;
  union dato vel_referencia;
  union dato vel_error;
  union dato duty;
  uint16_t maxDuty;

  uint32_t contador_pasado = 0, contador_actual = 0, tiempoPulsos = 0;
  uint32_t contadorVelocidad = 0, velocidadActual = 0, velocidadPasada = 0;

  uint8_t TxFlag = RESET;
  uint32_t inicioTx = 0;			//instante en el que comenzó el ensayo
  uint32_t duracionMedicion = 1000;	//[ms] duracion del envio de datos una vez recibo el escalon
  uint8_t variableAGraficar = 0;
  uint8_t msg[14] = {0};
  uint8_t estado = CONFIG;

  float Kp, Ki, Kt;
  float entrada_PI = 0, salida_PI = 0, H = 0, H_1 = 0, I = 0, P = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM3_Init(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_TIM3_Init();

  //Arranco los timers
  HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);
  HAL_TIM_OC_Start_IT(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);

  maxDuty = htim4.Instance->ARR;
  htim4.Instance->CCR1 = 0;
  vel_referencia.dato_int = -1;
  vel_medida.dato_int = 0;

  contador_pasado = __HAL_TIM_GET_COUNTER(&htim2);

  while (1){
	//el bucle principal se ejecuta cada 1ms (TIM3)
	  if(TxFlag){
		//Cargo los parametros de la medicion (duracion, variable a graficar, kp, ki)
		  if(estado == CONFIG){
			  vel_medida.dato_int = 0;
			  vel_referencia.dato_int = -1;
			  char temp[4] = {0};
			  union dato duracion, kp, ki, variable, inicio;
			  //espero el comando para iniciar la secuencia de configuracion
			  do{
				  HAL_UART_Receive(&huart1, temp, 4, HAL_MAX_DELAY);
			  }while( strncmp(temp, "init", 4) != 0 );

			  HAL_UART_Receive(&huart1, temp, 4, HAL_MAX_DELAY);
			  memcpy(duracion.dato_char, temp, 4);
			  duracionMedicion = duracion.dato_int;

			  HAL_UART_Receive(&huart1, temp, 4, HAL_MAX_DELAY);
			  memcpy(kp.dato_char, temp, 4);
			  Kp = kp.dato_float;

			  HAL_UART_Receive(&huart1, temp, 4, HAL_MAX_DELAY);
			  memcpy(ki.dato_char, temp, 4);
			  Ki = ki.dato_float;
			  Kt = Ki*0.0005;

			  HAL_UART_Receive(&huart1, temp, 4, HAL_MAX_DELAY);
			  memcpy(variable.dato_char, temp, 4);
			  variableAGraficar = variable.dato_int;

			  estado = ESPERA;
		  }else if(estado == ESPERA){	
			  if(vel_referencia.dato_int >= 0){
				  inicioTx = HAL_GetTick();
				  estado = MEDICION;
			  }
		  }else if(estado == MEDICION){		//mientras no llegue la señal de finalizacion (vel_ref = -1) y no haya expirado el tiempo, continua la medicion
			  if((vel_referencia.dato_int != -1) && (HAL_GetTick() - inicioTx <= duracionMedicion)){
				  if(variableAGraficar == 1)
					sprintf(msg, "%d %d\n", vel_referencia.dato_int, vel_medida.dato_int);
				  else
					sprintf(msg, "%d %d\n", vel_referencia.dato_int, duty.dato_int);
				  HAL_UART_Transmit_DMA(&huart1, msg, strlen(msg));

				  //Si la velocidad no se actualiza por 10 ms, la fuerzo a 0
				  velocidadActual = vel_medida.dato_int;
				  if(velocidadActual == velocidadPasada){
					  contadorVelocidad ++;
				  }else{
					  contadorVelocidad = 0;
					  velocidadPasada = velocidadActual;
				  }
				  if(contadorVelocidad > 10){
					  vel_medida.dato_int = 0;
					  contadorVelocidad = 0;
				  }

			  }else{
				//Llego la señal de finalizacion, freno el motor y envio confimacion a la PC
				  htim4.Instance->CCR1 = 0;
				  HAL_UART_Transmit_DMA(&huart1, "-1", strlen("-1"));
				  estado = CONFIG;
			  }
		  }

		  HAL_UART_Receive_DMA(&huart1, (uint8_t *) vel_referencia.dato_char, 4);
		  TxFlag = RESET;

  	  }
  }
}



void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim){
//Base de tiempos de 1KHz
	if(htim->Instance == TIM3){
		TxFlag = SET;
		if(estado == MEDICION){

			vel_error.dato_float = vel_referencia.dato_int - vel_medida.dato_int;
			entrada_PI = vel_error.dato_float;
			 //Bloque P:
			P = entrada_PI * Kp;
			//Anti-windup -> el acumulador (integrador del PI) solo funcionará cuando el error de velocidad sea menor a 400rpm
			if(vel_error.dato_float < 400)
				H = entrada_PI + H_1;
			else{
				H = 0;
				H_1 = 0;
			}
			//Bloque I:
			I = Kt * (H + H_1);
			//Sumo la parte proporcional e integral para obtener el PI completo
			salida_PI = P + I;
			H_1 = H;

			//limitador
			if(salida_PI < 0)
				salida_PI = 0;
			if(salida_PI > 8)
				salida_PI = 8;

			//conversión tension de control [0-8V] -> duty [0-3200]
			duty.dato_int = salida_PI * maxDuty / 8;
			htim4.Instance->CCR1 = duty.dato_int;

			}else if(estado == CONFIG){
				entrada_PI = salida_PI = H = H_1 = I = P = 0;
			}

	}
}

//Interrupcion para contar pulsos del encoder
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == EXTI_0_Pin){
		if(estado == MEDICION){
			contador_actual = __HAL_TIM_GET_COUNTER(&htim2);
			if(contador_actual >= contador_pasado){
				tiempoPulsos = contador_actual - contador_pasado;
			}else{
				tiempoPulsos = (4294967295 - contador_pasado) + contador_actual;
			}

			vel_medida.dato_int = (1 * 60 * 64000000) / (tiempoPulsos * 32);
			contador_pasado = __HAL_TIM_GET_COUNTER(&htim2);
		}
	}
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
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
  if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
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

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 63999;
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
  if (HAL_TIM_OC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_TIMING;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_OC_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 3199;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, D_OUT6_Pin|D_OUT7_Pin|D_OUT8_Pin|D_OUT2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, D_OUT1_Pin|LED_Pin|D_OUT3_Pin|D_OUT5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : EXTI_0_Pin */
  GPIO_InitStruct.Pin = EXTI_0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(EXTI_0_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : D_OUT6_Pin D_OUT7_Pin D_OUT8_Pin D_OUT2_Pin */
  GPIO_InitStruct.Pin = D_OUT6_Pin|D_OUT7_Pin|D_OUT8_Pin|D_OUT2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : D_OUT1_Pin LED_Pin D_OUT3_Pin D_OUT5_Pin */
  GPIO_InitStruct.Pin = D_OUT1_Pin|LED_Pin|D_OUT3_Pin|D_OUT5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : D_IN8_Pin D_IN7_Pin D_IN6_Pin D_IN5_Pin
                           D_IN4_Pin */
  GPIO_InitStruct.Pin = D_IN8_Pin|D_IN7_Pin|D_IN6_Pin|D_IN5_Pin
                          |D_IN4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : D_IN3_Pin D_IN2_Pin */
  GPIO_InitStruct.Pin = D_IN3_Pin|D_IN2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : D_IN1_Pin */
  GPIO_InitStruct.Pin = D_IN1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(D_IN1_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}

/* USER CODE BEGIN 4 */

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
