#include "app_ui.h"
#include "st7789.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

static char stringBuffer[64];
static int16_t previousSpeed = -999;
static float previousAccel = -999.0f;
static float previousPower = -999.0f;
static int16_t previousPitch = -999;
static int16_t previousRoll = -999;
static SD_Status_t lastSDStatus = SD_ERROR;

#define COLOR_PINK        0xF81F
#define COLOR_BOX_IDLE    0x2104
#define COLOR_DARKGRAY    0x2104
#define COLOR_GRAY        0x8410

static void SelectDisplay(void)
{
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

static void DrawCenteredText(uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bg)
{
    uint16_t width = strlen(str) * font.width;
    ST7789_WriteString((320 - width) / 2, y, str, font, color, bg);
}

static void DrawRightAlignedText(uint16_t endX, uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bg)
{
    uint16_t width = strlen(str) * font.width;
    if (width > endX) return;
    ST7789_WriteString(endX - width, y, str, font, color, bg);
}

static void DrawThickPinkBorder(void)
{
    ST7789_DrawLine(0, 23, 319, 23, COLOR_PINK);
    ST7789_DrawLine(0, 239, 319, 239, COLOR_PINK);
    ST7789_DrawLine(0, 23, 0, 239, COLOR_PINK);
    ST7789_DrawLine(319, 23, 319, 239, COLOR_PINK);

    ST7789_DrawLine(1, 24, 318, 24, COLOR_PINK);
    ST7789_DrawLine(1, 238, 318, 238, COLOR_PINK);
    ST7789_DrawLine(1, 24, 1, 238, COLOR_PINK);
    ST7789_DrawLine(318, 24, 318, 238, COLOR_PINK);
}

static void DrawArrow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t dir, uint16_t color)
{
    if (dir == 0)
    {
        for(int i=0; i<h; i++)
            ST7789_DrawLine(x + w/2 - (i*w/h)/2, y + i, x + w/2 + (i*w/h)/2, y + i, color);
    }
    else if (dir == 1)
    {
        for(int i=0; i<h; i++)
            ST7789_DrawLine(x + w/2 - (i*w/h)/2, y + h - i, x + w/2 + (i*w/h)/2, y + h - i, color);
    }
    else if (dir == 2)
    {
        for(int i=0; i<w; i++)
            ST7789_DrawLine(x + i, y + h/2 - (i*h/w)/2, x + i, y + h/2 + (i*h/w)/2, color);
    }
    else if (dir == 3)
    {
        for(int i=0; i<w; i++)
            ST7789_DrawLine(x + w - i, y + h/2 - (i*h/w)/2, x + w - i, y + h/2 + (i*h/w)/2, color);
    }
}

void UI_Init(void)
{
    SelectDisplay();
    ST7789_Init();
    ST7789_Fill_Color(BLACK);
}

void UI_ShowSplashScreen(void)
{
    SelectDisplay();
    ST7789_Fill_Color(BLACK);

    ST7789_DrawFilledRectangle(0, 50, 320, 4, COLOR_PINK);
    DrawCenteredText(65, "MIERNIK PARAMETROW", Font_16x26, WHITE, BLACK);
    DrawCenteredText(100, "RUCHU POJAZDU", Font_16x26, WHITE, BLACK);
    ST7789_DrawFilledRectangle(0, 140, 320, 4, COLOR_PINK);

    ST7789_WriteString(5, 225, "ver 3.0", Font_7x10, COLOR_GRAY, BLACK);
    HAL_Delay(1500);
}

void UI_ShowSplashPrompt(void)
{
    SelectDisplay();
    DrawCenteredText(180, "Nacisnij aby kontynuowac", Font_11x18, COLOR_PINK, BLACK);
}

