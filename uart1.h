#ifndef UART1_H
#define UART1_H

#include "stm32f3xx.h"
#include "gpio.h"
#include "uart2.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void Uart1_Init(void);
void GPS_ProcessData(void);
uint8_t GPS_IsLineReady(void);
const char* GPS_GetLine(void);
void GPS_ClearLine(void);
void GPS_SendLineToPC(void);

#endif
