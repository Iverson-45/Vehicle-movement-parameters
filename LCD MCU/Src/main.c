/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (LCD Receiver - Racing Layout V3)
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
#define VAL_THRESHOLD 0.02f
#define UI_UPDATE_DELAY 50
#define SAMPLES_COUNT 5
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint32_t weight = 1700;

// Telemetry data (Volatile for ISR)
volatile int16_t speed = 0;
volatile float acceleration = 0.0f;
volatile int16_t pitch = 0;
volatile int16_t roll = 0;

// Records (MAX/MIN)
volatile int16_t max_speed = 0;
volatile float max_acceleration = 0.0f;
volatile float min_acceleration = 0.0f; // Debug: Min Acc
float max_power_hp = 0.0f;

// Averaging buffers
float acc_samples[SAMPLES_COUNT];
int16_t pitch_samples[SAMPLES_COUNT];
int16_t roll_samples[SAMPLES_COUNT];
uint8_t sample_index = 0;
uint8_t samples_ready = 0;

// UART receive
char rx_byte;
char buffer[BUFFER_SIZE];
uint8_t buffer_index = 0;

// Display tracking (Anti-flicker)
float old_speed = -999.0f;
float old_max_speed = -999.0f;

float old_acc_avg = -999.0f;
float old_max_acc = -999.0f;
float old_min_acc = 999.0f;

float old_power_hp = -999.0f;
float old_max_power_hp = -999.0f;

int16_t old_pitch_avg = -999;
int16_t old_roll_avg = -999;

char txt_buffer[TXT_BUFFER_SIZE];
char debug_buf[DEBUG_BUFFER_SIZE];

volatile uint8_t system_state = 99;
uint8_t ui_initialized = 0;

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
float CalculateAverageFloat(float *arr, uint8_t size);
int16_t CalculateAverageInt(int16_t *arr, uint8_t size);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void ShowCentered(uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bgcolor)
{
    uint16_t str_len = strlen(str);
    uint16_t str_width = str_len * font.width;

    uint16_t x = 0;
    if (str_width < 320) {
        x = (320 - str_width) / 2;
    }

    ST7789_WriteString(x, y, str, font, color, bgcolor);
}

float CalculateAverageFloat(float *arr, uint8_t size)
{
    float sum = 0.0f;
    for(uint8_t i=0; i<size; i++) sum += arr[i];
    return sum / size;
}

int16_t CalculateAverageInt(int16_t *arr, uint8_t size)
{
    int32_t sum = 0;
    for(uint8_t i=0; i<size; i++) sum += arr[i];
    return (int16_t)(sum / size);
}

void PerformHandshake(void)
{
    ST7789_Fill_Color(BLACK);

    ShowCentered(80, "Miernik Parametrow", Font_16x26, WHITE, BLACK);
    ShowCentered(110, "Ruchu Pojazdu",      Font_16x26, WHITE, BLACK);

    ST7789_WriteString(6, 200, "ver. 1.0", Font_7x10, WHITE, BLACK);

    HAL_Delay(2000);
    ST7789_Fill_Color(BLACK);

    ShowCentered(90, "Connecting...", Font_16x26, WHITE, BLACK);

    uint8_t sync_char = 'd';
    while(system_state == 99) {
        HAL_UART_Transmit(&huart1, &sync_char, 1, 10);
        HAL_Delay(500);
    }

    ST7789_Fill_Color(BLACK);
    ShowCentered(70, "Connected!", Font_16x26, WHITE, BLACK);

    HAL_Delay(1000);

    ST7789_Fill_Color(BLACK);
    ShowCentered(50, "Waiting for",    Font_16x26, WHITE, BLACK);
    ShowCentered(90, "Calibration...", Font_16x26, WHITE, BLACK);
}

void HandleConfirmation1(void)
{
    uint8_t msg = '1';
    HAL_UART_Transmit(&huart1, &msg, 1, 10);
    send_confirm_1 = 0;

    ST7789_Fill_Color(BLACK);
    ShowCentered(50, "Gravity OK!", Font_16x26, WHITE, BLACK);
    ShowCentered(90, "Move Forward", Font_16x26, WHITE, BLACK);
}

void HandleConfirmation2(void)
{
    uint8_t msg = '2';
    HAL_UART_Transmit(&huart1, &msg, 1, 10);
    send_confirm_2 = 0;

    ST7789_Fill_Color(BLACK);
    ShowCentered(50, "CALIBRATION", Font_16x26, WHITE, BLACK);
    ShowCentered(90, "COMPLETE", Font_16x26, WHITE, BLACK);

    HAL_Delay(1500);
    system_state = 3;
}

