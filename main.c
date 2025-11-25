#include "main.h"

int main(void)
{
	//FPU on
	SCB->CPACR |= (0xF << 20);

    SysTick_Config(8000000 / 1000);

    Uart2_Init();
    Uart1_Init();
    I2C_Init();

    GPS_Init();
    ST7567_Init();


    ST7567_SetCursor(0, 32);
    ST7567_Puts("System starting");
    ST7567_Flush();
    delay_ms(2000);

    ST7567_Clear();


    while(1)
        {
            GPS_ProcessData();

            GpsSendToPC();

            GpsDisplayLCD();
        }
}
