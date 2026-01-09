#ifndef APP_UI_H
#define APP_UI_H

#include "main.h"
#include "input_handler.h"
#include "sd_logger.h"
#include "st7789.h"

void UI_Init(void);
void UI_ShowSplashScreen(void);
void UI_ShowSplashPrompt(void);
void UI_ShowConnectionStatus(uint8_t status);
void UI_Update(AppState_t currentState, uint8_t *refreshLevel, TelemetryData_t *telemetry);

#endif
