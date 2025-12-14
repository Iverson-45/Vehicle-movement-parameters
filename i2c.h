#ifndef I2C_H
#define I2C_H

#include "stm32f4xx.h"
#include "gpio.h"

void I2C_Init(void);
void I2C_ReadRegister(uint8_t slave_addr, uint8_t reg_addr, uint8_t *buffer, uint8_t len);
void I2C_WriteRegister(uint8_t addr, uint8_t control, const uint8_t* data, uint16_t len);

#endif
