#include "sd_logger.h"
#include "fatfs.h"
#include "utils.h"
#include "input_handler.h"
#include <string.h>

#define LOG_BUFFER_SIZE 512
#define SYNC_INTERVAL_MS 1000

static FATFS fileSystem;
static FIL logFile;
static char dataBuffer[LOG_BUFFER_SIZE];
static uint16_t bufferIndex = 0;

static uint32_t recordingStartTime = 0;
static SD_Status_t sdCardStatus = SD_NOT_PRESENT;
static uint8_t isFileOpen = 0;
static uint8_t isCardEjected = 0;
static uint32_t lastConnectionCheckTime = 0;
static uint32_t lastSyncTime = 0;

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
    lastSyncTime = 0;

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
        if(f_mount(&fileSystem, "", 1) == FR_OK)
            sdCardStatus = SD_READY;
        else
            return 0;
    }

    recordingStartTime = 0;
    char fileName[12];
    char numStr[4];

    // MANUALNE SKŁADANIE NAZWY PLIKU - pewność poprawnego nazewnictwa dla FATFS
    for (int i = 0; i < 99; i++)
    {
        fileName[0] = 'L'; fileName[1] = 'o'; fileName[2] = 'g';
        IntToString(i, numStr);
        if (i < 10) {
            fileName[3] = '0';
            fileName[4] = numStr[0];
        } else {
            fileName[3] = numStr[0];
            fileName[4] = numStr[1];
        }
        fileName[5] = '.'; fileName[6] = 'C'; fileName[7] = 'S'; fileName[8] = 'V'; fileName[9] = 0;

        if (f_open(&logFile, fileName, FA_READ) != FR_OK) break;
        f_close(&logFile);
    }

    if (f_open(&logFile, fileName, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK)
    {
        isFileOpen = 1;
        sdCardStatus = SD_WRITING;
        f_puts("Time_s,Speed_km/h,Accel_m/s2,Power_HP,Pitch_deg,Roll_deg\n", &logFile);
        f_sync(&logFile);
        bufferIndex = 0;
        lastSyncTime = HAL_GetTick();
        return 1;
    }

    sdCardStatus = SD_ERROR;
    return 0;
}

void Logger_AddData(TelemetryData_t *data)
{
    if (!isFileOpen || isCardEjected) return;

    if (recordingStartTime == 0)
        recordingStartTime = HAL_GetTick();

    float powerHP = ((float)vehicleMassKg * data->accel * data->speed) / 2684.5f;
    if (powerHP < 0) powerHP = 0;

    float relativeTime = (HAL_GetTick() - recordingStartTime) * 0.001f;

    char valBuf[16];
    char *ptr = &dataBuffer[bufferIndex];

    // Ręczne, bardzo szybkie rzeźbienie w stringach by uniknąć narzutu systemowego sprintf
    FloatToString(relativeTime, valBuf, 2);
    char *src = valBuf; while(*src) *ptr++ = *src++; *ptr++ = ',';

    IntToString(data->speed, valBuf);
    src = valBuf; while(*src) *ptr++ = *src++; *ptr++ = ',';

    FloatToString(data->accel, valBuf, 2);
    src = valBuf; while(*src) *ptr++ = *src++; *ptr++ = ',';

    FloatToString(powerHP, valBuf, 1);
    src = valBuf; while(*src) *ptr++ = *src++; *ptr++ = ',';

    IntToString(data->pitch, valBuf);
    src = valBuf; while(*src) *ptr++ = *src++; *ptr++ = ',';

    IntToString(data->roll, valBuf);
    src = valBuf; while(*src) *ptr++ = *src++; *ptr++ = '\n';

    uint16_t length = (uint16_t)(ptr - &dataBuffer[bufferIndex]);
    bufferIndex += length;

    // FIZYCZNY ZAPIS ZBIORCZY. Minimalizuje dławienie szyny SPI i chroni UART.
    if (bufferIndex >= LOG_BUFFER_SIZE - 64)
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
}

void Logger_FlushBuffer(void)
{
    if (isFileOpen && !isCardEjected)
    {
        // WYMUSZENIE FIZYCZNEGO ZAPISU DO FATFS MAX RAZ NA SEKUNDĘ
        if (HAL_GetTick() - lastSyncTime > SYNC_INTERVAL_MS)
        {
            if (bufferIndex > 0)
            {
                UINT bytesWritten;
                if (f_write(&logFile, dataBuffer, bufferIndex, &bytesWritten) != FR_OK)
                {
                    f_close(&logFile);
                    isFileOpen = 0;
                    sdCardStatus = SD_ERROR;
                    return;
                }
                bufferIndex = 0;
            }

            if (f_sync(&logFile) != FR_OK)
            {
                f_close(&logFile);
                isFileOpen = 0;
                sdCardStatus = SD_ERROR;
            }
            lastSyncTime = HAL_GetTick();
        }
    }
    else if (!isCardEjected)
    {
        Logger_CheckConnection();
    }
}

void Logger_StopRecording(void)
{
    if (isFileOpen)
    {
        if (bufferIndex > 0 && !isCardEjected)
        {
             UINT bytesWritten;
             f_write(&logFile, dataBuffer, bufferIndex, &bytesWritten);
        }

        f_close(&logFile);
        isFileOpen = 0;
    }
    sdCardStatus = SD_READY;
}

void Logger_EjectCard(void)
{
    if (isFileOpen)
    {
        f_close(&logFile);
        isFileOpen = 0;
    }
    f_mount(NULL, "", 0);
    isCardEjected = 1;
    sdCardStatus = SD_NOT_PRESENT;
}

SD_Status_t Logger_GetStatus(void)
{
    if (isCardEjected) return SD_NOT_PRESENT;
    return isFileOpen ? SD_WRITING : sdCardStatus;
}
