#include "bmi160.h"

BMI160_Data bmi160_data;

volatile uint8_t bmi160_data_ready = 0;

void BMI160_Read(uint8_t reg_addr, uint8_t *data, uint8_t length)
{
	I2C_ReadRegister(BMI160_ADDR, reg_addr, data, length);
}

void BMI160_Write(uint8_t reg_addr, uint8_t data)
{
	I2C_WriteRegister(BMI160_ADDR, reg_addr, &data, 1);
}

void BMI160_Init(void)
{
	BMI160_Write(0x7E, 0xB6); //Soft reset
	delay_ms(50);

    BMI160_Write(0x7E, 0x11); //Accel Normal mode
    delay_ms(4);

    BMI160_Write(0x7E, 0x15); //Gyro Normal mode
    delay_ms(80);

    BMI160_Write(0x40, 0x28); //Accel 0x28 ODR=100Hz, 0x26 ODR=25Hz
    delay_us(500);

    BMI160_Write(0x41, 0x0C); //Accel range=+-16g
    delay_us(2);

    BMI160_Write(0x42, 0x28); //Gyro 0x28 ODR=100Hz, 0x26 ODR=25Hz
    delay_us(2);

	BMI160_Write(0x43, 0x00); //Gyro range =+-2000*
	delay_us(2);

	BMI160_Write(0x51, 0x10); // Turn on interrupts
	delay_us(2);

	BMI160_Write(0x56, 0x80); // Data ready to int1 pin
	delay_us(2);

	BMI160_Write(0x53, 0x0A); // INT1 config: output enable, push-pull, active high
	delay_us(2);
}


void BMI160_ReadData(BMI160_Data *d)
{
	uint8_t buffer[6];

	// gyroscope
	BMI160_Read(0x0C, buffer, 6);
	d->gx = ((int16_t)((buffer[1]<<8)|buffer[0]) * 2000.0f / 32768.0f);
	d->gy = ((int16_t)((buffer[3]<<8)|buffer[2]) * 2000.0f / 32768.0f);
	d->gz = ((int16_t)((buffer[5]<<8)|buffer[4]) * 2000.0f / 32768.0f);

	// accelerometer
	BMI160_Read(0x12, buffer, 6);
	d->ax = (int16_t)((buffer[1]<<8)|buffer[0]) * 16.0f * 9.80665f / 32768.0f;
	d->ay = (int16_t)((buffer[3]<<8)|buffer[2]) * 16.0f * 9.80665f / 32768.0f;
	d->az = (int16_t)((buffer[5]<<8)|buffer[4]) * 16.0f * 9.80665f / 32768.0f;
}

void EXTI2_IRQHandler(void)
{
    if (EXTI->PR & (1 << 2))
    {
        EXTI->PR |=(1 << 2);

        bmi160_data_ready = 1;
    }
}






