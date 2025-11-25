#ifndef UART_H
#define UART_H

#include "stm32f3xx.h"
#include "gpio.h"
#include <stdint.h>


void Uart2_Init(void);
void UART2_SendChar(char c);
void UART2_SendString(const char *str);


void Uart1_Init(void);
void UART1_SendByte(uint8_t data);

#endif
