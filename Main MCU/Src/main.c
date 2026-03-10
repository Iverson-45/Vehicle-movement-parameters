#include "main.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

typedef struct {
    float x, y, z;
} vector;

typedef enum {
    CAL_STATE_WAIT_HANDSHAKE,
    CAL_STATE_WAIT_START_CMD, // NOWY STAN - oczekiwanie na 'k'
    CAL_STATE_GRAVITY_CALC,
    CAL_STATE_WAIT_STEP1_CONFIRM,
    CAL_STATE_MOVEMENT_CALC,
    CAL_STATE_WAIT_STEP2_CONFIRM,
    CAL_STATE_RUNNING
} CalibrationFlow_t;

volatile CalibrationFlow_t current_flow = CAL_STATE_WAIT_HANDSHAKE;
volatile uint8_t lcd_ack_state = 0; // Updated via USART6_IRQHandler

float RotationMatrix[3][3];
const float dt = 0.01f;
const float alpha = 0.02f;

char buffer[64];
char speedbuf[16];
char accbuf[16];
char rollbuf[16];
char pitchbuf[16];

void SystemInit(void)
{
	SCB->CPACR |= (0xF << 20);
}

float VectorLength(vector vec)
{
	return sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
}

void NormalizeVector(vector *vec);
vector VectorCross(vector a, vector b);
vector RotatedVector(float rotation[3][3], vector vec);

