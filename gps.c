#include "gps.h"


#define GPS_BUF_SIZE 256

static volatile char gps_buffer[GPS_BUF_SIZE];
static volatile uint16_t gps_write_idx = 0;
static volatile uint16_t gps_read_idx = 0;

static char gps_line[128];
static int gps_line_idx = 0;
volatile uint8_t gps_line_ready = 0;

static uint8_t last_speed = 255;


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
	uint32_t isr = USART1->ISR;

	if (isr & USART_ISR_RXNE)
	{
		uint8_t rx_data = (uint8_t)USART1->RDR;

		gps_buffer[gps_write_idx++] = rx_data;
		if (gps_write_idx >= GPS_BUF_SIZE)
		{
				gps_write_idx = 0;
		}

	}

	if (isr & USART_ISR_ORE)
	{
	USART1->ICR |= USART_ICR_ORECF;
	}
}

static char GPS_ReadChar(void)
{
    char c = gps_buffer[gps_read_idx];
    gps_read_idx = (gps_read_idx + 1) % GPS_BUF_SIZE;
    return c;
}

void GPS_ProcessData(void)
{
    while (gps_write_idx != gps_read_idx)
    {
        char c = GPS_ReadChar();

        if (c == '\n')
        {
            gps_line[gps_line_idx] = '\0';
            gps_line_idx = 0;
            gps_line_ready = 1;
        }
        else if (c != '\r')
        {
            if (gps_line_idx < sizeof(gps_line)-1)
                gps_line[gps_line_idx++] = c;
        }
    }
}

void GpsSendToPC(void)
{
    if (!gps_line_ready) {
        return;
    }

    const char* line = gps_line;

    UART2_SendString(line);
    UART2_SendString("\r\n");
}

static uint8_t parse_speed_from_line(const char* line)
{
    uint8_t comma_count = 0;
    uint8_t speed = 0;

    while(*line != '\0')
    {
        if(*line == ',')
        {
        	comma_count++;
        }

		else if(comma_count == 7 && *line >= '0' && *line <= '9')
        {
            speed = speed*10 + (*line - '0');
        }

        else if(comma_count > 7)
            break;

        line++;
    }

    return speed;
}

void GpsDisplayLCD(void)
{
    if (!gps_line_ready) return;

    uint8_t speed = parse_speed_from_line(gps_line);

    if(speed != last_speed)
    {
        last_speed = speed;

        char buffor[16];

        sprintf(buffor, "Speed: %d Km/h", speed);

        ST7567_SetCursor(4, 32);
        ST7567_Puts(buffor);
        ST7567_Flush();
    }

    gps_line_ready = 0;
}
