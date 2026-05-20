#include "app_ui.h"
#include "st7789.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>


static char stringBuffer[32];
static int16_t previousSpeed = -999;
static float previousAccel = -999.0f;
static float previousPower = -999.0f;
static int16_t previousPitch = -999;
static int16_t previousRoll = -999;
static SD_Status_t lastSDStatus = SD_ERROR;


#define COLOR_BOX_ACTIVE  0x4208
#define COLOR_BOX_IDLE    BLACK
#define COLOR_LIME        0x87F0
#define COLOR_YELLOW      0xFFE0
#define COLOR_GRAY        0x8410
#define COLOR_CYAN        0x07FF
#define M_COLOR_LBLUE     0x03BF
#define M_COLOR_DBLUE     0x28D1
#define M_COLOR_RED       0xE800
#define M_COLOR_WHITE     0xFFFF


static void SelectDisplay(void)
{
    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

static void DrawCenteredText(uint16_t y, const char *str, FontDef font, uint16_t color, uint16_t bg)
{
    uint16_t width = strlen(str) * font.width;
    ST7789_WriteString((320 - width) / 2, y, str, font, color, bg);
}

static void DrawScreenBorder(uint16_t color)
{
    ST7789_DrawLine(0, 0, 319, 0, color);     // Top
    ST7789_DrawLine(0, 239, 319, 239, color); // Bottom
    ST7789_DrawLine(0, 0, 0, 239, color);     // Left
    ST7789_DrawLine(319, 0, 319, 239, color); // Right
}

// dir: 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT
static void DrawArrow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t dir, uint16_t color)
{
    if (dir == 0) // UP
    {
        for(int i=0; i<h; i++)
            ST7789_DrawLine(x + w/2 - (i*w/h)/2, y + i, x + w/2 + (i*w/h)/2, y + i, color);
    }
    else if (dir == 1) // DOWN
    {
        for(int i=0; i<h; i++)
            ST7789_DrawLine(x + w/2 - (i*w/h)/2, y + h - i, x + w/2 + (i*w/h)/2, y + h - i, color);
    }
    else if (dir == 2) // LEFT
    {
        for(int i=0; i<w; i++)
            ST7789_DrawLine(x + i, y + h/2 - (i*h/w)/2, x + i, y + h/2 + (i*h/w)/2, color);
    }
    else if (dir == 3) // RIGHT
    {
        for(int i=0; i<w; i++)
            ST7789_DrawLine(x + w - i, y + h/2 - (i*h/w)/2, x + w - i, y + h/2 + (i*h/w)/2, color);
    }
}

static void UI_DrawMLogo(uint16_t start_x, uint16_t start_y, uint16_t h)
{
    const uint16_t w_obj = h / 2.2;
    const float slant_factor = 0.72f;
    const uint16_t max_slant = (uint16_t)((float)h * slant_factor);
    uint16_t x_m_start = start_x + (w_obj * 3) + (h / 9);
    int16_t x_vert1 = x_m_start + max_slant + w_obj - 20;
    int16_t x_skew2_base = x_vert1 + w_obj - 22;
    int16_t x_vert2 = x_skew2_base + max_slant + w_obj - 20;

    for (uint16_t i = 0; i < h; i++)
    {
        uint16_t y = start_y + i;
        uint16_t slant = (uint16_t)((float)(h - i) * slant_factor);
        uint16_t x = start_x + slant;

        ST7789_DrawFilledRectangle(x, y, w_obj, 1, M_COLOR_LBLUE);
        x += w_obj;
        ST7789_DrawFilledRectangle(x, y, w_obj, 1, M_COLOR_DBLUE);
        x += w_obj;
        ST7789_DrawFilledRectangle(x, y, w_obj, 1, M_COLOR_RED);

        ST7789_DrawFilledRectangle(x_m_start + slant, y, w_obj, 1, M_COLOR_WHITE);
        ST7789_DrawFilledRectangle(x_vert1, y, w_obj-2, 1, M_COLOR_WHITE);
        ST7789_DrawFilledRectangle(x_skew2_base + slant, y, w_obj-2, 1, M_COLOR_WHITE);
        ST7789_DrawFilledRectangle(x_vert2, y, w_obj-4, 1, M_COLOR_WHITE);
    }
}
/* USER CODE END PFP */

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

    DrawCenteredText(30, "Miernik Parametrow", Font_16x26, WHITE, BLACK);
    DrawCenteredText(60, "Ruchu Pojazdu", Font_16x26, WHITE, BLACK);

   // UI_DrawMLogo(70, 110, 50);
    //ST7789_WriteString(68, 161, "Performance", Font_16x26, WHITE, BLACK);

    ST7789_WriteString(2, 220, "ver 1.0.2", Font_7x10, WHITE, BLACK);
    HAL_Delay(1500);
}

void UI_ShowSplashPrompt(void)
{
    SelectDisplay();
    DrawCenteredText(195, "Press to continue", Font_11x18, WHITE, BLACK);
}