void UI_ShowConnectionStatus(uint8_t status)
{
    SelectDisplay();

    if (status == 0)
    {
        ST7789_Fill_Color(BLACK);
        DrawCenteredText(90, "LOADING...", Font_11x18, WHITE, BLACK);

        for (int i = 0; i <= 150; i += 10) {
            ST7789_DrawFilledRectangle(85, 132, i, 16, COLOR_PINK);
            HAL_Delay(25);
        }
    }
    else if (status == 1)
    {
        ST7789_DrawFilledRectangle(85 + 150, 132, 180 - 150, 16, COLOR_PINK);
        HAL_Delay(250);

        ST7789_Fill_Color(BLACK);
        DrawCenteredText(110, "POLACZONO POMYSLNIE!", Font_11x18, WHITE, BLACK);
    }
    else
    {
        ST7789_Fill_Color(BLACK);
        DrawCenteredText(80, "BLAD POLACZENIA!", Font_16x26, COLOR_PINK, BLACK);
    }
}

static void RenderMassInputScreen(uint8_t refreshLevel)
{
    if (refreshLevel == 2)
    {
        ST7789_Fill_Color(BLACK);
        ST7789_DrawFilledRectangle(0, 0, 320, 25, COLOR_BOX_IDLE);
        ST7789_DrawLine(0, 25, 320, 25, COLOR_PINK);
        DrawCenteredText(5, "KONFIGURACJA POJAZDU", Font_7x10, WHITE, COLOR_BOX_IDLE);

        DrawCenteredText(50, "WPROWADZ MASE [kg]", Font_11x18, COLOR_PINK, BLACK);
    }

    for(int i = 0; i < 4; i++)
    {
        uint16_t xPos = 55 + (i * 55);
        uint16_t bgColor = (i == inputCursorIndex) ? COLOR_PINK : COLOR_BOX_IDLE;
        uint16_t fontColor = (i == inputCursorIndex) ? WHITE : COLOR_GRAY;

        if (refreshLevel)
        {
            ST7789_DrawFilledRectangle(xPos, 100, 45, 60, bgColor);
            IntToString(massInputBuffer[i], stringBuffer);
            ST7789_WriteString(xPos + 14, 117, stringBuffer, Font_16x26, fontColor, bgColor);
        }
    }
}

static void RenderMainMenu(uint8_t refreshLevel)
{
    if (refreshLevel == 2) {
        ST7789_Fill_Color(BLACK);
        ST7789_DrawFilledRectangle(0, 0, 320, 25, COLOR_BOX_IDLE);
        ST7789_DrawLine(0, 25, 320, 25, COLOR_PINK);
        DrawCenteredText(5, "MENU GLOWNE", Font_7x10, WHITE, COLOR_BOX_IDLE);
    }

    uint16_t bg0 = (menuSelection == 0) ? COLOR_PINK : COLOR_BOX_IDLE;
    uint16_t tx0 = (menuSelection == 0) ? WHITE : COLOR_GRAY;
    ST7789_DrawFilledRectangle(50, 75, 220, 45, bg0);
    DrawCenteredText(88, "START POMIARU", Font_11x18, tx0, bg0);

    uint16_t bg1 = (menuSelection == 1) ? COLOR_PINK : COLOR_BOX_IDLE;
    uint16_t tx1 = (menuSelection == 1) ? WHITE : COLOR_GRAY;
    ST7789_DrawFilledRectangle(50, 140, 220, 45, bg1);
    DrawCenteredText(153, "ZMIEN MASE", Font_11x18, tx1, bg1);
}

static void RenderPauseMenu(uint8_t refreshLevel)
{
    if (refreshLevel == 2)
    {
        ST7789_Fill_Color(BLACK);
        ST7789_DrawFilledRectangle(0, 0, 320, 25, COLOR_BOX_IDLE);
        ST7789_DrawLine(0, 25, 320, 25, COLOR_PINK);
        DrawCenteredText(5, "SYSTEM PAUSED", Font_7x10, WHITE, COLOR_BOX_IDLE);
    }

    uint16_t bg0 = (pauseMenuOption == 0) ? COLOR_PINK : COLOR_DARKGRAY;
    uint16_t tx0 = WHITE;
    ST7789_DrawFilledRectangle(60, 60, 200, 35, bg0);
    DrawCenteredText(68, "WZNOW", Font_11x18, tx0, bg0);

    uint16_t bg1 = (pauseMenuOption == 1) ? COLOR_PINK : COLOR_DARKGRAY;
    ST7789_DrawFilledRectangle(60, 110, 200, 35, bg1);
    DrawCenteredText(118, "ZAKONCZ I ZAPISZ", Font_11x18, tx0, bg1);

    uint16_t bg2 = (pauseMenuOption == 2) ? COLOR_PINK : COLOR_DARKGRAY;
    ST7789_DrawFilledRectangle(60, 160, 200, 35, bg2);
    DrawCenteredText(168, "WYSUN KARTE", Font_11x18, tx0, bg2);
}

