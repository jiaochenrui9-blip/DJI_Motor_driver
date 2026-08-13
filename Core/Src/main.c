/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "dji_motor_manager.h"
#include "dji_m3508.h"
#include "dji_m2006.h"
#include "dji_gm6020.h"
#include "motor_control.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* 主循环每1ms更新一次轴控制并发送CAN电流。 */
#define MOTOR_CONTROL_PERIOD_MS 1U
/* 八电机低速速度闭环测试目标。 */
#define M3508_TEST_SPEED_RPM  1000
#define GM6020_TEST_SPEED_RPM  50
#define M2006_TEST_SPEED_RPM   100

#define M3508_TEST_CURRENT_LIMIT 4000.0f
#define GM6020_TEST_CURRENT_LIMIT 1000.0f
#define M2006_TEST_CURRENT_LIMIT 2000.0f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;
CAN_HandleTypeDef hcan2;

UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
static DJI_MotorManager_t dji_motor_manager_can1;
static DJI_MotorManager_t dji_motor_manager_can2;
static DJI_Motor_t m3508_motors[4];
static DJI_Motor_t gm6020_motors[2];
static DJI_Motor_t m2006_motors[2];

static volatile HAL_StatusTypeDef motor_tx_status_can1 = HAL_OK;
static volatile HAL_StatusTypeDef motor_tx_status_can2 = HAL_OK;

static uint32_t last_motor_control_tick = 0U;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN1_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_CAN2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_CAN1_Init();
  MX_USART6_UART_Init();
  MX_CAN2_Init();
  /* USER CODE BEGIN 2 */
  if (DJI_MotorManager_Init(&dji_motor_manager_can1,&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  if (DJI_MotorManager_Init(&dji_motor_manager_can2,&hcan2) != HAL_OK)
  {
    Error_Handler();
  }

  for (uint8_t index = 0U; index < 4U; ++index)
  {
    if ((DJI_M3508_Register(&dji_motor_manager_can1, &m3508_motors[index],
                            index + 1U, 0x201U + index,
                            0x200U, index) != HAL_OK) ||
        (MotorControl_Init(&m3508_motors[index], M3508_TEST_CURRENT_LIMIT,
                           1, 1,
                           3.0f, 0.02f, 0.0f,
                           15.0f, 0.0f, 0.0f) != HAL_OK))
    {
      Error_Handler();
    }

    MotorControl_SetMode(&m3508_motors[index], MOTOR_MODE_SPEED);
    MotorControl_SetTargetSpeed(&m3508_motors[index], M3508_TEST_SPEED_RPM);
  }

  for (uint8_t index = 0U; index < 2U; ++index)
  {
    if ((DJI_GM6020_Register(&dji_motor_manager_can2, &gm6020_motors[index],
                             index + 1U, 0x205U + index,
                             0x1FEU, index) != HAL_OK) ||
        (MotorControl_Init(&gm6020_motors[index], GM6020_TEST_CURRENT_LIMIT,
                           1, 1,
                           20.0f, 0.02f, 0.0f,
                           15.0f, 0.0f, 0.0f) != HAL_OK))
    {
      Error_Handler();
    }

    MotorControl_SetMode(&gm6020_motors[index], MOTOR_MODE_SPEED);
    MotorControl_SetTargetSpeed(&gm6020_motors[index], GM6020_TEST_SPEED_RPM);
  }

  for (uint8_t index = 0U; index < 2U; ++index)
  {
    if ((DJI_M2006_Register(&dji_motor_manager_can2, &m2006_motors[index],
                            index + 1U, 0x201U + index,
                            0x200U, index) != HAL_OK) ||
        (MotorControl_Init(&m2006_motors[index], M2006_TEST_CURRENT_LIMIT,
                           1, 1,
                           3.0f, 0.02f, 0.0f,
                           15.0f, 0.0f, 0.0f) != HAL_OK))
    {
      Error_Handler();
    }

    MotorControl_SetMode(&m2006_motors[index], MOTOR_MODE_SPEED);
    MotorControl_SetTargetSpeed(&m2006_motors[index], M2006_TEST_SPEED_RPM);
  }

  if ((DJI_MotorManager_Start(&dji_motor_manager_can1) != HAL_OK) ||
      (DJI_MotorManager_Start(&dji_motor_manager_can2) != HAL_OK))
  {
    Error_Handler();
  }


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    const uint32_t now_tick = HAL_GetTick();

    if ((now_tick - last_motor_control_tick) >= MOTOR_CONTROL_PERIOD_MS)
    {
      last_motor_control_tick = now_tick;

      for (uint8_t index = 0U; index < 4U; ++index)
      {
        MotorControl_Update(&m3508_motors[index], now_tick);
      }

      for (uint8_t index = 0U; index < 2U; ++index)
      {
        MotorControl_Update(&gm6020_motors[index], now_tick);
        MotorControl_Update(&m2006_motors[index], now_tick);
      }

      motor_tx_status_can1 = DJI_MotorManager_SendAll(&dji_motor_manager_can1);
      motor_tx_status_can2 = DJI_MotorManager_SendAll(&dji_motor_manager_can2);
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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 3;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_9TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_4TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = ENABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = ENABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief CAN2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN2_Init(void)
{

  /* USER CODE BEGIN CAN2_Init 0 */

  /* USER CODE END CAN2_Init 0 */

  /* USER CODE BEGIN CAN2_Init 1 */

  /* USER CODE END CAN2_Init 1 */
  hcan2.Instance = CAN2;
  hcan2.Init.Prescaler = 3;
  hcan2.Init.Mode = CAN_MODE_NORMAL;
  hcan2.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan2.Init.TimeSeg1 = CAN_BS1_9TQ;
  hcan2.Init.TimeSeg2 = CAN_BS2_4TQ;
  hcan2.Init.TimeTriggeredMode = DISABLE;
  hcan2.Init.AutoBusOff = ENABLE;
  hcan2.Init.AutoWakeUp = DISABLE;
  hcan2.Init.AutoRetransmission = ENABLE;
  hcan2.Init.ReceiveFifoLocked = DISABLE;
  hcan2.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN2_Init 2 */

  /* USER CODE END CAN2_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/* HAL收到FIFO0消息后，把处理工作转交给M3508管理器。 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  if (hcan->Instance == CAN1)
  {
    DJI_MotorManager_RxFifo0Callback(&dji_motor_manager_can1, hcan);
  }
  else if (hcan->Instance == CAN2)
  {
    DJI_MotorManager_RxFifo0Callback(&dji_motor_manager_can2, hcan);
  }
}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */

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
#ifdef USE_FULL_ASSERT
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
