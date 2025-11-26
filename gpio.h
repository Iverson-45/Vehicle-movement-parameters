/*
 * gpio.h
 *
 *  Created on: Nov 23, 2025
 *      Author: darki
 */

#ifndef GPIO_H_
#define GPIO_H_

#include "stm32f3xx.h"

void I2cPins(void);
void Uart2Pins(void);
void Uart1Pins(void);
void Init_GPIO_Interrupt(void);

#endif /* GPIO_H_ */

