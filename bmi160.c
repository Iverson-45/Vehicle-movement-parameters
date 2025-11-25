#include "bmi160.h"

#define BMI160_ADDR 0x69
BMI160_Data bmi160_data;

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
	BMI160_Write(0x7E, 0x11); // accel normal mode
	BMI160_Write(0x7E, 0x15); // gyro normal mode
	delay_ms(1);

	// gyroscope
	BMI160_Write(0x42, 0x28); // ODR 100Hz, normal filter
	BMI160_Write(0x43, 0x00); // ±2000°/s
	delay_ms(1);

	// accelerometer
	BMI160_Write(0x40, 0x28); // ODR 100Hz, normal filter
	BMI160_Write(0x41, 0x0C); // ±16g
	delay_ms(1);
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