void InitializeUI(void)
{
    ST7789_Fill_Color(BLACK);

    ST7789_DrawLine(0, 42, 320, 42, GRAY);
    ST7789_DrawLine(0, 84, 320, 84, GRAY);
    ST7789_DrawLine(0, 126, 320, 126, GRAY);
    ST7789_DrawLine(160, 0, 160, 126, GRAY);

    ST7789_WriteString(5, 10,  "v:",  Font_16x26, WHITE, BLACK);
    ST7789_WriteString(110, 20, "km/h", Font_7x10, LIGHTGREEN, BLACK); // Przesunięte w lewo
    // a
    ST7789_WriteString(5, 52,  "a:",  Font_16x26, WHITE, BLACK);
    ST7789_WriteString(110, 62, "m/s2", Font_7x10, LIGHTGREEN, BLACK); // Poprawione
    // P
    ST7789_WriteString(5, 94,  "P:",  Font_16x26, WHITE, BLACK);
    ST7789_WriteString(110, 104, "HP", Font_7x10, LIGHTGREEN, BLACK);  // Duże litery

    // --- Column 2 (MAX) ---
    // v_m
    ST7789_WriteString(165, 10, "v", Font_16x26, YELLOW, BLACK);
    ST7789_WriteString(180, 26, "max", Font_7x10,  YELLOW, BLACK);
    ST7789_WriteString(204, 10, ":", Font_16x26, YELLOW, BLACK);
    // a_m
    ST7789_WriteString(165, 52, "a", Font_16x26, YELLOW, BLACK);
    ST7789_WriteString(180, 68, "max", Font_7x10,  YELLOW, BLACK);
    ST7789_WriteString(204, 52, ":", Font_16x26, YELLOW, BLACK);
    // P_m
    ST7789_WriteString(165, 94, "P", Font_16x26, YELLOW, BLACK);
    ST7789_WriteString(180, 110, "max", Font_7x10,  YELLOW, BLACK);
    ST7789_WriteString(204, 94, ":", Font_16x26, YELLOW, BLACK);

    // --- Bottom (Pitch/Roll) ---
    ST7789_WriteString(5, 135, "Pitch:",  Font_11x18, CYAN, BLACK);
    ST7789_WriteString(165, 135, "Roll:",   Font_11x18, CYAN, BLACK);

    // --- DEBUG: MIN ACCELERATION ---
    ST7789_WriteString(120, 160, "amin:", Font_7x10, RED, BLACK);

    ui_initialized = 1;
}

