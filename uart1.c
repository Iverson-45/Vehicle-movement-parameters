#include "uart1.h"

#define GPS_BUF_SIZE 256
volatile char gps_buffer[GPS_BUF_SIZE];
volatile uint16_t gps_write_idx = 0;
volatile uint16_t gps_read_idx = 0;

static char gps_line[128];
static int gps_line_idx = 0;
volatile uint8_t gps_line_ready = 0;



void Uart1_Init(void)
{
    Uart1Pins();

    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    USART1->CR1 = 0;
    USART1->CR2 = 0;
    USART1->CR3 = 0;

    USART1->BRR = 833;
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    while (USART1->ISR & USART_ISR_RXNE)
        (void)USART1->RDR;

    USART1->ICR |= USART_ICR_ORECF;
    USART1->CR1 |= USART_CR1_RXNEIE;

    NVIC_SetPriority(USART1_IRQn, 3);
    NVIC_EnableIRQ(USART1_IRQn);
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


void GPS_SendLineToPC(void)
{
    if (gps_line_ready == 0) {
        return;
    }
 char buffor[16];
    uint8_t comma_count = 0;
    uint8_t buffor_pos = 0;


    memset(buffor, 0, sizeof(buffor));

    const char* ptr = gps_line;

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
            if (buffor_pos < sizeof(buffor) - 1)
            {
                buffor[buffor_pos] = ptr[i];
                buffor_pos++;
            }
        }
    }
    buffor[buffor_pos] = '\0';

    float speed = atof(buffor);

    char velocity[32];
    sprintf(velocity, "Speed: %.2f km/h\r\n", speed);


    UART2_SendString(velocity);


    gps_line_ready = 0;
}
