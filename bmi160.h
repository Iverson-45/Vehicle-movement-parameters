#ifndef BMI160_H_
#define BMI160_H_

#include "i2c.h"
#include "SysTick.h"
#include "uart.h"
#include <stdbool.h>

typedef struct {
    float ax, ay, az;
    float gx, gy, gz;
} BMI160_Data;

void BMI160_ReadData(BMI160_Data *d);
void BMI160_Init(void);


#endif
