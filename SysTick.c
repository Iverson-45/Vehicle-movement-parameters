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

void delay_us(uint32_t us)
{
    uint32_t start = Tick;
    while ((Tick - start) < us);
}

void delay_ms(uint32_t ms)
{
	for(uint32_t i=0; i<1000; i++)
	{
		delay_us(ms);
	}
}

