/*
 * uart2.h
 *
 *  Created on: Nov 23, 2025
 *      Author: darki
 */

#ifndef UART2_H_
#define UART2_H_

#include "stm32f3xx.h"
#include "gpio.h"


void Uart2_Init(void);
void UART2_SendChar(char c);
void UART2_SendString(const char *str);

#endif /* UART2_H_ */
