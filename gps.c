#include "gps.h"


volatile char buffer[128];

volatile uint8_t gps_ready_flag = 0;


const uint8_t UBX_RATE_5HZ[] = {
    0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xC8, 0x00, 0x01, 0x00, 0x01, 0x00, 0xDE, 0x6A
};

const uint8_t UBX_NAV5_AUTOMOTIVE[] = {
    0xB5, 0x62, 0x06, 0x24, 0x24, 0x00,
    0xFF, 0xFF, 0x04, 0x03, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x27, 0x00, 0x00,
    0x05, 0x00, 0xFA, 0x00, 0xFA, 0x00,
    0x64, 0x00, 0x2C, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x16, 0xDC
};

const uint8_t UBX_MSG_DISABLE_GGA[] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x00, 0x00, 0xFA, 0x0F};
const uint8_t UBX_MSG_DISABLE_GLL[] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x01, 0x00, 0xFB, 0x11};
const uint8_t UBX_MSG_DISABLE_GSA[] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x02, 0x00, 0xFC, 0x13};
const uint8_t UBX_MSG_DISABLE_GSV[] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x03, 0x00, 0xFD, 0x15};
const uint8_t UBX_MSG_DISABLE_RMC[] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x04, 0x00, 0xFE, 0x17};
const uint8_t UBX_MSG_ENABLE_VTG[] = {0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x05, 0x01, 0xFF, 0x19};


void UBX_SendCommand(const uint8_t *data, uint16_t len)
{
    for(uint16_t i=0; i<len; i++)
    {
        UART1_SendByte(data[i]);
    }
}

void GPS_Init(void)
{
	UBX_SendCommand(UBX_NAV5_AUTOMOTIVE, sizeof(UBX_NAV5_AUTOMOTIVE));
    delay_ms(100);

    UBX_SendCommand(UBX_RATE_5HZ, sizeof(UBX_RATE_5HZ));
    delay_ms(100);

    UBX_SendCommand(UBX_MSG_DISABLE_GGA, sizeof(UBX_MSG_DISABLE_GGA));
    delay_ms(50);
    UBX_SendCommand(UBX_MSG_DISABLE_GLL, sizeof(UBX_MSG_DISABLE_GLL));
    delay_ms(50);
    UBX_SendCommand(UBX_MSG_DISABLE_GSA, sizeof(UBX_MSG_DISABLE_GSA));
    delay_ms(50);
    UBX_SendCommand(UBX_MSG_DISABLE_GSV, sizeof(UBX_MSG_DISABLE_GSV));
    delay_ms(50);
    UBX_SendCommand(UBX_MSG_DISABLE_RMC, sizeof(UBX_MSG_DISABLE_RMC));
    delay_ms(50);

    UBX_SendCommand(UBX_MSG_ENABLE_VTG, sizeof(UBX_MSG_ENABLE_VTG));
}

void USART1_EXTI25_IRQHandler(void)
{
	static uint8_t index =0;

	if (USART1->ISR & USART_ISR_RXNE)
	{
		if(USART1->RDR == '\n')
		{
			buffer[index] = '\0';

			gps_ready_flag = 1;

			index = 0;
		}

		else
		{
			buffer[index] = USART1->RDR;

			index++;
		}
	}
}


float ParseSpeed(void)
{
	uint8_t CommaCounter = 0;

	char *p = buffer;

	while(*p != '\0' && CommaCounter < 7)
	{
		if(*p == ',')
		{
			CommaCounter++;
		}

		p++;
	}

	char SpeedBuf[8];

	char *q = SpeedBuf;

	while(*p !=',')
	{
		*q = *p++;

		q++;
	}

	*q = '\0';

	return atof(SpeedBuf);
}



