static void RenderCalibration(uint8_t step)
{
    ST7789_Fill_Color(BLACK);
    ST7789_DrawFilledRectangle(0, 0, 320, 25, COLOR_BOX_IDLE);
    ST7789_DrawLine(0, 25, 320, 25, COLOR_PINK);
    DrawCenteredText(5, "KALIBRACJA CZUJNIKOW", Font_7x10, WHITE, COLOR_BOX_IDLE);

    if (step == 0)
    {
        DrawCenteredText(85, "Etap 1: Stoj w miejscu", Font_11x18, COLOR_PINK, BLACK);
        DrawCenteredText(115, "(upewnij sie ze stoisz", Font_11x18, WHITE, BLACK);
        DrawCenteredText(135, "na plaskim podlozu)", Font_11x18, WHITE, BLACK);
    }
    else if (step == 1)
    {
        DrawCenteredText(85, "Etap 2: Rusz lekko do przodu", Font_11x18, COLOR_PINK, BLACK);
        DrawCenteredText(115, "(nie hamuj)", Font_11x18, WHITE, BLACK);
    }
    else
    {
        DrawCenteredText(110, "ZAKONCZONO!", Font_16x26, GREEN, BLACK);
    }
}

static void RenderDashboard(TelemetryData_t *data, uint8_t fullRedraw)
{
    static int16_t maxSpeed = 0;
    static float maxAccel = 0;
    static float maxPower = 0;

    static uint16_t prevUpColor = COLOR_DARKGRAY;
    static uint16_t prevDownColor = COLOR_DARKGRAY;
    static uint16_t prevLeftColor = COLOR_DARKGRAY;
    static uint16_t prevRightColor = COLOR_DARKGRAY;

    const uint16_t labelEndX = 90;
    const uint16_t dataStartX = 110;
    const uint16_t rowY[3] = {45, 105, 165};
    const uint16_t maxOffsY = 20;

    if (data->speed > maxSpeed) maxSpeed = data->speed;
    if (data->accel > maxAccel) maxAccel = data->accel;

    float currentPower = ((float)vehicleMassKg * data->accel * data->speed) / 2684.5f;
    if (currentPower < 0) currentPower = 0;
    if (currentPower > maxPower) maxPower = currentPower;

    if (fullRedraw)
    {
        ST7789_Fill_Color(BLACK);

        ST7789_DrawFilledRectangle(0, 0, 320, 22, COLOR_DARKGRAY);
        ST7789_WriteString(8, 6, "LIVE MEASURES", Font_7x10, WHITE, COLOR_DARKGRAY);
        ST7789_WriteString(260, 6, "SD:", Font_7x10, COLOR_GRAY, COLOR_DARKGRAY);

        DrawThickPinkBorder();

        DrawRightAlignedText(labelEndX, rowY[0], "SPEED", Font_11x18, COLOR_PINK, BLACK);
        DrawRightAlignedText(labelEndX, rowY[0] + maxOffsY, "MAX", Font_11x18, COLOR_PINK, BLACK);

        DrawRightAlignedText(labelEndX, rowY[1], "POWER", Font_11x18, COLOR_PINK, BLACK);
        DrawRightAlignedText(labelEndX, rowY[1] + maxOffsY, "MAX", Font_11x18, COLOR_PINK, BLACK);

        DrawRightAlignedText(labelEndX, rowY[2], "ACC.", Font_11x18, COLOR_PINK, BLACK);
        DrawRightAlignedText(labelEndX, rowY[2] + maxOffsY, "MAX", Font_11x18, COLOR_PINK, BLACK);

        DrawArrow(150, 32, 20, 10, 0, COLOR_DARKGRAY);
        DrawArrow(150, 215, 20, 10, 1, COLOR_DARKGRAY);
        DrawArrow(8, 115, 10, 20, 2, COLOR_DARKGRAY);
        DrawArrow(300, 115, 10, 20, 3, COLOR_DARKGRAY);

        prevUpColor = COLOR_DARKGRAY;
        prevDownColor = COLOR_DARKGRAY;
        prevLeftColor = COLOR_DARKGRAY;
        prevRightColor = COLOR_DARKGRAY;

        previousSpeed = -999; previousAccel = -999.0f; previousPower = -999.0f;
        previousPitch = -999; previousRoll = -999; lastSDStatus = SD_ERROR;
    }

    if (data->speed != previousSpeed)
    {
        sprintf(stringBuffer, "%3d km/h   ", data->speed);
        ST7789_WriteString(dataStartX, rowY[0], stringBuffer, Font_11x18, WHITE, BLACK);

        sprintf(stringBuffer, "%3d km/h   ", maxSpeed);
        ST7789_WriteString(dataStartX, rowY[0] + maxOffsY, stringBuffer, Font_11x18, WHITE, BLACK);
        previousSpeed = data->speed;
    }

    if (fabsf(currentPower - previousPower) > 1.0f)
    {
        sprintf(stringBuffer, "%4d HP   ", (int)currentPower);
        ST7789_WriteString(dataStartX, rowY[1], stringBuffer, Font_11x18, WHITE, BLACK);

        sprintf(stringBuffer, "%4d HP   ", (int)maxPower);
        ST7789_WriteString(dataStartX, rowY[1] + maxOffsY, stringBuffer, Font_11x18, WHITE, BLACK);
        previousPower = currentPower;
    }

    if (fabsf(data->accel - previousAccel) > 0.05f)
    {
        char tempStr[16];
        FloatToString(data->accel, tempStr, 1);
        sprintf(stringBuffer, "%5s m/s2   ", tempStr);
        ST7789_WriteString(dataStartX, rowY[2], stringBuffer, Font_11x18, WHITE, BLACK);

        FloatToString(maxAccel, tempStr, 1);
        sprintf(stringBuffer, "%5s m/s2   ", tempStr);
        ST7789_WriteString(dataStartX, rowY[2] + maxOffsY, stringBuffer, Font_11x18, WHITE, BLACK);

        previousAccel = data->accel;
    }

    uint16_t currentUpColor = (data->pitch > 2) ? COLOR_PINK : COLOR_DARKGRAY;
    uint16_t currentDownColor = (data->pitch < -2) ? COLOR_PINK : COLOR_DARKGRAY;
    uint16_t currentRightColor = (data->roll > 2) ? COLOR_PINK : COLOR_DARKGRAY;
    uint16_t currentLeftColor = (data->roll < -2) ? COLOR_PINK : COLOR_DARKGRAY;

    if (currentUpColor != prevUpColor) {
        DrawArrow(150, 32, 20, 10, 0, currentUpColor);
        prevUpColor = currentUpColor;
    }
    if (currentDownColor != prevDownColor) {
        DrawArrow(150, 215, 20, 10, 1, currentDownColor);
        prevDownColor = currentDownColor;
    }
    if (currentLeftColor != prevLeftColor) {
        DrawArrow(8, 115, 10, 20, 2, currentLeftColor);
        prevLeftColor = currentLeftColor;
    }
    if (currentRightColor != prevRightColor) {
        DrawArrow(300, 115, 10, 20, 3, currentRightColor);
        prevRightColor = currentRightColor;
    }

    SD_Status_t currentSDStatus = Logger_GetStatus();
    if (currentSDStatus != lastSDStatus)
    {
        uint16_t statusColor = (currentSDStatus == SD_READY) ? GREEN :
                               (currentSDStatus == SD_WRITING ? WHITE : COLOR_PINK);
        ST7789_DrawFilledCircle(295, 11, 4, statusColor);
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
