#ifndef ST7567S_H
#define ST7567S_H

#include <stdint.h>
#include <stdbool.h>
#include "i2c.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define ST7567_WIDTH   128
#define ST7567_HEIGHT  64
#define ST7567_PAGES   (ST7567_HEIGHT / 8)
#define DISPLAY_I2C_ADDR 0x3F
#define ST7567_CONTROL_CMD  0x00
#define ST7567_CONTROL_DATA 0x40


void ST7567_Init(void);
void ST7567_DisplayOn(void);
void ST7567_DisplayOff(void);
void ST7567_Clear(void);
void ST7567_Invert(bool invert);
void ST7567_SetContrast(uint8_t val);
void ST7567_Flush(void);
void ST7567_SetPixel(uint16_t x, uint16_t y, bool color);
bool ST7567_GetPixel(uint16_t x, uint16_t y);
void ST7567_DrawLine(int x0, int y0, int x1, int y1, bool color);
void ST7567_DrawRect(int x, int y, int w, int h, bool color);
void ST7567_FillRect(int x, int y, int w, int h, bool color);
void ST7567_DrawCircle(int x0, int y0, int r, bool color);
void ST7567_DrawBitmap(const uint8_t *bmp, uint16_t w, uint16_t h, int x, int y);
void ST7567_ScrollH(int offset);
void ST7567_ScrollV(int pages);
void ST7567_Sleep(bool enable);
void ST7567_SetRotation(bool inverted);

void ST7567_SetCursor(uint8_t col, uint8_t row);
void ST7567_Putc(char c);
void ST7567_Puts(const char *s);
void ST7567_Printf(const char *fmt, ...);


#endif // ST7567S_H

