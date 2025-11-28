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


    UART2_SendString("Start\n\r");

    char buffor[64];

    BMI160_Data imu;

    //uint8_t Lcd_counter = 0;

    char chuj[16];

    while(1)
	{
            if (gps_ready_flag)
            {
            	sprintf(chuj,"speed: %.2f", ParseSpeed());
				UART2_SendString((const char *)chuj);
				UART2_SendString("\r\n");

				UART2_SendString((const char *)buffer);
				UART2_SendString("\r\n");

				gps_ready_flag = 0;

				//Lcd_counter++;
            }

            if (bmi160_data_ready)
			{
				BMI160_ReadData(&imu);

				sprintf(buffor, "ax: %.2f, ay: %.2f, az: %.2f \r\n", imu.ax, imu.ay, imu.az);
				UART2_SendString(buffor);

				bmi160_data_ready = 0;
			}

            /*if (Lcd_counter == 3)
            {
            	ST7567_Clear();

            	ST7567_SetCursor(4, 32);

            	ST7567_Puts(chuj);

            	ST7567_Flush();

            	Lcd_counter = 0;
            }*/
	}
}
