#include "uart.h"

void Uart1_Init(void) // UART1 -> PC (115200 baud)
{
    Uart1Pins();

    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    USART1->CR1 &= ~USART_CR1_UE;
    USART1->CR1 = 0;
    USART1->CR2 = 0;
    USART1->CR3 = 0;

    USART1->BRR = 96000000UL / 115200;

    USART1->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);
}

void Uart2_Init(void) // UART2 -> GPS (9600 baud)
{
    Uart2Pins();

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    USART2->CR1 &= ~USART_CR1_UE;
    USART2->CR1 = 0;

    USART2->BRR = 48000000UL / 9600;

    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);

    USART2->CR1 |= USART_CR1_RXNEIE;
    NVIC_SetPriority(USART2_IRQn, 2);
    NVIC_EnableIRQ(USART2_IRQn);
}

void Uart6_Init(void) // UART6 -> STM32
{
    Uart6Pins(); // PA11 (TX), PA12 (RX) - AF8

    RCC->APB2ENR |= RCC_APB2ENR_USART6EN;

    USART6->CR1 &= ~USART_CR1_UE;
    USART6->CR1 = 0;

    USART6->BRR = 96000000UL / 115200;

    USART6->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);
}

void UART_SendChar(USART_TypeDef *USARTx, char c)
{
    while (!(USARTx->SR & USART_SR_TXE));
    USARTx->DR = (uint8_t)c;
}

void UART_SendString(USART_TypeDef *USARTx, const char *str)
{
    while (*str) UART_SendChar(USARTx, *str++);
}

void UART2_SendByte(uint8_t data)
{
    while (!(USART2->SR & USART_SR_TXE));
    USART2->DR = data;
}
