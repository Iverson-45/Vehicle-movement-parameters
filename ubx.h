#ifndef UBX_H
#define UBX_H

#include "main.h"
#include <stdint.h>
#include "Systick.h"
#include "stm32f3xx.h"
// ------------------- UBX Functions -------------------

void UBX_Init(void);
void UBX_SendCommand(const uint8_t* data, uint16_t len);

#endif