void UI_ShowConnectionStatus(uint8_t status)
{
    SelectDisplay();
    ST7789_Fill_Color(BLACK);
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

    for(int i = 0; i < 4; i++)
    {
        uint16_t xPos = 80 + (i * 45);
        uint16_t bgColor = (i == inputCursorIndex) ? COLOR_BOX_ACTIVE : BLACK;

        if (refreshLevel)
        {
            ST7789_DrawFilledRectangle(xPos, 90, 30, 40, bgColor);
            IntToString(massInputBuffer[i], stringBuffer);
            ST7789_WriteString(xPos + 7, 100, stringBuffer, Font_16x26, WHITE, bgColor);
        }
    }
}

static void RenderMainMenu(uint8_t refreshLevel)
{
    if (refreshLevel == 2) ST7789_Fill_Color(BLACK);

    uint16_t bg0 = (menuSelection == 0) ? COLOR_BOX_ACTIVE : BLACK;
    ST7789_DrawFilledRectangle(60, 60, 200, 30, bg0);
    ST7789_WriteString(85, 66, "START MEASURE", Font_11x18, WHITE, bg0);

    uint16_t bg1 = (menuSelection == 1) ? COLOR_BOX_ACTIVE : BLACK;
    ST7789_DrawFilledRectangle(60, 100, 200, 30, bg1);
    ST7789_WriteString(100, 106, "CHANGE MASS", Font_11x18, WHITE, bg1);
}

static void RenderPauseMenu(uint8_t refreshLevel)
{
    if (refreshLevel == 2)
    {
    	ST7789_Fill_Color(BLACK);
        DrawCenteredText(50, "PAUSED", Font_16x26, YELLOW, BLACK);
    }

    uint16_t bg0 = (pauseMenuOption == 0) ? COLOR_BOX_ACTIVE : BLACK;
    ST7789_DrawFilledRectangle(70, 90, 180, 30, bg0);
    DrawCenteredText(96, "RESUME", Font_11x18, WHITE, bg0);

    uint16_t bg1 = (pauseMenuOption == 1) ? COLOR_BOX_ACTIVE : BLACK;
    ST7789_DrawFilledRectangle(70, 130, 180, 30, bg1);
    DrawCenteredText(136, "STOP & SAVE", Font_11x18, WHITE, bg1);

    uint16_t bg2 = (pauseMenuOption == 2) ? COLOR_BOX_ACTIVE : BLACK;
    ST7789_DrawFilledRectangle(70, 170, 180, 30, bg2);
    DrawCenteredText(176, "EJECT", Font_11x18, WHITE, bg2);
}

static void RenderCalibration(uint8_t step)
{
    if (step == 0)
    {
        ST7789_Fill_Color(0x0000);
        DrawCenteredText(50, "CALIBRATION STEP 1", Font_16x26, YELLOW, BLACK);
        DrawCenteredText(90, "Do not move", Font_16x26, YELLOW, BLACK);
        DrawCenteredText(130, "Waiting...", Font_16x26, WHITE, BLACK);
    }
    else if (step == 1)
    {
        ST7789_Fill_Color(0x0000);
        DrawCenteredText(50, "STEP 1 OK", Font_16x26, GREEN, BLACK);
        DrawCenteredText(90, "Move Forward", Font_16x26, WHITE, BLACK);
    }
    else
    {
        ST7789_Fill_Color(0x0000);
        DrawCenteredText(70, "STEP 2 COMPLETE", Font_16x26, GREEN, BLACK);
    }
}

