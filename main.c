#include "main.h"

void SystemInit(void)
{
	//FPU on
	SCB->CPACR |= (0xF << 20);
}

typedef struct {
	float x, y, z;
}vector;

const float alpha = 0.8;

const float dt = 0.01;

const float friction = 0.95;

uint8_t first_reading = 1;

int main(void)
{
	SystemClock_Config_100MHz();

    SysTick_Config(98000000 / 1000000);

    Uart6_Init();
    Uart2_Init();
    Uart1_Init();
    I2C_Init();
    Init_GPIO_Interrupt();

    GPS_Init();
    BMI160_Init();

    delay_ms(200);

    UART_SendString(USART1, "Start\n\r");

    char BMIread[64];

    BMI160_Data imu;

    vector orientation_gravity;
    vector real_gravity;
    
    vector acceleration;
    vector ImuVelocity;

    vector orientation_angle;
    vector real_angle;
    

    float scalar_acceleration = 0;

    char LCDspeed[32];

    while(1)
	{
            if (gps_ready_flag)
            {
            	sprintf(LCDspeed,"Speed: %.2f km/h", ParseSpeed());
				UART_SendString(USART1, (const char *)LCDspeed);
				UART_SendString(USART1, "\r\n");

				UART_SendString(USART1, (const char *)GPSbuffer);
				UART_SendString(USART1, "\r\n");

				gps_ready_flag = 0;
            }


            if (bmi160_data_ready)
			{
				BMI160_ReadData(&imu);

				//calibration
				if(first_reading)
				{
					orientation_gravity.x = imu.ax;
					orientation_gravity.y = imu.ay;
					orientation_gravity.z = imu.az;

					real_gravity.x = imu.ax;
					real_gravity.y = imu.ay;
					real_gravity.z = imu.az;

					real_angle.x = atan2f(real_gravity.y, real_gravity.z) * 180.0f / 3.14159f;
					real_angle.y = atan2f(-real_gravity.x, sqrtf(real_gravity.y*real_gravity.y + real_gravity.z*real_gravity.z)) * 180.0f / 3.14159f;

					first_reading = 0;
				}

				orientation_gravity.x = alpha * orientation_gravity.x + (1.0f - alpha) * imu.ax;
				orientation_gravity.y = alpha * orientation_gravity.y + (1.0f - alpha) * imu.ay;
				orientation_gravity.z = alpha * orientation_gravity.z + (1.0f - alpha) * imu.az;

				acceleration.x = imu.ax - orientation_gravity.x;
				acceleration.y = imu.ay - orientation_gravity.y;
				acceleration.z = imu.az - orientation_gravity.z;

				orientation_angle.x = (atan2f(orientation_gravity.y, orientation_gravity.z) * 180.0f / 3.14159f) - real_angle.x;
				orientation_angle.y = (atan2f(-orientation_gravity.x, sqrtf(orientation_gravity.y*orientation_gravity.y + orientation_gravity.z*orientation_gravity.z)) * 180.0f / 3.14159f) - real_angle.y;

				if (fabsf(acceleration.x) < 0.2f) acceleration.x = 0.0f;
				if (fabsf(acceleration.y) < 0.2f) acceleration.y = 0.0f;
				if (fabsf(acceleration.z) < 0.2f) acceleration.z = 0.0f;

				if (fabsf(orientation_angle.x) < 1.0f) orientation_angle.x = 0.0f;
				if (fabsf(orientation_angle.y) < 1.0f) orientation_angle.y = 0.0f;

				ImuVelocity.x += acceleration.x *dt;
				ImuVelocity.y += acceleration.y *dt;
				ImuVelocity.z += acceleration.z *dt;

				ImuVelocity.x *= friction;
				ImuVelocity.y *= friction;
				ImuVelocity.z *= friction;


				if(sqrt(ImuVelocity.x*ImuVelocity.x + ImuVelocity.y*ImuVelocity.y + ImuVelocity.z*ImuVelocity.z ) > 0)
				{
					// a_scalar = (a * v)/|v|
					scalar_acceleration = ((acceleration.x * ImuVelocity.x) + (acceleration.y * ImuVelocity.y) + (acceleration.z * ImuVelocity.z))
											/sqrt((ImuVelocity.x*ImuVelocity.x) + (ImuVelocity.y*ImuVelocity.y) + (ImuVelocity.z*ImuVelocity.z));
				}


				sprintf(BMIread,"Roll: %.2f, Pitch: %.2f\r\n", orientation_angle.x, orientation_angle.y);
				UART_SendString(USART1, BMIread);


				bmi160_data_ready = 0;
			}
	}
}
