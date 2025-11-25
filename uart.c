#include "uart.h"

void Uart2_Init(void)
{
	Uart2Pins();

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    USART2->CR1 &= ~USART_CR1_UE;

    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;

	USART2->BRR = 8000000 / 9600;

	USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);
}

void UART2_SendChar(char c)
{
    while (!(USART2->ISR & USART_ISR_TXE));
    USART2->TDR = c;
}

void UART2_SendString(const char *str)
{
    while (*str)
    {
        UART2_SendChar(*str++);
    }
}


void Uart1_Init(void)
{
    Uart1Pins();

    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    USART1->CR1 = 0;
    USART1->CR2 = 0;
    USART1->CR3 = 0;

    USART1->BRR = 8000000 / 9600;

    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    while (USART1->ISR & USART_ISR_RXNE)
        (void)USART1->RDR;
}


void UART1_SendByte(uint8_t data)
{
    while (!(USART1->ISR & USART_ISR_TXE));
    USART1->TDR = data;
}
