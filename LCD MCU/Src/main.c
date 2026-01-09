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
#include "fatfs.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "tim.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>

#include "utils.h"
#include "input_handler.h"
#include "sd_logger.h"
#include "app_ui.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    int32_t speed;
    float   accel;
    int32_t pitch;
    int32_t roll;
    uint8_t count;
} AvgAccumulator;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RX_BUFFER_SIZE 64
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
AppState_t      currentAppState = STATE_INIT;
TelemetryData_t telemetryData = {0};

uint8_t  rxBufferIndex = 0;
uint8_t  rxByte;
volatile uint8_t isLineReady = 0;
uint8_t  uiRefreshRequest = 0;

char rxBuffer[RX_BUFFER_SIZE];

static AvgAccumulator sdAccumulator = {0};
static AvgAccumulator displayAccumulator = {0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void ProcessUARTLine(void);
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
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  // Initialize Input and Logger immediately (non-blocking logic requires them)
  Input_Init(&htim3);
  Logger_Init();
  UI_Init();
  HAL_UART_Receive_IT(&huart1, &rxByte, 1);

  // Start with Splash Screen
  currentAppState = STATE_SPLASH;
  UI_ShowSplashScreen();

  uint32_t splashTimer = HAL_GetTick();
  uint8_t splashPromptDisplayed = 0;
  static AppState_t previousAppState = STATE_INIT;

  uint32_t lastPingTime = 0;
  uint32_t startTime = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      Input_Process(&currentAppState, &uiRefreshRequest);

      // Splash Screen Logic
      if (currentAppState == STATE_SPLASH)
      {
          if (!splashPromptDisplayed && (HAL_GetTick() - splashTimer > 1000))
          {
              UI_ShowSplashPrompt();
              splashPromptDisplayed = 1;
          }
          // Button press detection is handled in Input_Process,
          // which transitions state to STATE_CONNECTING
      }

      // Detect transition to STATE_CONNECTING to initialize timers
      if (previousAppState == STATE_SPLASH && currentAppState == STATE_CONNECTING)
      {
          UI_ShowConnectionStatus(0);
          lastPingTime = HAL_GetTick();
          startTime = HAL_GetTick();
      }
      previousAppState = currentAppState;

      if (isLineReady)
      {
          ProcessUARTLine();
          isLineReady = 0;
          rxBufferIndex = 0;
          memset(rxBuffer, 0, RX_BUFFER_SIZE);
      }

      if (currentAppState == STATE_CONNECTING)
      {
          if (HAL_GetTick() - lastPingTime > 500)
          {
              uint8_t pingChar = 'd';
              HAL_UART_Transmit(&huart1, &pingChar, 1, 10);
              lastPingTime = HAL_GetTick();
          }
          if (HAL_GetTick() - startTime > 10000)
          {
              UI_ShowConnectionStatus(2);
              HAL_Delay(2000);
              NVIC_SystemReset();
          }
      }

      Logger_FlushBuffer();

      if (currentAppState == STATE_MEASUREMENT)
      {
          if (Logger_GetStatus() == SD_READY && !Logger_IsFileOpen())
          {
               Logger_StartNewFile();
          }
      }

      if (uiRefreshRequest > 0 && currentAppState != STATE_CONNECTING)
      {
          UI_Update(currentAppState, &uiRefreshRequest, &telemetryData);
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
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (rxByte == '\n' || rxByte == '\r')
        {
            if (rxBufferIndex > 0)
            {
                rxBuffer[rxBufferIndex] = 0;
                isLineReady = 1;
            }
        }
        else if (rxBufferIndex < RX_BUFFER_SIZE - 2)
        {
            rxBuffer[rxBufferIndex++] = rxByte;
        }
        else
        {
            rxBufferIndex = 0;
        }
        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    __HAL_UART_CLEAR_OREFLAG(huart);
    HAL_UART_Receive_IT(&huart1, &rxByte, 1);
}

void ProcessUARTLine(void)
{
    if (currentAppState == STATE_CONNECTING)
    {
        if (strstr(rxBuffer, "ack"))
        {
            UI_ShowConnectionStatus(1);
            HAL_Delay(1000);
            currentAppState = STATE_MASS_INPUT;
            uiRefreshRequest = 2;
        }
    }
    else if (currentAppState == STATE_CALIBRATION)
    {
        if (strstr(rxBuffer, "c1"))
        {
            uint8_t msg = '1';
            HAL_UART_Transmit(&huart1, &msg, 1, 10);
            uiRefreshRequest = 3;
        }
        else if (strstr(rxBuffer, "c2"))
        {
            uint8_t msg = '2';
            HAL_UART_Transmit(&huart1, &msg, 1, 10);
            uiRefreshRequest = 4;
            HAL_Delay(500);

            Logger_StartNewFile();
            currentAppState = STATE_MEASUREMENT;
            uiRefreshRequest = 2;
        }
    }
    else if (currentAppState == STATE_MEASUREMENT)
    {
        int16_t speed = 0, pitch = 0, roll = 0;
        float   accel = 0.0f;
        char    header = rxBuffer[0];
        char* valStr = &rxBuffer[1];

        if      (header == 'v') speed = StringToInt(valStr);
        else if (header == 'a') accel = StringToFloat(valStr);
        else if (header == 'p') pitch = StringToInt(valStr);
        else if (header == 'r') roll  = StringToInt(valStr);

        telemetryData.timestamp = HAL_GetTick();

        if (header == 'v') telemetryData.speed = speed;
        if (header == 'a') telemetryData.accel = accel;
        if (header == 'p') telemetryData.pitch = pitch;
        if (header == 'r') telemetryData.roll  = roll;

        // Logging Accumulator (2 samples)
        sdAccumulator.speed += telemetryData.speed;
        sdAccumulator.accel += telemetryData.accel;
        sdAccumulator.pitch += telemetryData.pitch;
        sdAccumulator.roll  += telemetryData.roll;
        sdAccumulator.count++;

        if (sdAccumulator.count >= 2)
        {
            TelemetryData_t avgData;
            avgData.timestamp = telemetryData.timestamp;
            avgData.speed     = sdAccumulator.speed / 2;
            avgData.accel     = sdAccumulator.accel / 2.0f;
            avgData.pitch     = sdAccumulator.pitch / 2;
            avgData.roll      = sdAccumulator.roll / 2;

            Logger_AddData(&avgData);
            memset(&sdAccumulator, 0, sizeof(sdAccumulator));
        }

        // Display Accumulator (8 samples)
        displayAccumulator.speed += telemetryData.speed;
        displayAccumulator.accel += telemetryData.accel;
        displayAccumulator.pitch += telemetryData.pitch;
        displayAccumulator.roll  += telemetryData.roll;
        displayAccumulator.count++;

        if (displayAccumulator.count >= 8)
        {
            telemetryData.speed = displayAccumulator.speed / 8;
            telemetryData.accel = displayAccumulator.accel / 8.0f;
            telemetryData.pitch = displayAccumulator.pitch / 8;
            telemetryData.roll  = displayAccumulator.roll / 8;

            uiRefreshRequest = 1;
            memset(&displayAccumulator, 0, sizeof(displayAccumulator));
        }
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
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  * where the assert_param error has occurred.
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
