#ifndef UART_H
#define UART_H

#include "stm32f4xx.h"
#include "gpio.h"
#include <stdint.h>

void Uart1_Init(void);
void Uart2_Init(void);
void Uart6_Init(void);

void UART_SendChar(USART_TypeDef *USARTx, char c);
void UART_SendString(USART_TypeDef *USARTx, const char *str);
void UART2_SendByte(uint8_t data);

extern volatile uint8_t lcd_ack_state;
void USART6_IRQHandler(void);

#endif
