#include "sd_logger.h"
#include "fatfs.h"
#include "utils.h"
#include "input_handler.h"
#include <string.h>
#include <stdio.h>

static FATFS fileSystem;
static FIL logFile;
static char dataBuffer[128];
static uint16_t bufferIndex = 0;

static uint32_t recordingStartTime = 0;
static SD_Status_t sdCardStatus = SD_NOT_PRESENT;
static uint8_t isFileOpen = 0;
static uint8_t isCardEjected = 0;
static uint32_t lastConnectionCheckTime = 0;

uint8_t Logger_IsFileOpen(void)
{
    return isFileOpen;
}

void Logger_Init(void)
{
    sdCardStatus = SD_NOT_PRESENT;
    isFileOpen = 0;
    isCardEjected = 0;
    bufferIndex = 0;

    HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}

void Logger_CheckConnection(void)
{
    if (isCardEjected || isFileOpen) return;

    if (HAL_GetTick() - lastConnectionCheckTime < 3000) return;
    lastConnectionCheckTime = HAL_GetTick();

    if (f_mount(&fileSystem, "", 0) == FR_OK)
    {
        sdCardStatus = SD_READY;
    }
    else
    {
        sdCardStatus = SD_ERROR;
    }
}

uint8_t Logger_StartNewFile(void)
{
    if (isCardEjected || isFileOpen) return 0;

    if (sdCardStatus != SD_READY)
    {
        if(f_mount(&fileSystem, "", 1) == FR_OK) sdCardStatus = SD_READY;
        else return 0;
    }

    recordingStartTime = 0;
    char fileName[12];

    for (int i = 0; i < 99; i++)
    {
        sprintf(fileName, "Log%02d.CSV", i);
        if (f_open(&logFile, fileName, FA_READ) != FR_OK) break;
        f_close(&logFile);
    }

    if (f_open(&logFile, fileName, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
    {
        isFileOpen = 1;
        sdCardStatus = SD_WRITING;
        f_puts("Time_ms,Speed_km/h,Accel_m/s2,Power_HP,Pitch_deg,Roll_deg\n", &logFile);
        f_sync(&logFile);
        bufferIndex = 0;
        return 1;
    }

    sdCardStatus = SD_ERROR;
    return 0;
}

void Logger_AddData(TelemetryData_t *data)
{
    if (!isFileOpen) return;

    if (recordingStartTime == 0) recordingStartTime = HAL_GetTick();

    float powerHP = ((float)Input_GetVehicleMass() * data->accel * data->speed) / 2684.5f;
    if (powerHP < 0) powerHP = 0;

    uint32_t relativeTime = HAL_GetTick() - recordingStartTime;

    char lineBuffer[64];
    char accelStr[8], powerStr[8];
    FloatToString(data->accel, accelStr, 2);
    FloatToString(powerHP, powerStr, 1);

    int length = sprintf(lineBuffer, "%lu,%d,%s,%s,%d,%d\n",
                         relativeTime, data->speed, accelStr, powerStr, data->pitch, data->roll);

    if (bufferIndex + length >= sizeof(dataBuffer))
    {
        Logger_FlushBuffer();
    }

    if (isFileOpen)
    {
        memcpy(&dataBuffer[bufferIndex], lineBuffer, length);
        bufferIndex += length;
    }
}

void Logger_FlushBuffer(void)
{
    if (isFileOpen && bufferIndex > 0)
    {
        UINT bytesWritten;

        if (f_write(&logFile, dataBuffer, bufferIndex, &bytesWritten) != FR_OK)
        {
            f_close(&logFile);
            isFileOpen = 0;
            sdCardStatus = SD_ERROR;
        }

        bufferIndex = 0;
    }
    else if (!isFileOpen && !isCardEjected)
    {
        Logger_CheckConnection();
    }
}

void Logger_StopRecording(void)
{
    if (isFileOpen)
    {
        f_close(&logFile);
        isFileOpen = 0;
    }
    sdCardStatus = SD_READY;
}

void Logger_EjectCard(void)
{
    Logger_StopRecording();
    f_mount(NULL, "", 0);
    isCardEjected = 1;
    sdCardStatus = SD_NOT_PRESENT;
}

SD_Status_t Logger_GetStatus(void)
{
    if (isCardEjected) return SD_NOT_PRESENT;
    return isFileOpen ? SD_WRITING : sdCardStatus;
}
