#include "main.h"

typedef struct {
	float x, y, z;
}vector;

const float alpha = 0.8;

const float dt = 0.01;

const float friction = 0.95;

uint8_t first_reading = 1;

int main(void)
{
	//FPU on
	SCB->CPACR |= (0xF << 20);

	SystemClock_Config_100MHz();

    SysTick_Config(100000000 / 1000000);

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

    vector gravity;

    vector acceleration;

    vector ImuVelocity;

    vector degrees;

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

				if(first_reading)
				{
					gravity.x = imu.ax;
					gravity.y = imu.ay;
					gravity.z = imu.az;

					degrees.x = imu.gx*dt;
					degrees.y = imu.gy*dt;
					degrees.z = imu.gz*dt;

					first_reading = 0;
				}

				//Low pass filter 1 - alpha
				gravity.x = alpha * gravity.x + (1.0f - alpha) * imu.ax;
				gravity.y = alpha * gravity.y + (1.0f - alpha) * imu.ay;
				gravity.z = alpha * gravity.z + (1.0f - alpha) * imu.az;

				acceleration.x = imu.ax - gravity.x;
				acceleration.y = imu.ay - gravity.y;
				acceleration.z = imu.az - gravity.z;

				//Ignoring dead zone
				if (fabsf(acceleration.x) < 0.2f)
				{
				    acceleration.x = 0.0f;
				}

				if (fabsf(acceleration.y) < 0.2f)
				{
				    acceleration.y = 0.0f;
				}

				if (fabsf(acceleration.z) < 0.2f)
				{
				    acceleration.z = 0.0f;
				}

				//Integrating acceleration to get velocity
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

				if(fabsf(imu.gx) >=2)
				{
					degrees.x += imu.gx*dt;
				}

				if(fabsf(imu.gy) >=1)
				{
					degrees.y += imu.gy*dt;
				}

				if(fabsf(imu.gz) >=1)
				{
					degrees.z += imu.gz*dt;
				}


				//sprintf(BMIread, "Acceleration: %.2f [m/s^2] \r\n",scalar_acceleration);
				sprintf(BMIread,"gx: %.2f, gy: %.2f, gz: %.2f,  ax: %.2f, ay:%.2f, az: %.2f\r\n", degrees.x, degrees.y, degrees.z, acceleration.x, acceleration.y, acceleration.z);
				UART_SendString(USART1, BMIread);




				bmi160_data_ready = 0;
			}
	}
}



































