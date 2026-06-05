#include "main.h"
#include "fatfs.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"
#include "tim.h"
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "input_handler.h"
#include "sd_logger.h"
#include "app_ui.h"

#define RX_BUFFER_SIZE 64

AppState_t      currentAppState = STATE_INIT;
TelemetryData_t telemetryData = {0};

uint8_t  rxBufferIndex = 0;
uint8_t  rxByte;
volatile uint8_t isLineReady = 0;
uint8_t  uiRefreshRequest = 0;
char rxBuffer[RX_BUFFER_SIZE];

void SystemClock_Config(void);
void ProcessUARTLine(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_FATFS_Init();

  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  Input_Init(&htim3);
  Logger_Init();
  UI_Init();
  HAL_UART_Receive_IT(&huart1, &rxByte, 1);

  currentAppState = STATE_SPLASH;
  UI_ShowSplashScreen();

  uint32_t splashTimer = HAL_GetTick();
  uint8_t splashPromptDisplayed = 0;
  static AppState_t previousAppState = STATE_INIT;

  uint32_t lastPingTime = 0;
  uint32_t startTime = 0;

  static uint8_t calibrationStateInitialized = 0;
  static uint32_t calibrationTimer = 0;
  static uint32_t lastCalibPing = 0;

  while (1)
  {
      Input_Process(&currentAppState, &uiRefreshRequest);

      if (currentAppState == STATE_SPLASH) {
          if (!splashPromptDisplayed && (HAL_GetTick() - splashTimer > 1000)) {
              UI_ShowSplashPrompt();
              splashPromptDisplayed = 1;
          }
      }

      if (previousAppState == STATE_SPLASH && currentAppState == STATE_CONNECTING) {
          UI_ShowConnectionStatus(0);
          lastPingTime = HAL_GetTick();
          startTime = HAL_GetTick();
      }
      previousAppState = currentAppState;

      if (isLineReady) {
          ProcessUARTLine();
          isLineReady = 0;
          rxBufferIndex = 0;
          memset(rxBuffer, 0, RX_BUFFER_SIZE);
      }

      if (currentAppState == STATE_CONNECTING) {
          if (HAL_GetTick() - lastPingTime > 500) {
              uint8_t pingChar = 'd';
              HAL_UART_Transmit(&huart1, &pingChar, 1, 10);
              lastPingTime = HAL_GetTick();
          }
          if (HAL_GetTick() - startTime > 10000) {
              UI_ShowConnectionStatus(2);
              HAL_Delay(2000);
              NVIC_SystemReset();
          }
      }
      else if (currentAppState == STATE_CALIBRATION) {
          if (calibrationStateInitialized == 0) {
              calibrationTimer = HAL_GetTick();
              lastCalibPing = HAL_GetTick();
              calibrationStateInitialized = 1;
          }

          if ((HAL_GetTick() - calibrationTimer) > 2000) {
              if (HAL_GetTick() - lastCalibPing > 200) {
                  uint8_t msg = 'k';
                  HAL_UART_Transmit(&huart1, &msg, 1, 10);
                  lastCalibPing = HAL_GetTick();
              }
          }
      } else {
          calibrationStateInitialized = 0;
      }

      Logger_FlushBuffer();

      if (currentAppState == STATE_MEASUREMENT) {
          if (Logger_GetStatus() == SD_READY && !Logger_IsFileOpen()) {
               Logger_StartNewFile();
          }
      }

      if (uiRefreshRequest > 0 && currentAppState != STATE_CONNECTING) {
          UI_Update(currentAppState, &uiRefreshRequest, &telemetryData);
      }
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (rxByte == '\n' || rxByte == '\r') {
            if (rxBufferIndex > 0) { rxBuffer[rxBufferIndex] = 0; isLineReady = 1; }
        }
        else if (rxBufferIndex < RX_BUFFER_SIZE - 2) { rxBuffer[rxBufferIndex++] = rxByte; }
        else { rxBufferIndex = 0; }
        HAL_UART_Receive_IT(&huart1, &rxByte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    __HAL_UART_CLEAR_OREFLAG(huart);
    HAL_UART_Receive_IT(&huart1, &rxByte, 1);
}

void ProcessUARTLine(void) {
    if (currentAppState == STATE_CONNECTING) {
        if (strstr(rxBuffer, "ack")) {
            UI_ShowConnectionStatus(1); HAL_Delay(1000);
            currentAppState = STATE_MASS_INPUT; uiRefreshRequest = 2;
        }
    }
    else if (currentAppState == STATE_CALIBRATION) {
        if (strstr(rxBuffer, "c1")) {
            uint8_t msg = '1'; HAL_UART_Transmit(&huart1, &msg, 1, 10);
            uiRefreshRequest = 3;
        }
        else if (strstr(rxBuffer, "c2")) {
            uint8_t msg = '2'; HAL_UART_Transmit(&huart1, &msg, 1, 10);
            uiRefreshRequest = 4; HAL_Delay(500);
            Logger_StartNewFile(); currentAppState = STATE_MEASUREMENT; uiRefreshRequest = 2;
        }
    }
    else if (currentAppState == STATE_MEASUREMENT) {
        int16_t speed = 0, pitch = 0, roll = 0; float accel = 0.0f;
        char header = rxBuffer[0]; char* valStr = &rxBuffer[1];

        if      (header == 'v') speed = StringToInt(valStr);
        else if (header == 'a') accel = StringToFloat(valStr);
        else if (header == 'p') pitch = StringToInt(valStr);
        else if (header == 'r') roll  = StringToInt(valStr);

        telemetryData.timestamp = HAL_GetTick();

        if (header == 'v') telemetryData.speed = speed;
        if (header == 'a') telemetryData.accel = accel;
        if (header == 'p') telemetryData.pitch = pitch;
        if (header == 'r') telemetryData.roll  = roll;

        // Ostatni znak transmisji z serii to "p", gdy dojdzie wysylamy wszystko!
        if (header == 'p')
        {
            Logger_AddData(&telemetryData);
            uiRefreshRequest = 1;
        }
    }
}

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
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                   RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
      Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
