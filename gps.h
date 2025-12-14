#ifndef GPS_H
#define GPS_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "SysTick.h"
#include "uart.h"
#include "i2c.h"

extern volatile char GPSbuffer[128];

extern volatile uint8_t gps_ready_flag;


void GPS_Init(void);
void UBX_SendCommand(const uint8_t* data, uint16_t len);


void USART2_IRQHandler(void);
float ParseSpeed(void);

#endif
