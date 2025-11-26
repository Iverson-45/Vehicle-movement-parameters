#ifndef BMI160_H_
#define BMI160_H_

#include "i2c.h"
#include "SysTick.h"
#include "uart.h"
#include <stdbool.h>
#include "gpio.h"

#define BMI160_ADDR 0x69

typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
} BMI160_Data;

extern volatile uint8_t bmi160_data_ready;

void BMI160_ReadData(BMI160_Data *d);
void BMI160_Read(uint8_t reg_addr, uint8_t *data, uint8_t length);
void BMI160_Init(void);
void EXTI1_IRQHandler(void);


#endif
