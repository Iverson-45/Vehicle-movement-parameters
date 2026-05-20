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

extern uint16_t vehicleMassKg;
extern uint8_t  menuSelection;
extern uint8_t  inputCursorIndex;
extern uint8_t  massInputBuffer[4];
extern uint8_t  pauseMenuOption;

void Input_Init(TIM_HandleTypeDef *timerHandle);
void Input_Process(AppState_t *currentState, uint8_t *refreshFlag);

#endif
