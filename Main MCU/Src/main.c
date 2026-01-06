#include "main.h"

void SystemInit(void)
{
	//FPU on
	SCB->CPACR |= (0xF << 20);
}

typedef struct {
	float x, y, z;
}vector;



float VectorLength(vector vec)
{
	return sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}


void NormalizeVector(vector *vec)
{
	float length = VectorLength(*vec);

	if(length > 0.0001f)
	{
		vec->x /=length;
		vec->y /=length;
		vec->z /=length;
	}
}


vector VectorCross(vector a, vector b)
{
    vector out;
    out.x = a.y * b.z - a.z * b.y;
    out.y = a.z * b.x - a.x * b.z;
    out.z = a.x * b.y - a.y * b.x;

    return out;
}

vector RotatedVector (float rotation[3][3], vector vec)
{
	vector out;

	out.x = rotation[0][0] * vec.x + rotation[0][1] * vec.y + rotation[0][2] * vec.z; // przód tył
	out.y = rotation[1][0] * vec.x + rotation[1][1] * vec.y + rotation[1][2] * vec.z; // prawo lewo
	out.z = rotation[2][0] * vec.x + rotation[2][1] * vec.y + rotation[2][2] * vec.z; // dół góra

	return out;
}


float RotationMatrix[3][3];

const float dt = 0.01f;
const float alpha = 0.8f;

uint8_t calibration = 1;

volatile uint8_t lcd_ack_state = 0;
// 0 = Brak połączenia
// 1 = Handshake OK
// 2 = Step 1 OK
// 3 = Step 2 OK