void UpdateDisplay(void)
{
    // Buffer fill
    acc_samples[sample_index] = acceleration;
    pitch_samples[sample_index] = pitch;
    roll_samples[sample_index] = roll;

    sample_index++;
    if(sample_index >= SAMPLES_COUNT) {
        sample_index = 0;
        samples_ready = 1;
    }

    // --- RECORDS TRACKING (MAX/MIN) ---
    if (speed > max_speed) max_speed = speed;
    if (acceleration > max_acceleration) max_acceleration = acceleration;
    if (acceleration < min_acceleration) min_acceleration = acceleration;

    // --- DISPLAY REFRESH (Every 5 samples) ---
    if(samples_ready)
    {
        // Averages
        float avg_acc = CalculateAverageFloat(acc_samples, SAMPLES_COUNT);
        int16_t avg_pitch = CalculateAverageInt(pitch_samples, SAMPLES_COUNT);
        int16_t avg_roll = CalculateAverageInt(roll_samples, SAMPLES_COUNT);

        // Power Calc
        float power_calc = 0.0f;
        if (avg_acc > 0 && speed > 0) {
            power_calc = ((float)weight * avg_acc * (float)speed) / 2684.5f;
        } else {
            power_calc = 0.0f;
        }

        if (power_calc > max_power_hp) max_power_hp = power_calc;

        // ---------------- DISPLAY ----------------

        // 1. SPEED (v) - X=40 (przesunięte w lewo z 50)
        if (speed != (int16_t)old_speed) {
            sprintf(txt_buffer, "%3d", speed);
            ST7789_WriteString(40, 10, txt_buffer, Font_16x26, WHITE, BLACK);
            old_speed = (float)speed;
        }
        // v_m (max) - X=210
        if (max_speed != (int16_t)old_max_speed) {
            sprintf(txt_buffer, "%3d", max_speed);
            ST7789_WriteString(210, 10, txt_buffer, Font_16x26, YELLOW, BLACK);
            old_max_speed = (float)max_speed;
        }

        // 2. ACCELERATION (a) - X=40
        if (fabsf(avg_acc - old_acc_avg) > VAL_THRESHOLD) {
            sprintf(txt_buffer, "%2.1f", avg_acc);
            ST7789_WriteString(40, 52, txt_buffer, Font_16x26, WHITE, BLACK);
            old_acc_avg = avg_acc;
        }
        // a_m (max) - X=210
        if (fabsf(max_acceleration - old_max_acc) > VAL_THRESHOLD) {
            sprintf(txt_buffer, "%4.2f", max_acceleration);
            ST7789_WriteString(210, 52, txt_buffer, Font_16x26, YELLOW, BLACK);
            old_max_acc = max_acceleration;
        }

        // 3. POWER (P) - X=40
        if (fabsf(power_calc - old_power_hp) > 0.1f) {
            sprintf(txt_buffer, "%3.0f", power_calc);
            ST7789_WriteString(40, 94, txt_buffer, Font_16x26, WHITE, BLACK);
            old_power_hp = power_calc;
        }
        // P_m (max) - X=210
        if (fabsf(max_power_hp - old_max_power_hp) > 0.1f) {
            sprintf(txt_buffer, "%3.0f", max_power_hp);
            ST7789_WriteString(210, 94, txt_buffer, Font_16x26, YELLOW, BLACK);
            old_max_power_hp = max_power_hp;
        }

        // 4. ANGLES (Pitch / Roll) - Zbliżone do etykiet
        if (avg_pitch != old_pitch_avg) {
            sprintf(txt_buffer, "%3d", avg_pitch);
            ST7789_WriteString(75, 132, txt_buffer, Font_16x26, WHITE, BLACK); // X=75
            old_pitch_avg = avg_pitch;
        }
        if (avg_roll != old_roll_avg) {
            sprintf(txt_buffer, "%3d", avg_roll);
            ST7789_WriteString(225, 132, txt_buffer, Font_16x26, WHITE, BLACK); // X=225
            old_roll_avg = avg_roll;
        }

        // 5. DEBUG: MIN ACCELERATION
        if (fabsf(min_acceleration - old_min_acc) > VAL_THRESHOLD) {
            sprintf(txt_buffer, "%4.1f", min_acceleration);
            ST7789_WriteString(160, 160, txt_buffer, Font_7x10, RED, BLACK);
            old_min_acc = min_acceleration;
        }

        samples_ready = 0;
    }
}

void ProcessUARTBuffer(void)
{
    if (buffer_index == 0) return;
    buffer[buffer_index] = '\0';

    if (strstr(buffer, "ack") != NULL) {
        if (system_state == 99) system_state = 0;
    }
    else if (strstr(buffer, "c1") != NULL) {
        if (system_state < 1) { system_state = 1; send_confirm_1 = 1; }
    }
    else if (strstr(buffer, "c2") != NULL) {
        if (system_state < 2) { system_state = 2; send_confirm_2 = 1; }
    }
    else if (system_state == 3 || (system_state == 2 && (buffer[0]=='v' || buffer[0]=='a')))
    {
        if(system_state == 2) system_state = 3;
        switch (buffer[0])
        {
            case 'v': speed = atoi(&buffer[1]); break;
            case 'a': acceleration = atof(&buffer[1]); break;
            case 'p': pitch = atoi(&buffer[1]); break;
            case 'r': roll = atoi(&buffer[1]); break;
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
    HAL_Init();
    SystemClock_Config();

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    ST7789_Init();
    ST7789_Fill_Color(BLACK);

    // Start UART reception
    HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);

    PerformHandshake();
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        if (send_confirm_1) HandleConfirmation1();
        if (send_confirm_2) HandleConfirmation2();

        if (system_state == 3)
        {
            if (!ui_initialized) InitializeUI();
            UpdateDisplay();
            HAL_Delay(10);
        }
        /* USER CODE END WHILE */
    }
    /* USER CODE END 3 */
}

/* ... (Reszta funkcji konfiguracyjnych i callbacków bez zmian) ... */
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
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        if (rx_byte == '\n' || rx_byte == '\r') ProcessUARTBuffer();
        else if (buffer_index < BUFFER_SIZE - 2) {
            if (buffer_index > 0 || (rx_byte != ' ' && rx_byte != 0)) buffer[buffer_index++] = rx_byte;
        } else buffer_index = 0;
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART1) {
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_PEFLAG(huart);
        HAL_UART_Receive_IT(&huart1, (uint8_t *)&rx_byte, 1);
    }
}
void Error_Handler(void) { __disable_irq(); while (1); }
