/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f3xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define EXTI_0_Pin GPIO_PIN_0
#define EXTI_0_GPIO_Port GPIOA
#define EXTI_0_EXTI_IRQn EXTI0_IRQn
#define D_OUT6_Pin GPIO_PIN_3
#define D_OUT6_GPIO_Port GPIOA
#define D_OUT7_Pin GPIO_PIN_4
#define D_OUT7_GPIO_Port GPIOA
#define D_OUT8_Pin GPIO_PIN_6
#define D_OUT8_GPIO_Port GPIOA
#define D_OUT2_Pin GPIO_PIN_7
#define D_OUT2_GPIO_Port GPIOA
#define D_OUT1_Pin GPIO_PIN_0
#define D_OUT1_GPIO_Port GPIOB
#define D_IN8_Pin GPIO_PIN_1
#define D_IN8_GPIO_Port GPIOB
#define D_IN7_Pin GPIO_PIN_10
#define D_IN7_GPIO_Port GPIOB
#define D_IN6_Pin GPIO_PIN_11
#define D_IN6_GPIO_Port GPIOB
#define D_IN5_Pin GPIO_PIN_12
#define D_IN5_GPIO_Port GPIOB
#define D_IN4_Pin GPIO_PIN_13
#define D_IN4_GPIO_Port GPIOB
#define LED_Pin GPIO_PIN_14
#define LED_GPIO_Port GPIOB
#define D_IN3_Pin GPIO_PIN_8
#define D_IN3_GPIO_Port GPIOA
#define D_IN2_Pin GPIO_PIN_11
#define D_IN2_GPIO_Port GPIOA
#define D_IN1_Pin GPIO_PIN_12
#define D_IN1_GPIO_Port GPIOA
#define D_OUT3_Pin GPIO_PIN_5
#define D_OUT3_GPIO_Port GPIOB
#define D_OUT5_Pin GPIO_PIN_7
#define D_OUT5_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
#define CONFIG 1
#define ESPERA 2
#define MEDICION 3
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