int main(void)
{
    SystemClock_Config();
    SysTick_Config(98000000 / 1000000);

    Uart6_Init();        // Communication with LCD
    Uart2_Init();
    Uart1_Init();        // Debug Output
    I2C_Init();
    Init_GPIO_Interrupt();
    GPS_Init();
    BMI160_Init();

    UART_SendString(USART1, "Measurement MCU: Waiting for Handshake...\n\r");

    BMI160_Data imu;
    vector raw_acc, raw_gyro, raw_acc_sum = {0.0f}, gravity_average = {0.0f}, gravity_versor = {0.0f};
    vector linear_accel, body_accel, body_incline;
    uint8_t sample_count = 0;
    float roll = 0.0f, pitch = 0.0f, acceleration;
    int16_t current_speed = 0;

    while (1)
    {
        /* GPS Parsing */
        if (gps_ready_flag)
        {
            current_speed = (int16_t)ParseSpeed();
            sprintf(speedbuf, "v%d\n", current_speed);
            UART_SendString(USART6, speedbuf);
            gps_ready_flag = 0;
        }

        /* Check for Restart Command 'd' at any time to reset flow */
        if (lcd_ack_state == 1 && current_flow == CAL_STATE_RUNNING)
        {
             current_flow = CAL_STATE_WAIT_HANDSHAKE;
        }

        /* IMU and Handshake Logic */
        if (bmi160_data_ready)
        {
            BMI160_ReadData(&imu);
            raw_acc.x = imu.ax; raw_acc.y = imu.ay; raw_acc.z = imu.az;
            raw_gyro.x = imu.gx; raw_gyro.y = imu.gy; raw_gyro.z = imu.gz;

            switch (current_flow)
            {
                case CAL_STATE_WAIT_HANDSHAKE:
                    /* Handshake Dictated by LCD MCU 'd' */
                    if (lcd_ack_state == 1)
                    {
                        lcd_ack_state = 0; // Clear flag

                        UART_SendString(USART1, "Handshake Received. Sending ACK...\n\r");
                        // Send ACK to LCD to confirm connection
                        for(int i=0; i<5; i++) {
                            UART_SendString(USART6, "ack\n");
                            delay_ms(20);
                        }

                        // Wait for actual Start command ('k')
                        current_flow = CAL_STATE_WAIT_START_CMD;
                    }
                    break;

                case CAL_STATE_WAIT_START_CMD:
                    /* Wait for 'k' from LCD (sent after 2s delay) */
                    if (lcd_ack_state == 4) // 4 matches 'k' in ISR
                    {
                        lcd_ack_state = 0;
                        UART_SendString(USART1, "Start Command Received. Calibrating...\n\r");

                        // Reset variables
                        sample_count = 0;
                        raw_acc_sum.x = 0; raw_acc_sum.y = 0; raw_acc_sum.z = 0;

                        current_flow = CAL_STATE_GRAVITY_CALC;
                    }
                    break;

                case CAL_STATE_GRAVITY_CALC:
                    raw_acc_sum.x += raw_acc.x;
                    raw_acc_sum.y += raw_acc.y;
                    raw_acc_sum.z += raw_acc.z;

                    if (++sample_count >= 100)
                    {
                        gravity_average.x = raw_acc_sum.x / sample_count;
                        gravity_average.y = raw_acc_sum.y / sample_count;
                        gravity_average.z = raw_acc_sum.z / sample_count;
                        gravity_versor = gravity_average;
                        NormalizeVector(&gravity_versor);

                        UART_SendString(USART1, "Gravity Locked. Waiting for LCD Step 1 Sync...\n\r");
                        lcd_ack_state = 0;
                        current_flow = CAL_STATE_WAIT_STEP1_CONFIRM;
                    }
                    break;

                case CAL_STATE_WAIT_STEP1_CONFIRM:
                    /* Send status until LCD sends '1' */
                    UART_SendString(USART6, "c1\n");
                    delay_ms(200);
                    if (lcd_ack_state == 2)
                    {
                        UART_SendString(USART1, "Step 1 Confirmed. Waiting for Movement...\n\r");
                        lcd_ack_state = 0;
                        current_flow = CAL_STATE_MOVEMENT_CALC;
                    }
                    break;

                case CAL_STATE_MOVEMENT_CALC:
                    linear_accel.x = raw_acc.x - gravity_average.x;
                    linear_accel.y = raw_acc.y - gravity_average.y;
                    linear_accel.z = raw_acc.z - gravity_average.z;

                    if (VectorLength(linear_accel) > 2.5f)
                    {
                        vector z_axis = gravity_versor;
                        vector movement = linear_accel;
                        NormalizeVector(&movement);
                        vector y_axis = VectorCross(z_axis, movement);
                        NormalizeVector(&y_axis);
                        vector x_axis = VectorCross(y_axis, z_axis);
                        NormalizeVector(&x_axis);

                        RotationMatrix[0][0] = x_axis.x; RotationMatrix[0][1] = x_axis.y; RotationMatrix[0][2] = x_axis.z;
                        RotationMatrix[1][0] = y_axis.x; RotationMatrix[1][1] = y_axis.y; RotationMatrix[1][2] = y_axis.z;
                        RotationMatrix[2][0] = z_axis.x; RotationMatrix[2][1] = z_axis.y; RotationMatrix[2][2] = z_axis.z;

                        UART_SendString(USART1, "Movement Locked. Waiting for LCD Step 2 Sync...\n\r");
                        lcd_ack_state = 0;
                        current_flow = CAL_STATE_WAIT_STEP2_CONFIRM;
                    }
                    break;

                case CAL_STATE_WAIT_STEP2_CONFIRM:
                    /* Send status until LCD sends '2' */
                    UART_SendString(USART6, "c2\n");
                    delay_ms(200);
                    if (lcd_ack_state == 3)
                    {
                        UART_SendString(USART1, "Step 2 Confirmed. Running Telemetry.\n\r");
                        lcd_ack_state = 0;
                        current_flow = CAL_STATE_RUNNING;
                    }
                    break;

                case CAL_STATE_RUNNING:
                    body_accel = RotatedVector(RotationMatrix, raw_acc);
                    body_incline = RotatedVector(RotationMatrix, raw_gyro);

                    float acc_roll = atan2f(body_accel.y, body_accel.z) * 57.296f;
                    float acc_pitch = atan2f(-body_accel.x, sqrtf(body_accel.y*body_accel.y + body_accel.z*body_accel.z)) * 57.296f;


                    if(fabs(body_accel.x) >0.8f)
                    {
                    	roll  = roll  + body_incline.x * dt;
						pitch = pitch + body_incline.y * dt;
                    }

                    else
                    {
                    	roll  = (1.0f - alpha) * (roll  + body_incline.x * dt) + (alpha * acc_roll);
                    	pitch = (1.0f - alpha) * (pitch + body_incline.y * dt) + (alpha * acc_pitch);
                    }

                    acceleration = body_accel.x + (9.81f * sinf(pitch * 3.14159f / 180.0f));

                    if (fabsf(acceleration) <= 0.15f) acceleration = 0.0f;

                    /* Data packet transmission */
                    sprintf(accbuf, "a%.1f\n", acceleration); UART_SendString(USART6, accbuf);
                    sprintf(rollbuf, "r%d\n", (int8_t)roll);  UART_SendString(USART6, rollbuf);
                    sprintf(pitchbuf, "p%d\n", (int8_t)pitch); UART_SendString(USART6, pitchbuf);
                    break;
            }
            bmi160_data_ready = 0;
        }
    }
}

void USART6_IRQHandler(void)
{
    if (USART6->SR & USART_SR_RXNE)
    {
        char rx_data = USART6->DR;
        if (rx_data == 'd') lcd_ack_state = 1; // Connect
        if (rx_data == '1') lcd_ack_state = 2; // Step 1 Ack
        if (rx_data == '2') lcd_ack_state = 3; // Step 2 Ack
        if (rx_data == 'k') lcd_ack_state = 4; // Start Calibration
    }
}

void NormalizeVector(vector *vec)
{
    float len = VectorLength(*vec);
    if (len > 0.0001f) {
        vec->x /= len; vec->y /= len; vec->z /= len;
    }
}

vector VectorCross(vector a, vector b)
{
    vector res;
    res.x = a.y * b.z - a.z * b.y;
    res.y = a.z * b.x - a.x * b.z;
    res.z = a.x * b.y - a.y * b.x;
    return res;
}

vector RotatedVector(float rotation[3][3], vector vec)
{
    vector res;
    res.x = rotation[0][0] * vec.x + rotation[0][1] * vec.y + rotation[0][2] * vec.z;
    res.y = rotation[1][0] * vec.x + rotation[1][1] * vec.y + rotation[1][2] * vec.z;
    res.z = rotation[2][0] * vec.x + rotation[2][1] * vec.y + rotation[2][2] * vec.z;
    return res;
}
