#include "SysTick.h"


volatile uint32_t Tick;

void SysTick_Handler(void)
{
    Tick++;
}

uint32_t millis()
{
	return Tick;
}

void delay_ms(uint32_t ms)
{
    uint32_t start = Tick;
    while ((Tick - start) < ms);
}


