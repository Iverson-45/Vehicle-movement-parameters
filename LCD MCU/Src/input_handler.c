#include "input_handler.h"
#include "sd_logger.h"
#include <stdlib.h>

static TIM_HandleTypeDef *hEncoderTimer;
static int16_t previousEncoderCount = 0;
static uint32_t lastButtonPressTime = 0;

static uint8_t massInputBuffer[4] = {0, 0, 0, 0};
static uint8_t inputCursorIndex = 0;
static uint8_t menuSelection = 0;
static uint16_t vehicleMassKg = 0;
static uint8_t pauseMenuOption = 0; // 0=RESUME, 1=STOP, 2=EJECT

void Input_Init(TIM_HandleTypeDef *timerHandle)
{
    hEncoderTimer = timerHandle;
    HAL_TIM_Encoder_Start(hEncoderTimer, TIM_CHANNEL_ALL);
    previousEncoderCount = (int16_t)__HAL_TIM_GET_COUNTER(hEncoderTimer);
}

void Input_Process(AppState_t *currentState, uint8_t *refreshFlag)
{
    int16_t currentCount = (int16_t)__HAL_TIM_GET_COUNTER(hEncoderTimer);
    int16_t diff = currentCount - previousEncoderCount;

    if (abs(diff) >= 2)
    {
        int8_t direction = (diff > 0) ? 1 : -1;

        if (*currentState == STATE_MASS_INPUT)
        {
            int8_t newValue = massInputBuffer[inputCursorIndex] + direction;

            if (newValue > 9) newValue = 0;
            else if (newValue < 0) newValue = 9;

            massInputBuffer[inputCursorIndex] = (uint8_t)newValue;
            *refreshFlag = 1;
        }
        else if (*currentState == STATE_MENU)
        {
            int8_t selection = menuSelection + direction;

            if (selection > 2) selection = 0;
            else if (selection < 0) selection = 2;

            menuSelection = (uint8_t)selection;
            *refreshFlag = 1;
        }
        else if (*currentState == STATE_MEASURE_OPT)
        {
            int8_t option = pauseMenuOption + direction;

            if (option > 2) option = 0;
            else if (option < 0) option = 2;

            pauseMenuOption = (uint8_t)option;
            *refreshFlag = 1;
        }
        previousEncoderCount = currentCount;
    }

    if (HAL_GPIO_ReadPin(ENC_BTN_GPIO_Port, ENC_BTN_Pin) == GPIO_PIN_RESET)
    {
        if (HAL_GetTick() - lastButtonPressTime > 300)
        {
            if (*currentState == STATE_SPLASH)
            {
                *currentState = STATE_CONNECTING;
                *refreshFlag = 2;
            }
            else if (*currentState == STATE_MASS_INPUT)
            {
                inputCursorIndex++;
                if (inputCursorIndex >= 4)
                {
                    vehicleMassKg = (massInputBuffer[0] * 1000) +
                                    (massInputBuffer[1] * 100) +
                                    (massInputBuffer[2] * 10) +
                                    massInputBuffer[3];
                    *currentState = STATE_MENU;
                    *refreshFlag = 2;
                }
                else *refreshFlag = 1;
            }
            else if (*currentState == STATE_MENU)
            {
                if (menuSelection == 0)
                    *currentState = STATE_CALIBRATION;
                else if (menuSelection == 1)
                {
                    inputCursorIndex = 0;
                    *currentState = STATE_MASS_INPUT;
                }
                else if (menuSelection == 2)
                    Logger_EjectCard();

                *refreshFlag = 2;
            }
            else if (*currentState == STATE_MEASUREMENT)
            {
                pauseMenuOption = 0;
                *currentState = STATE_MEASURE_OPT;
                *refreshFlag = 2;
            }
            else if (*currentState == STATE_MEASURE_OPT)
            {
                if (pauseMenuOption == 0)
                {
                    *currentState = STATE_MEASUREMENT;
                    *refreshFlag = 2;
                }
                else if (pauseMenuOption == 1)
                {
                    Logger_StopRecording();
                    NVIC_SystemReset();
                }
                else if (pauseMenuOption == 2)
                {
                    Logger_EjectCard();
                    *currentState = STATE_MENU;
                    *refreshFlag = 2;
                }
            }
            lastButtonPressTime = HAL_GetTick();
        }
    }
}

uint16_t Input_GetVehicleMass(void)      { return vehicleMassKg; }
uint8_t  Input_GetMenuSelection(void)    { return menuSelection; }
uint8_t  Input_GetInputCursorIndex(void) { return inputCursorIndex; }
uint8_t* Input_GetInputBuffer(void)      { return massInputBuffer; }
uint8_t  Input_GetPauseMenuOption(void)  { return pauseMenuOption; }
