#include "main.h"

int main(void)
{
	//FPU on
	SCB->CPACR |= (0xF << 20);

	SystemClock_Config_64MHz();
	PeriphClock_Config();

    SysTick_Config(64000000 / 1000000);

    Uart2_Init();
    Uart1_Init();
    I2C_Init();
    Init_GPIO_Interrupt();

    ST7567_Init();
	ST7567_Invert(0);
	ST7567_SetRotation(1);
    ST7567_SetCursor(4, 32);
    ST7567_Puts("System starting");
	ST7567_Flush();

    GPS_Init();
    BMI160_Init();

    delay_ms(2000);
    ST7567_Clear();
    ST7567_Flush();




    char buffor[64];

    BMI160_Data imu;
    while(1)
	{
            GPS_ProcessData();
            GpsSendToPC();
            GpsDisplayLCD();

            if (bmi160_data_ready)
			{
				BMI160_ReadData(&imu);

				sprintf(buffor, "ax: %.2f, ay: %.2f, az: %.2f \r\n", imu.ax, imu.ay, imu.az);
				UART2_SendString(buffor);

				bmi160_data_ready = 0;
			}
	}
}
