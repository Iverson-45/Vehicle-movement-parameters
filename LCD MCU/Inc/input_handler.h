#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include "main.h"

typedef enum {
    STATE_INIT,
    STATE_SPLASH,
    STATE_CONNECTING,
    STATE_MASS_INPUT,
    STATE_MENU,
    STATE_CALIBRATION,
    STATE_MEASUREMENT,
    STATE_MEASURE_OPT
} AppState_t;

void Input_Init(TIM_HandleTypeDef *timerHandle);
void Input_Process(AppState_t *currentState, uint8_t *refreshFlag);

uint16_t Input_GetVehicleMass(void);
uint8_t  Input_GetMenuSelection(void);
uint8_t  Input_GetInputCursorIndex(void);
uint8_t* Input_GetInputBuffer(void);
uint8_t  Input_GetPauseMenuOption(void);

#endif
