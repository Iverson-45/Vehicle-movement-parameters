#include "clock.h"

void SystemClock_Config_64MHz(void)
{
    FLASH->ACR |= FLASH_ACR_PRFTBE;
    FLASH->ACR &= ~FLASH_ACR_LATENCY;
    FLASH->ACR |= FLASH_ACR_LATENCY_1;

    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0);

    RCC->CR &= ~RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);

    RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLMUL);
    RCC->CFGR |= RCC_CFGR_PLLMUL16;
    RCC->CFGR &= ~RCC_CFGR_HPRE;
    RCC->CFGR &= ~RCC_CFGR_PPRE2;
    RCC->CFGR &= ~RCC_CFGR_PPRE1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);
}

void PeriphClock_Config(void)
{
    RCC->CFGR3 |= RCC_CFGR3_I2C1SW;
    RCC->CFGR3 &= ~RCC_CFGR3_USART1SW;
    RCC->CFGR3 |= RCC_CFGR3_USART1SW_0;

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // EXTI
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;   // I2C1
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN; // USART1
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN; // USART2
}


