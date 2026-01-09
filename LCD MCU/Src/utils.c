#include "utils.h"
#include <stdio.h>
#include <string.h>
#include "usart.h"


void Debug_Log(const char* format, ...)
{
    char logBuffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(logBuffer, sizeof(logBuffer), format, args);
    va_end(args);

    HAL_UART_Transmit(&huart1, (uint8_t*)logBuffer, strlen(logBuffer), 100);
}

int32_t StringToInt(const char *string)
{
    int32_t result = 0;
    int8_t sign = 1;
    if (*string == '-')
    {
        sign = -1;
        string++;
    }

    while (*string >= '0' && *string <= '9')
    {
        result = result * 10 + (*string++ - '0');
    }

    return result * sign;
}

float StringToFloat(const char *string)
{
    float result = 0.0f;
    float factor = 1.0f;
    int8_t sign = 1;

    if (*string == '-')
    {
        sign = -1;
        string++;
    }

    while (*string >= '0' && *string <= '9')
    {
        result = result * 10.0f + (*string - '0');
        string++;
    }

    if (*string == '.')
    {
        string++;

        while (*string >= '0' && *string <= '9')
        {
            factor *= 0.1f;
            result += (*string - '0') * factor;
            string++;
        }
    }
    return result * sign;
}

void FloatToString(float value, char *buffer, int precision)
{
    if (value < 0)
    {
        *buffer++ = '-';
        value = -value;
    }

    int integerPart = (int)value;
    float fractionalPart = value - (float)integerPart;
    int length = sprintf(buffer, "%d", integerPart);
    buffer += length;

    if (precision > 0)
    {
        *buffer++ = '.';

        while (precision--)
        {
            fractionalPart *= 10.0f;
            int digit = (int)fractionalPart;
            *buffer++ = digit + '0';
            fractionalPart -= (float)digit;
        }
    }
    *buffer = 0;
}