int main(void)
{
	SystemClock_Config();
	SysTick_Config(98000000 / 1000000);

    Uart6_Init();
	Uart2_Init();
	Uart1_Init();
	I2C_Init();
	Init_GPIO_Interrupt();
	GPS_Init();
	BMI160_Init();

	UART_SendString(USART1, "Start. Waiting for LCD...\n\r");
	delay_ms(200);

	while(lcd_ack_state == 0);

	UART_SendString(USART1, "LCD Found! Sending ACK...\n\r");

	for(int i=0; i<5; i++)
	{
		UART_SendString(USART6, "ack\n");
		delay_ms(20);
	}

	delay_ms(1000);

    BMI160_Data imu;
    vector raw_acc = {0.0f};
	vector raw_gyro = {0.0f};

    vector raw_acc_sum = {0.0f};
    uint8_t N = 0;
    vector gravity_average = {0.0f};

    vector gravity_versor = {0.0f};
    vector linear_acceleration = {0.0f};

    vector body_acceleration = {0.0f};
    vector body_inclination = {0.0f};

    float acc_roll = 0.0f;
	float acc_pitch = 0.0f;
	float roll = 0.0f;
	float pitch = 0.0f;
	float acceleration;

    char buffer[64];

    char speedbuf[16];
    char accbuf[16];
    char rollbuf[16];
    char pitchbuf[16];

    int16_t speed =0;

    while(1)
    	{
			if (gps_ready_flag)
			{
				speed = (int16_t)ParseSpeed();

				sprintf(speedbuf, "v%d\n", speed);
				UART_SendString(USART6, (const char*)speedbuf);

				gps_ready_flag = 0;
			}


			if (bmi160_data_ready)
			{
				BMI160_ReadData(&imu);
				raw_acc.x = imu.ax; raw_acc.y = imu.ay; raw_acc.z = imu.az;
				raw_gyro.x = imu.gx; raw_gyro.y = imu.gy; raw_gyro.z = imu.gz;


				if(calibration == 1)
				{
					raw_acc_sum.x += raw_acc.x;
					raw_acc_sum.y += raw_acc.y;
					raw_acc_sum.z += raw_acc.z;

					N++;

					if(N >= 100)
					{
						gravity_average.x = raw_acc_sum.x / N;
						gravity_average.y = raw_acc_sum.y / N;
						gravity_average.z = raw_acc_sum.z / N;

						gravity_versor = gravity_average;
						NormalizeVector(&gravity_versor);

						UART_SendString(USART1, "Gravity OK. Syncing Step 1...\n\r");

						delay_ms(2000);


						while(lcd_ack_state != 2)
						{
							UART_SendString(USART6, "c1\n");
							delay_ms(200);
						}

						UART_SendString(USART1, "LCD Confirmed Step 1!\n\r");
						calibration = 2;
					}
				}

				if (calibration == 2)
				{
					linear_acceleration.x = raw_acc.x - gravity_average.x;
					linear_acceleration.y = raw_acc.y - gravity_average.y;
					linear_acceleration.z = raw_acc.z - gravity_average.z;

					if ((VectorLength(linear_acceleration) > 1.0f))
					{
						vector z_axis = gravity_versor;

						vector movement = linear_acceleration;
						NormalizeVector(&movement);

						vector y_axis = VectorCross(z_axis, movement);
						NormalizeVector(&y_axis);

						vector x_axis = VectorCross(y_axis, z_axis);
						NormalizeVector(&x_axis);

						RotationMatrix[0][0] = x_axis.x; RotationMatrix[0][1] = x_axis.y; RotationMatrix[0][2] = x_axis.z;
						RotationMatrix[1][0] = y_axis.x; RotationMatrix[1][1] = y_axis.y; RotationMatrix[1][2] = y_axis.z;
						RotationMatrix[2][0] = z_axis.x; RotationMatrix[2][1] = z_axis.y; RotationMatrix[2][2] = z_axis.z;

						UART_SendString(USART1, "Move OK. Syncing Step 2...\n\r");

						while(lcd_ack_state != 3)
						{
							UART_SendString(USART6, "c2\n");
							delay_ms(200);
						}

						UART_SendString(USART1, "LCD Confirmed Step 2! Running.\n\r");
						calibration = 0;
					}
				}

				if (calibration == 0)
				{
					body_acceleration = RotatedVector(RotationMatrix, raw_acc);
					body_inclination = RotatedVector(RotationMatrix, raw_gyro);

					acc_roll = atan2f(body_acceleration.y, body_acceleration.z) * 57.296f;
					acc_pitch = atan2f(-body_acceleration.x, sqrtf(body_acceleration.y*body_acceleration.y + body_acceleration.z*body_acceleration.z)) * 57.296f;

					roll  = alpha  * acc_roll + (1.0f-alpha) * (roll + body_inclination.x *dt);
					pitch = alpha * acc_pitch + (1.0f-alpha) * (pitch + body_inclination.y *dt);

					if (fabsf(roll) < 1) roll =0.0f;
					if (fabsf(pitch) < 1) pitch =0.0f;

					acceleration = body_acceleration.x + (9.81f * sinf(pitch * 3.14159f / 180.0f));



					if(fabsf(acceleration) <= 0.1f) acceleration = 0.0f;

						sprintf(buffer, "Roll: %d, Pitch: %d, Acc: %.1f [m/s^2]\r\n", (int8_t)roll, (int8_t)pitch, acceleration);
						UART_SendString(USART1, buffer);

						sprintf(accbuf, "a%.1f\n", acceleration);
						UART_SendString(USART6, (const char*)accbuf);

						sprintf(rollbuf, "r%d\n", (int8_t)roll);
						UART_SendString(USART6, (const char*)rollbuf);

						sprintf(pitchbuf, "p%d\n", (int8_t)pitch);
						UART_SendString(USART6, (const char*)pitchbuf);
				}
			}
    	}
}

void USART6_IRQHandler(void)
{
    if (USART6->SR & USART_SR_RXNE)
    {
        char rx = USART6->DR;

        if (rx == 'd') lcd_ack_state = 1;
        if (rx == '1') lcd_ack_state = 2;
        if (rx == '2') lcd_ack_state = 3;
    }
}
