#include "gpio.h"

void I2cPins(void)
{
    // B6 - SCL, B7 - SDA
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // Alternate function (10)
    GPIOB->MODER &= ~(GPIO_MODER_MODER6 | GPIO_MODER_MODER7);
    GPIOB->MODER |= (GPIO_MODER_MODER6_1 | GPIO_MODER_MODER7_1);

    // Open-drain (1)
    GPIOB->OTYPER |= (GPIO_OTYPER_OT_6 | GPIO_OTYPER_OT_7);

    // High speed (10) lub Very High speed (11)
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED6_1 | GPIO_OSPEEDR_OSPEED7_1);


    // Pull-up (01)
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR6 | GPIO_PUPDR_PUPDR7);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPDR6_0 | GPIO_PUPDR_PUPDR7_0);

    // AF4 = I2C1 B6 i B7
    GPIOB->AFR[0] &= ~(GPIO_AFRL_AFSEL6 | GPIO_AFRL_AFSEL7);
    GPIOB->AFR[0] |= (4 << GPIO_AFRL_AFSEL6_Pos) | (4 << GPIO_AFRL_AFSEL7_Pos);
}

void Uart2Pins(void)
{
	// PA2 - TX, PA3 - RX
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER3);
    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER3_1);

    GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED2_1 | GPIO_OSPEEDR_OSPEED3_1);

    // PA2 no-pull, PA3 pull-up
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPDR2 | GPIO_PUPDR_PUPDR3);
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR3_0;

    // AF7 = USART2
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2 | GPIO_AFRL_AFSEL3);
    GPIOA->AFR[0] |= (7 << GPIO_AFRL_AFSEL2_Pos) | (7 << GPIO_AFRL_AFSEL3_Pos);
}

void Uart1Pins(void)
{
    // PA9 (TX), PA10 (RX)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    // --- PA9 (TX) ---
    GPIOA->MODER &= ~GPIO_MODER_MODER9;
    GPIOA->MODER |= GPIO_MODER_MODER9_1; // AF

    GPIOA->OSPEEDR |= GPIO_OSPEEDR_OSPEED9_1; // High Speed

    // AF7  USART1  PA9
    GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL9;
    GPIOA->AFR[1] |= (7 << GPIO_AFRH_AFSEL9_Pos);

    // --- PA10 (RX) ---
    GPIOA->MODER &= ~GPIO_MODER_MODER10;
    GPIOA->MODER |= GPIO_MODER_MODER10_1; // AF

    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPDR10;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPDR10_0; // Pull-up

    // AF7  USART1  PA10
    GPIOA->AFR[1] &= ~GPIO_AFRH_AFSEL10;
    GPIOA->AFR[1] |= (7 << GPIO_AFRH_AFSEL10_Pos);
}

void Uart6Pins(void)
{
	// PA11 - TX, PA12 - RX
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    GPIOA->MODER &= ~(GPIO_MODER_MODER11 | GPIO_MODER_MODER12);
    GPIOA->MODER |= (GPIO_MODER_MODER11_1 | GPIO_MODER_MODER12_1);

    // AF8  USART6
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL11 | GPIO_AFRH_AFSEL12);
    GPIOA->AFR[1] |= (8 << GPIO_AFRH_AFSEL11_Pos) | (8 << GPIO_AFRH_AFSEL12_Pos);
}

void Init_GPIO_Interrupt(void)
{
	// PB2 external interrupt
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // PB2 input
    GPIOB->MODER &= ~(GPIO_MODER_MODER2);

    // PB2 pull-down
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPDR2);
    GPIOB->PUPDR |= GPIO_PUPDR_PUPDR2_1;

    // Connect EXTI2 to PB2
    SYSCFG->EXTICR[0] &= ~(SYSCFG_EXTICR1_EXTI2);
    SYSCFG->EXTICR[0] |= SYSCFG_EXTICR1_EXTI2_PB;

    // EXTI Configuration
    EXTI->IMR |= EXTI_IMR_MR2;
    EXTI->RTSR |= EXTI_RTSR_TR2; // Rising edge
    EXTI->PR |= EXTI_PR_PR2;     // Clear pending flag

    // NVIC  EXTI2
    NVIC_SetPriority(EXTI2_IRQn, 2);
    NVIC_EnableIRQ(EXTI2_IRQn);
}
