#include "main.h"

void SystemInit(void)
{
	//FPU on
	SCB->CPACR |= (0xF << 20);
}

typedef struct {
	float x, y, z;
}vector;

uint8_t calibration = 1;
uint16_t calibration_count = 0;
const uint16_t calibration_samples = 0.005f;

float cos_roll = 1.0f, sin_roll = 0.0f;
float cos_pitch = 1.0f, sin_pitch = 0.0f;

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

    char buffer[64];
    char LCDspeed[32];

    const float alpha = 0.96f;
    const float alpha_g = 0.8f;
    const float dt = 0.01f;
    const float friction = 0.95f;
    const float RadToDeg = 57.29578f;
    const float DegToRad = 0.0174533f;

    BMI160_Data imu = {0,0,0};

    vector gyro_bias = {0,0,0}; //Błąd spoczynkowy żyroskopu
    vector current_angle = {0,0,0}; //Aktaulny wylicvzony kąt
    vector start_offset = {0,0,0}; //Kąt początkowy
    vector angle = {0,0,0}; //Kąt

    vector gravity = {0,0,0};
    vector acceleration = {0,0,0};
    vector imu_velocity = {0,0,0};

    float scalar_acceleration = 0;

    float gx=0, gy=0, gz=0;

    float acc_roll=0;
    float acc_pitch=0;

    float virt_ax, virt_ay, virt_az;
	float virt_gx, virt_gy, virt_gz;

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

				if(calibration)
				{
					gyro_bias.x += imu.gx;
					gyro_bias.y += imu.gy;
					gyro_bias.z += imu.gz;

					gravity.x += imu.ax;
					gravity.y += imu.ay;
					gravity.z += imu.az;

					calibration_count ++;

					if(calibration_count >= 200)
					{
						gyro_bias.x *= calibration_samples;
						gyro_bias.y *= calibration_samples;
						gyro_bias.z *= calibration_samples;

						gravity.x *= calibration_samples;
						gravity.y *= calibration_samples;
						gravity.z *= calibration_samples;

						float start_roll = atan2f(gravity.y, gravity.z);
						float start_pitch = atan2f(-gravity.x, sqrtf(gravity.y*gravity.y + gravity.z*gravity.z));

						cos_roll = cosf(start_roll);
						sin_roll = sinf(start_roll);
						cos_pitch = cosf(start_pitch);
						sin_pitch = sinf(start_pitch);

						gravity.x = 0;
						gravity.y = 0;
						gravity.z = sqrtf(gravity.x*gravity.x + gravity.y*gravity.y + gravity.z*gravity.z); // ok. 1g (lub LSB)

						// Zerujemy aktualny kąt - startujemy od zera
						current_angle.x = 0;
						current_angle.y = 0;
						current_angle.z = 0;

						calibration = 0;
					}
					bmi160_data_ready = 0;
					continue;
				}

				gx = imu.gx - gyro_bias.x;
				gy = imu.gy - gyro_bias.y;
				gz = imu.gz - gyro_bias.z;

				if (fabsf(gx) < 1.f) gx = 0.0f;
				if (fabsf(gy) < 1.f) gy = 0.0f;
				if (fabsf(gz) < 1.f) gz = 0.0f;

				acc_roll  = atan2f(imu.ay, imu.az) * RadToDeg;
				acc_pitch = atan2f(-imu.ax, sqrtf(imu.ay*imu.ay + imu.az*imu.az)) * RadToDeg;

				current_angle.x = alpha * (current_angle.x + gx * dt) + (1.0f - alpha) * acc_roll;
				current_angle.y = alpha * (current_angle.y + gy * dt) + (1.0f - alpha) * acc_pitch;
				current_angle.z += gz * dt;

				angle.x = current_angle.x - start_offset.x;
				angle.y = current_angle.y - start_offset.y;
				angle.z = current_angle.z;

				gravity.x = alpha_g * gravity.x + (1.0f - alpha_g) * imu.ax;
				gravity.y = alpha_g * gravity.y + (1.0f - alpha_g) * imu.ay;
				gravity.z = alpha_g * gravity.z + (1.0f - alpha_g) * imu.az;

				acceleration.x = imu.ax - gravity.x;
				acceleration.y = imu.ay - gravity.y;
				acceleration.z = imu.az - gravity.z;

				if (fabsf(acceleration.x) < 0.15f) acceleration.x = 0.0f;
				if (fabsf(acceleration.y) < 0.15f) acceleration.y = 0.0f;
				if (fabsf(acceleration.z) < 0.15f) acceleration.z = 0.0f;

				imu_velocity.x += acceleration.x * dt;
				imu_velocity.y += acceleration.y * dt;
				imu_velocity.z += acceleration.z * dt;

				imu_velocity.x *= friction;
				imu_velocity.y *= friction;
				imu_velocity.z *= friction;

				float v_sq = imu_velocity.x*imu_velocity.x + imu_velocity.y*imu_velocity.y + imu_velocity.z*imu_velocity.z;
				if(v_sq > 0.001f)
				{
					scalar_acceleration = (acceleration.x * imu_velocity.x + acceleration.y * imu_velocity.y + acceleration.z * imu_velocity.z) / sqrtf(v_sq);
				}
				else
				{
					scalar_acceleration = 0;
				}

				sprintf(buffer, "dRoll: %.2f, dPitch: %.2f, dYaw: %.2f | Acc: %.2f\r\n",
						angle.x, angle.y, angle.z, scalar_acceleration);
				UART_SendString(USART1, buffer);

				bmi160_data_ready = 0;

			}
	}
}



































