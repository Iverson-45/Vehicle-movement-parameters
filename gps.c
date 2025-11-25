#include "gps.h"


#define GPS_BUF_SIZE 256

static volatile char gps_buffer[GPS_BUF_SIZE];
static volatile uint16_t gps_write_idx = 0;
static volatile uint16_t gps_read_idx = 0;

static char gps_line[128];
static int gps_line_idx = 0;
volatile uint8_t gps_line_ready = 0;

float V = 0;


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
    USART1->ICR |= USART_ICR_ORECF;
    USART1->CR1 |= USART_CR1_RXNEIE;

    NVIC_SetPriority(USART1_IRQn, 3);
    NVIC_EnableIRQ(USART1_IRQn);

    delay_ms(500);

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
    if (USART1->ISR & USART_ISR_ORE)
        USART1->ICR |= USART_ICR_ORECF;

    if (USART1->ISR & USART_ISR_RXNE)
    {
        char c = USART1->RDR;

        uint16_t next = (gps_write_idx + 1) % GPS_BUF_SIZE;
        if (next != gps_read_idx)
        {
            gps_buffer[gps_write_idx] = c;
            gps_write_idx = next;
        }
    }
}

static int GPS_DataAvailable(void)
{
    return (gps_write_idx != gps_read_idx);
}

static char GPS_ReadChar(void)
{
    char c = gps_buffer[gps_read_idx];
    gps_read_idx = (gps_read_idx + 1) % GPS_BUF_SIZE;
    return c;
}

void GPS_ProcessData(void)
{
    while (GPS_DataAvailable())
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
            if (gps_line_idx < 127)
                gps_line[gps_line_idx++] = c;
        }
    }
}

uint8_t GPS_IsLineReady(void)
{
    return gps_line_ready;
}

const char* GPS_GetLine(void)
{
    return gps_line;
}

void GPS_ClearLine(void)
{
    gps_line_ready = 0;
}


void GpsSendToPC(void)
{
    if (!GPS_IsLineReady()) {
        return;
    }

    const char* line = GPS_GetLine();

    UART2_SendString(line);
    UART2_SendString("\r\n");
}

void GpsDisplayLCD(void)
{
    if (!GPS_IsLineReady()) {
        return;
    }

    const char* line = GPS_GetLine();

    char buffor[16];
    uint8_t comma_count = 0;
    uint8_t buffor_pos = 0;

    memset(buffor, 0, sizeof(buffor));

    const char* ptr = line;

    for(int i = 0; ptr[i] != '\0'; i++)
    {
        if(ptr[i] == ',')
        {
            comma_count++;
            if (comma_count > 7) break;
            continue;
        }

        if(comma_count == 7)
        {
            if (ptr[i] != '*')
            {
                if (buffor_pos < sizeof(buffor) - 1)
                {
                    buffor[buffor_pos] = ptr[i];
                    buffor_pos++;
                }
            }
        }
    }
    buffor[buffor_pos] = '\0';

    V = atof(buffor);
    uint8_t speed = (uint8_t)V;

    ST7567_Clear();
    ST7567_SetCursor(0, 32);

    char velocity[32];
    sprintf(velocity, "Speed: %d km/h", speed);
    ST7567_Puts(velocity);

    ST7567_Flush();
    GPS_ClearLine();
}