static void RenderDashboard(TelemetryData_t *data, uint8_t fullRedraw)
{
    static int16_t maxSpeed = 0;
    static float maxAccel = 0;
    static float maxPower = 0;

    if (data->speed > maxSpeed) maxSpeed = data->speed;
    if (data->accel > maxAccel) maxAccel = data->accel;

    float currentPower = ((float)vehicleMassKg * data->accel * data->speed) / 2684.5f;
    if (currentPower < 0) currentPower = 0;
    if (currentPower > maxPower) maxPower = currentPower;

    if (fullRedraw)
    {
    	ST7789_Fill_Color(BLACK);
        DrawScreenBorder(WHITE);

        ST7789_DrawRectangle(1, 1, 318, 32, WHITE);
        DrawCenteredText(5, "Live measurements", Font_16x26, WHITE, BLACK);

        ST7789_WriteString(220, 44, "Gyro", Font_16x26, WHITE, BLACK);

        ST7789_WriteString(5, 45, "Speed:", Font_16x26, WHITE, BLACK);
        ST7789_WriteString(165, 50, "km/h", Font_11x18, WHITE, BLACK);

        ST7789_WriteString(45, 75, "Max:", Font_11x18, WHITE, BLACK);
        ST7789_WriteString(165, 77, "km/h", Font_7x10, WHITE, BLACK);

        ST7789_WriteString(5, 105, "Acc.:", Font_16x26, WHITE, BLACK);
        ST7789_WriteString(165, 110, "m/s", Font_11x18, WHITE, BLACK);
        ST7789_WriteString(198, 107, "2", Font_7x10, WHITE, BLACK);

        ST7789_WriteString(45, 135, "Max:", Font_11x18, WHITE, BLACK);
        ST7789_WriteString(165, 137, "m/s", Font_7x10, WHITE, BLACK);
        ST7789_WriteString(187, 135, "2", Font_7x10, WHITE, BLACK);

        ST7789_WriteString(5, 165, "Power:", Font_16x26, WHITE, BLACK);
        ST7789_WriteString(165, 170, "HP", Font_11x18, WHITE, BLACK);

        ST7789_WriteString(45, 195, "Max:", Font_11x18, WHITE, BLACK);
        ST7789_WriteString(165, 197, "HP", Font_7x10, WHITE, BLACK);

        DrawArrow(245, 90, 14, 8, 0, WHITE);
        DrawArrow(245, 190, 14, 8, 1, WHITE);
        DrawArrow(197, 133, 8, 14, 2, WHITE);
        DrawArrow(299, 133, 8, 14, 3, WHITE);

        ST7789_WriteString(199, 215, "SD card:", Font_11x18, WHITE, BLACK);
    }

    if (data->speed != previousSpeed || fullRedraw)
    {
        ST7789_DrawFilledRectangle(115, 50, 45, 18, BLACK);
        IntToString(data->speed, stringBuffer);
        ST7789_WriteString(115, 50, stringBuffer, Font_11x18, WHITE, BLACK);

        ST7789_DrawFilledRectangle(115, 77, 45, 18, BLACK);
        IntToString(maxSpeed, stringBuffer);
        ST7789_WriteString(115, 75, stringBuffer, Font_11x18, WHITE, BLACK);
        previousSpeed = data->speed;
    }

    if (fabsf(data->accel - previousAccel) > 0.05f || fullRedraw)
    {
        ST7789_DrawFilledRectangle(115, 110, 45, 18, BLACK);
        FloatToString(data->accel, stringBuffer, 1);
        ST7789_WriteString(115, 110, stringBuffer, Font_11x18, WHITE, BLACK);

        ST7789_DrawFilledRectangle(115, 137, 45, 18, BLACK);
        FloatToString(maxAccel, stringBuffer, 1);
        ST7789_WriteString(115, 135, stringBuffer, Font_11x18, WHITE, BLACK);
        previousAccel = data->accel;
    }

    if (fabsf(currentPower - previousPower) > 1.0f || fullRedraw)
    {
        ST7789_DrawFilledRectangle(115, 170, 45, 18, BLACK);
        IntToString((int32_t)currentPower, stringBuffer);
        ST7789_WriteString(115, 170, stringBuffer, Font_11x18, WHITE, BLACK);

        ST7789_DrawFilledRectangle(115, 197, 45, 18, BLACK);
        IntToString((int32_t)maxPower, stringBuffer);
        ST7789_WriteString(115, 195, stringBuffer, Font_11x18, WHITE, BLACK);
        previousPower = currentPower;
    }

    if (data->pitch != previousPitch || fullRedraw)
    {
        ST7789_DrawFilledRectangle(237, 105, 30, 25, BLACK);
        ST7789_DrawFilledRectangle(237, 155, 30, 25, BLACK);

        if (data->pitch != 0)
        {
            IntToString(abs(data->pitch), stringBuffer);
            int16_t val_w = strlen(stringBuffer) * 11;

            if (data->pitch > 0)
                ST7789_WriteString(252 - val_w/2, 105, stringBuffer, Font_11x18, WHITE, BLACK);
            else
                ST7789_WriteString(252 - val_w/2, 160, stringBuffer, Font_11x18, WHITE, BLACK);
        }
        previousPitch = data->pitch;
    }

    if (data->roll != previousRoll || fullRedraw)
    {
        ST7789_DrawFilledRectangle(210, 130, 30, 18, BLACK);
        ST7789_DrawFilledRectangle(265, 130, 30, 18, BLACK);

        if (data->roll != 0)
        {
            IntToString(abs(data->roll), stringBuffer);
            if (data->roll < 0)
            {
            	ST7789_WriteString(215, 131, stringBuffer, Font_11x18, WHITE, BLACK);
            }

            else
            {
            	ST7789_WriteString(268, 131, stringBuffer, Font_11x18, WHITE, BLACK);
            }

        }
        previousRoll = data->roll;
    }

    SD_Status_t currentSDStatus = Logger_GetStatus();
    if (currentSDStatus != lastSDStatus || fullRedraw)
    {
        uint16_t statusColor = (currentSDStatus == SD_READY) ? GREEN :
                               (currentSDStatus == SD_WRITING ? YELLOW : RED);

        ST7789_DrawFilledCircle(295, 224, 5, statusColor);
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
