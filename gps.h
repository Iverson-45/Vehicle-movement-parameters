#ifndef GPS_H
#define GPS_H

#include "stm32f3xx.h"
#include <stdint.h>
#include "SysTick.h"
#include "uart.h"
#include "st7567s.h"
#include "i2c.h"

extern volatile uint8_t gps_line_ready;


void GPS_Init(void);
void UBX_SendCommand(const uint8_t* data, uint16_t len);


void USART1_EXTI25_IRQHandler(void);
void GPS_ProcessData(void);
void GpsSendToPC(void);
void GpsDisplayLCD(void);

#endif
