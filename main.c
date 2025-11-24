#include "main.h"

int main(void)
{
	//FPU on
	SCB->CPACR |= (0xF << 20);

    SysTick_Config(8000000 / 1000);
    Uart2_Init();

    UART2_SendString("System started\r\n");
    delay_ms(100);

    Uart1_Init();
    UBX_Init();

    UART2_SendString("GPS init done\r\n");

    while(1)
    {

        GPS_ProcessData();


        GPS_SendLineToPC();


    }
}

