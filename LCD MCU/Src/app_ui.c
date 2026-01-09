#include "app_ui.h"
#include "st7789.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static char stringBuffer[24];
static int16_t previousSpeed = -999;
static float previousAccel = -999.0f;
static float previousPower = -999.0f;
static int16_t previousPitch = -999;
static int16_t previousRoll = -999;
static SD_Status_t lastSDStatus = SD_ERROR;

static void SelectDisplay(void)
{
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

static void DrawCenteredText(uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bg)
{
    uint16_t width = strlen(str) * font.width;
    ST7789_WriteString((320 - width) / 2, y, str, font, color, bg);
}

void UI_Init(void)
{
    SelectDisplay();
    ST7789_Init();
    ST7789_Fill_Color(0x0000);
}

void UI_ShowSplashScreen(void)
{
    SelectDisplay();
    ST7789_Fill_Color(0x0000);
    DrawCenteredText(65, "MIERNIK PARAMETROW", Font_16x26, WHITE, BLACK);
    DrawCenteredText(95, "RUCHU", Font_16x26, WHITE, BLACK);
    DrawCenteredText(125, "POJAZDU", Font_16x26, WHITE, BLACK);

    ST7789_WriteString(2, 220, "ver 1.0", Font_7x10, WHITE, BLACK);
}

void UI_ShowSplashPrompt(void)
{
    SelectDisplay();
    DrawCenteredText(180, "Press to continue", Font_11x18, YELLOW, BLACK);
}

void UI_ShowConnectionStatus(uint8_t status)
{
    SelectDisplay();
    ST7789_Fill_Color(0x0000);
    if (status == 0)
    {
        DrawCenteredText(55, "Connecting with", Font_16x26, WHITE, BLACK);
        DrawCenteredText(85, "main ECU...", Font_16x26, WHITE, BLACK);
    }
    else if (status == 1)
    {
        DrawCenteredText(70, "Connected!", Font_16x26, GREEN, BLACK);
    }
    else
    {
        DrawCenteredText(55, "Connection", Font_16x26, RED, BLACK);
        DrawCenteredText(85, "Failed!", Font_16x26, RED, BLACK);
    }
}

static void RenderMassInputScreen(uint8_t refreshLevel)
{
    if (refreshLevel == 2)
    {
        ST7789_Fill_Color(0x0000);
        DrawCenteredText(40, "ENTER MASS [kg]", Font_16x26, YELLOW, BLACK);
        ST7789_DrawRectangle(50, 80, 270, 140, GRAY);
    }

    uint8_t *digits = Input_GetInputBuffer();
    uint8_t cursor = Input_GetInputCursorIndex();

    for(int i = 0; i < 4; i++)
    {
        uint16_t xPos = 80 + (i * 45);
        uint16_t bgColor = (i == cursor) ? LIGHTGRAY : BLACK;

        if (refreshLevel)
        {
            ST7789_DrawFilledRectangle(xPos, 90, 30, 40, bgColor);
            sprintf(stringBuffer, "%d", digits[i]);
            ST7789_WriteString(xPos + 7, 100, stringBuffer, Font_16x26, WHITE, bgColor);
        }
    }
}

static void RenderMainMenu(uint8_t refreshLevel)
{
    if (refreshLevel == 2) ST7789_Fill_Color(BLACK);
    uint8_t selection = Input_GetMenuSelection();

    uint16_t bg0 = (selection == 0) ? LIGHTGRAY : BLACK;
    ST7789_DrawFilledRectangle(60, 60, 200, 30, bg0);
    ST7789_WriteString(85, 66, "START MEASURE", Font_11x18, WHITE, bg0);

    uint16_t bg1 = (selection == 1) ? LIGHTGRAY : BLACK;
    ST7789_DrawFilledRectangle(60, 100, 200, 30, bg1);
    ST7789_WriteString(100, 106, "CHANGE MASS", Font_11x18, WHITE, bg1);

    uint16_t bg2 = (selection == 2) ? LIGHTGRAY : BLACK;
    ST7789_DrawFilledRectangle(60, 140, 200, 30, bg2);
    ST7789_WriteString(100, 146, (Logger_GetStatus() == SD_NOT_PRESENT) ? "SD EJECTED" : "EJECT SD", Font_11x18, WHITE, bg2);
}

static void RenderPauseMenu(uint8_t refreshLevel)
{
    if (refreshLevel == 2)
    {
        ST7789_DrawFilledRectangle(40, 40, 280, 200, BLACK);
        ST7789_DrawRectangle(40, 40, 280, 200, YELLOW);
        DrawCenteredText(50, "PAUSED", Font_16x26, YELLOW, BLACK);
    }

    uint8_t selection = Input_GetPauseMenuOption();

    uint16_t bg0 = (selection == 0) ? LIGHTGRAY : BLACK;
    ST7789_DrawFilledRectangle(60, 90, 200, 30, bg0);
    ST7789_WriteString(110, 96, "RESUME", Font_11x18, WHITE, bg0);

    uint16_t bg1 = (selection == 1) ? LIGHTGRAY : BLACK;
    ST7789_DrawFilledRectangle(60, 130, 200, 30, bg1);
    ST7789_WriteString(90, 136, "STOP & SAVE", Font_11x18, WHITE, bg1);

    uint16_t bg2 = (selection == 2) ? LIGHTGRAY : BLACK;
    ST7789_DrawFilledRectangle(60, 170, 200, 30, bg2);
    ST7789_WriteString(110, 176, "EJECT", Font_11x18, WHITE, bg2);
}

static void RenderCalibration(uint8_t step)
{
    if (step == 0)
    {
        ST7789_Fill_Color(0x0000);
        DrawCenteredText(50, "CALIBRATION", Font_16x26, YELLOW, BLACK);
        DrawCenteredText(90, "Waiting...", Font_16x26, WHITE, BLACK);
    }
    else if (step == 1)
    {
        ST7789_Fill_Color(0x0000);
        DrawCenteredText(50, "GRAVITY OK", Font_16x26, GREEN, BLACK);
        DrawCenteredText(90, "Move Forward", Font_16x26, WHITE, BLACK);
    }
    else
    {
        ST7789_Fill_Color(0x0000);
        DrawCenteredText(70, "COMPLETE", Font_16x26, GREEN, BLACK);
    }
}

static void RenderDashboard(TelemetryData_t *data, uint8_t fullRedraw)
{
    static int16_t maxSpeed = 0;
    static float maxAccel = 0;
    static float maxPower = 0;

    if (data->speed > maxSpeed) maxSpeed = data->speed;
    if (data->accel > maxAccel) maxAccel = data->accel;

    float currentPower = ((float)Input_GetVehicleMass() * data->accel * data->speed) / 2684.5f;

    if (currentPower < 0) currentPower = 0;
    if (currentPower > maxPower) maxPower = currentPower;

    if (fullRedraw)
    {
        ST7789_Fill_Color(0x0000);
        ST7789_DrawRectangle(0, 0, 319, 239, GRAY);
        ST7789_DrawRectangle(2, 2, 317, 237, GRAY);
        ST7789_DrawLine(0, 42, 320, 42, GRAY);
        ST7789_DrawLine(0, 84, 320, 84, GRAY);
        ST7789_DrawLine(0, 126, 320, 126, GRAY);
        ST7789_DrawLine(160, 0, 160, 126, GRAY);

        ST7789_WriteString(5, 10, "v:", Font_16x26, WHITE, BLACK);
        ST7789_WriteString(110, 20, "km/h", Font_7x10, LIGHTGREEN, BLACK);
        ST7789_WriteString(5, 52, "a:", Font_16x26, WHITE, BLACK);
        ST7789_WriteString(110, 62, "m/s2", Font_7x10, LIGHTGREEN, BLACK);
        ST7789_WriteString(5, 94, "P:", Font_16x26, WHITE, BLACK);
        ST7789_WriteString(110, 104, "HP", Font_7x10, LIGHTGREEN, BLACK);

        ST7789_WriteString(165, 10, "vmax:", Font_16x26, YELLOW, BLACK);
        ST7789_WriteString(165, 52, "amax:", Font_16x26, YELLOW, BLACK);
        ST7789_WriteString(165, 94, "Pmax:", Font_16x26, YELLOW, BLACK);

        ST7789_WriteString(5, 135, "Pitch:", Font_11x18, CYAN, BLACK);
        ST7789_WriteString(165, 135, "Roll:", Font_11x18, CYAN, BLACK);

        previousSpeed = -999; previousAccel = -999; previousPower = -999;
        previousPitch = -999; previousRoll = -999;
        maxSpeed = 0; maxAccel = 0; maxPower = 0;
    }

    if (data->speed != previousSpeed)
    {
        sprintf(stringBuffer, "%3d", data->speed);
        ST7789_WriteString(45, 10, stringBuffer, Font_16x26, WHITE, BLACK);
        sprintf(stringBuffer, "%3d", maxSpeed);
        ST7789_WriteString(250, 10, stringBuffer, Font_16x26, YELLOW, BLACK);
        previousSpeed = data->speed;
    }

    if (fabsf(data->accel - previousAccel) > 0.05f)
    {
        FloatToString(data->accel, stringBuffer, 1);
        ST7789_WriteString(45, 52, stringBuffer, Font_16x26, WHITE, BLACK);
        FloatToString(maxAccel, stringBuffer, 1);
        ST7789_WriteString(250, 52, stringBuffer, Font_16x26, YELLOW, BLACK);
        previousAccel = data->accel;
    }

    if (fabsf(currentPower - previousPower) > 1.0f)
    {
        sprintf(stringBuffer, "%3d", (int)currentPower);
        ST7789_WriteString(45, 94, stringBuffer, Font_16x26, WHITE, BLACK);
        sprintf(stringBuffer, "%3d", (int)maxPower);
        ST7789_WriteString(250, 94, stringBuffer, Font_16x26, YELLOW, BLACK);
        previousPower = currentPower;
    }

    if (data->pitch != previousPitch)
    {
        sprintf(stringBuffer, "%4d", data->pitch);
        ST7789_WriteString(75, 135, stringBuffer, Font_11x18, WHITE, BLACK);
        previousPitch = data->pitch;
    }

    if (data->roll != previousRoll)
    {
        sprintf(stringBuffer, "%4d", data->roll);
        ST7789_WriteString(225, 135, stringBuffer, Font_11x18, WHITE, BLACK);
        previousRoll = data->roll;
    }

    SD_Status_t currentSDStatus = Logger_GetStatus();

    if (currentSDStatus != lastSDStatus || fullRedraw)
    {
        uint16_t statusColor = (currentSDStatus == SD_READY) ? GREEN :
                               (currentSDStatus == SD_WRITING ? YELLOW : RED);

        ST7789_DrawFilledCircle(310, 160, 5, statusColor);
        lastSDStatus = currentSDStatus;
    }
}

void UI_Update(AppState_t currentState, uint8_t *refreshLevel, TelemetryData_t *telemetry)
{
    SelectDisplay();
    switch(currentState)
    {
        case STATE_MASS_INPUT:
            if (*refreshLevel) RenderMassInputScreen(*refreshLevel);
            break;

        case STATE_MENU:
            if (*refreshLevel) RenderMainMenu(*refreshLevel);
            break;

        case STATE_CALIBRATION:
            if (*refreshLevel >= 2) RenderCalibration(0);
            if (*refreshLevel == 3) RenderCalibration(1);
            if (*refreshLevel == 4) RenderCalibration(2);
            break;

        case STATE_MEASUREMENT:
            RenderDashboard(telemetry, *refreshLevel == 2);
            break;

        case STATE_MEASURE_OPT:
            if (*refreshLevel) RenderPauseMenu(*refreshLevel);
            break;

        default:
            break;
    }
    *refreshLevel = 0;
}
