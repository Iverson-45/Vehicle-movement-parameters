/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "st7789.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BUFFER_SIZE 32
#define TXT_BUFFER_SIZE 30
#define DEBUG_BUFFER_SIZE 64
#define ACC_THRESHOLD 0.02f
#define UI_UPDATE_DELAY 50
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// Telemetry data
volatile int16_t speed = 0;
volatile float acceleration = 0.0f;
volatile int16_t pitch = 0;
volatile int16_t roll = 0;
volatile float max_acceleration = 0.0f;

// UART receive
char rx_byte;
char buffer[BUFFER_SIZE];
uint8_t buffer_index = 0;

// Display update tracking
float old_speed = -999.0f;
float old_acceleration = -999.0f;
float old_max_acceleration = -999.0f;
int16_t old_pitch = -999;
int16_t old_roll = -999;

// Buffers
char txt_buffer[TXT_BUFFER_SIZE];
char debug_buf[DEBUG_BUFFER_SIZE];

// State machine: 99=Handshake, 0=Connected, 1=Gravity OK, 2=Full OK, 3=Running
volatile uint8_t system_state = 99;
uint8_t ui_initialized = 0;

// Confirmation flags
volatile uint8_t send_confirm_1 = 0;
volatile uint8_t send_confirm_2 = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void PerformHandshake(void);
void HandleConfirmation1(void);
void HandleConfirmation2(void);
void InitializeUI(void);
void UpdateDisplay(void);
void ProcessUARTBuffer(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void PerformHandshake(void)
{
    ST7789_WriteString(70, 100, "Connecting...", Font_16x26, CYAN, BLACK);

    uint8_t sync_char = 'd';
    while(system_state == 99)
    {
        HAL_UART_Transmit(&huart1, &sync_char, 1, 10);
        HAL_Delay(500);
    }

    ST7789_Fill_Color(BLACK);
    ST7789_WriteString(50, 100, "Connected!", Font_16x26, GREEN, BLACK);
    sprintf(debug_buf, "Connected! Entering State 0\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)debug_buf, strlen(debug_buf), 100);
    HAL_Delay(1000);

    ST7789_Fill_Color(BLACK);
    ST7789_WriteString(20, 100, "Waiting for", Font_11x18, YELLOW, BLACK);
    ST7789_WriteString(20, 125, "Calibration...", Font_11x18, YELLOW, BLACK);
}

void HandleConfirmation1(void)
{
    uint8_t msg = '1';
    HAL_UART_Transmit(&huart1, &msg, 1, 10);
    send_confirm_1 = 0;

    ST7789_Fill_Color(BLACK);
    ST7789_WriteString(10, 80, "Gravity OK!", Font_11x18, GREEN, BLACK);
    ST7789_WriteString(10, 110, "Move Forward ->", Font_11x18, WHITE, BLACK);

    sprintf(debug_buf, "Sent Confirm 1. Screen Updated.\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)debug_buf, strlen(debug_buf), 100);
}

void HandleConfirmation2(void)
{
    uint8_t msg = '2';
    HAL_UART_Transmit(&huart1, &msg, 1, 10);
    send_confirm_2 = 0;

    ST7789_Fill_Color(BLACK);
    ST7789_WriteString(40, 100, "CALIBRATION", Font_11x18, CYAN, BLACK);
    ST7789_WriteString(60, 130, "COMPLETE", Font_11x18, CYAN, BLACK);

    sprintf(debug_buf, "Sent Confirm 2. Calibration Done.\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)debug_buf, strlen(debug_buf), 100);

    HAL_Delay(1500);
    system_state = 3;
}

void InitializeUI(void)
{
    ST7789_Fill_Color(BLACK);
    ST7789_WriteString(10, 20,  "SPEED [km/h]:", Font_11x18, WHITE, BLACK);
    ST7789_WriteString(10, 70,  "ACCEL [m/s2]:", Font_11x18, WHITE, BLACK);
    ST7789_WriteString(10, 120, "PITCH [deg]:",  Font_11x18, WHITE, BLACK);
    ST7789_WriteString(10, 170, "ROLL  [deg]:",  Font_11x18, WHITE, BLACK);
    ST7789_WriteString(10, 215, "MAX ACC:",      Font_11x18, RED,   BLACK);
    ui_initialized = 1;
}

void UpdateDisplay(void)
{
    float current_acc = acceleration;
    int16_t current_speed = speed;
    int16_t current_pitch = pitch;
    int16_t current_roll = roll;

    // Track max acceleration
    if (current_acc > max_acceleration) max_acceleration = current_acc;

    // Update max acceleration display
    if (fabsf(max_acceleration - old_max_acceleration) > ACC_THRESHOLD)
    {
        sprintf(txt_buffer, "%5.2f", max_acceleration);
        ST7789_WriteString(180, 215, txt_buffer, Font_11x18, RED, BLACK);
        old_max_acceleration = max_acceleration;
    }

    // Update speed display
    if (current_speed != (int16_t)old_speed)
    {
        sprintf(txt_buffer, "%4d", current_speed);
        ST7789_WriteString(180, 20, txt_buffer, Font_11x18, WHITE, BLACK);
        old_speed = (float)current_speed;
    }

    // Update acceleration display
    if (fabsf(current_acc - old_acceleration) > ACC_THRESHOLD)
    {
        sprintf(txt_buffer, "%5.2f", current_acc);
        ST7789_WriteString(180, 70, txt_buffer, Font_11x18, WHITE, BLACK);
        old_acceleration = current_acc;
    }

    // Update pitch display
    if (current_pitch != old_pitch)
    {
        sprintf(txt_buffer, "%4d", current_pitch);
        ST7789_WriteString(180, 120, txt_buffer, Font_11x18, WHITE, BLACK);
        old_pitch = current_pitch;
    }

    // Update roll display
    if (current_roll != old_roll)
    {
        sprintf(txt_buffer, "%4d", current_roll);
        ST7789_WriteString(180, 170, txt_buffer, Font_11x18, WHITE, BLACK);
        old_roll = current_roll;
    }
}

void ProcessUARTBuffer(void)
{
    if (buffer_index == 0) return;

    buffer[buffer_index] = '\0';

    // Handshake
    if (strstr(buffer, "ack") != NULL)
    {
        if (system_state == 99)
        {
            system_state = 0;
            HAL_UART_Transmit(&huart2, (uint8_t*)"[LOG]: Handshake OK\r\n", 21, 100);
        }
    }
    // Calibration step 1
    else if (strstr(buffer, "c1") != NULL)
    {
        if (system_state < 1)
        {
            system_state = 1;
            send_confirm_1 = 1;
            HAL_UART_Transmit(&huart2, (uint8_t*)"[LOG]: Recv c1\r\n", 16, 100);
        }
    }
    // Calibration step 2
    else if (strstr(buffer, "c2") != NULL)
    {
        if (system_state < 2)
        {
            system_state = 2;
            send_confirm_2 = 1;
            HAL_UART_Transmit(&huart2, (uint8_t*)"[LOG]: Recv c2\r\n", 16, 100);
        }
    }
    // Telemetry data
    else if (system_state == 3 || (system_state == 2 && (buffer[0]=='v' || buffer[0]=='a')))
    {
        if(system_state == 2) system_state = 3;

        switch (buffer[0])
        {
            case 'v':
                speed = atoi(&buffer[1]);
                sprintf(debug_buf, "VAR Speed: %d\r\n", speed);
                HAL_UART_Transmit(&huart2, (uint8_t*)debug_buf, strlen(debug_buf), 10);
                break;

            case 'a':
                acceleration = atof(&buffer[1]);
                sprintf(debug_buf, "VAR Acc: %.2f\r\n", acceleration);
                HAL_UART_Transmit(&huart2, (uint8_t*)debug_buf, strlen(debug_buf), 10);
                break;

            case 'p':
                pitch = atoi(&buffer[1]);
                break;

            case 'r':
                roll = atoi(&buffer[1]);
                break;
        }
    }

    buffer_index = 0;
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
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    // Initialize display
    ST7789_Init();
    ST7789_Fill_Color(BLACK);
    ST7789_WriteString(98, 100, "Welcome", Font_16x26, WHITE, BLACK);
    HAL_Delay(3000);

    sprintf(debug_buf, "\r\n=== LCD SYSTEM START ===\r\n");
    HAL_UART_Transmit(&huart2, (uint8_t*)debug_buf, strlen(debug_buf), 100);

    // Start UART reception
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);

    // Perform handshake
    PerformHandshake();

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        // Handle calibration confirmations
        if (send_confirm_1) HandleConfirmation1();
        if (send_confirm_2) HandleConfirmation2();

        if (system_state == 3)
        {
            if (!ui_initialized) InitializeUI();
            UpdateDisplay();
            HAL_Delay(UI_UPDATE_DELAY);
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
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        if (rx_byte == '\n' || rx_byte == '\r')
        {
            ProcessUARTBuffer();
        }
        else if (buffer_index < BUFFER_SIZE - 2)
        {
            // Skip leading whitespace
            if (buffer_index > 0 || (rx_byte != ' ' && rx_byte != 0))
            {
                buffer[buffer_index++] = rx_byte;
            }
        }
        else
        {
            buffer_index = 0;
        }

        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_PEFLAG(huart);
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);
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
#endif
