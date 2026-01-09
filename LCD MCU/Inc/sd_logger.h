#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include "main.h"

typedef struct {
    uint32_t timestamp;
    int16_t  speed;
    float    accel;
    int16_t  pitch;
    int16_t  roll;
} TelemetryData_t;

typedef enum {
    SD_NOT_PRESENT,
    SD_READY,
    SD_WRITING,
    SD_ERROR
} SD_Status_t;

void        Logger_Init(void);
uint8_t     Logger_StartNewFile(void);
void        Logger_StopRecording(void);
void        Logger_AddData(TelemetryData_t *data);
void        Logger_FlushBuffer(void);
SD_Status_t Logger_GetStatus(void);
uint8_t     Logger_IsFileOpen(void);
void        Logger_EjectCard(void);

#endif
