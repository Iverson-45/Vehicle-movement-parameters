#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stdarg.h>

int32_t StringToInt(const char *string);
float   StringToFloat(const char *string);
void    FloatToString(float value, char *buffer, int precision);
void    Debug_Log(const char* format, ...);

#endif
