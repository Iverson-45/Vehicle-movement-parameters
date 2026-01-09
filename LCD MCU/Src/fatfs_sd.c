#include "fatfs_sd.h"
#include "main.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

extern SPI_HandleTypeDef hspi1;
#define HSPI_SDCARD &hspi1

#define SD_CS_LOW()   HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET)
#define SD_CS_HIGH()  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET)

static volatile DSTATUS DiskStatus = STA_NOINIT;
static uint8_t SDCardType;

static uint8_t SPI_ReceiveByte(void)
{
    uint8_t dummy = 0xFF, data;

    if(HAL_SPI_TransmitReceive(HSPI_SDCARD, &dummy, &data, 1, 10) != HAL_OK) return 0xFF;

    return data;
}

static void SPI_TransmitByte(uint8_t data)
{
    HAL_SPI_Transmit(HSPI_SDCARD, &data, 1, 10);
}

static uint8_t SD_WaitReady(void)
{
    uint32_t startTick = HAL_GetTick();
    uint8_t response;

    do
    {
        response = SPI_ReceiveByte();

        if (response == 0xFF) return 0xFF;

    } while (HAL_GetTick() - startTick < 200);

    return 0x00;
}

static uint8_t SD_SendCommand(uint8_t command, uint32_t argument)
{
    uint8_t crc, response;

    if (SD_WaitReady() != 0xFF) return 0xFF;

    SPI_TransmitByte(command);
    SPI_TransmitByte(argument >> 24);
    SPI_TransmitByte(argument >> 16);
    SPI_TransmitByte(argument >> 8);
    SPI_TransmitByte(argument);

    crc = (command == CMD0) ? 0x95 : (command == CMD8 ? 0x87 : 0x01);
    SPI_TransmitByte(crc);

    uint8_t retry = 10;

    do
    {
        response = SPI_ReceiveByte();
    } while ((response & 0x80) && --retry);

    return response;
}

static uint8_t SD_ReadBlock(BYTE *buffer, UINT length)
{
    uint8_t token;
    uint32_t startTick = HAL_GetTick();

    do
    {
        token = SPI_ReceiveByte();
    } while((token == 0xFF) && (HAL_GetTick() - startTick < 200));

    if(token != 0xFE) return 0;

    while(length--)
    {
        *buffer++ = SPI_ReceiveByte();
    }

    SPI_ReceiveByte(); // Discard CRC
    SPI_ReceiveByte();
    return 1;
}

static uint8_t SD_WriteBlock(const uint8_t *buffer, BYTE token)
{
    uint8_t response;
    uint32_t startTick = HAL_GetTick();

    if (SD_WaitReady() != 0xFF) return 0;

    SPI_TransmitByte(token);
    if (token != 0xFD)
    {
        HAL_SPI_Transmit(HSPI_SDCARD, (uint8_t*)buffer, 512, 100);
        SPI_ReceiveByte(); // Discard CRC
        SPI_ReceiveByte();

        while (1)
        {
            response = SPI_ReceiveByte();
            if ((response & 0x1F) == 0x05) break;
            if (HAL_GetTick() - startTick > 200) return 0;
        }

        startTick = HAL_GetTick();
        while (SPI_ReceiveByte() == 0)
        {
            if (HAL_GetTick() - startTick > 500) return 0;
        }
    }
    return 1;
}


DSTATUS SD_disk_initialize(BYTE pdrv)
{
    uint8_t i, type = 0, ocr[4];

    SD_CS_HIGH();
    for(i = 0; i < 10; i++) SPI_TransmitByte(0xFF);
    SD_CS_LOW();

    if (SD_SendCommand(CMD0, 0) == 1)
    {
        if (SD_SendCommand(CMD8, 0x1AA) == 1)
        {
            for (i = 0; i < 4; i++)
            {
                ocr[i] = SPI_ReceiveByte();
            }

            if (ocr[2] == 0x01 && ocr[3] == 0xAA)
            {
                uint32_t startTick = HAL_GetTick();

                while((HAL_GetTick() - startTick) < 1000)
                {
                    if (SD_SendCommand(CMD55, 0) <= 1 && SD_SendCommand(CMD41, 0x40000000) == 0) break;
                }

                if (SD_SendCommand(CMD58, 0) == 0)
                {
                    for (i = 0; i < 4; i++) ocr[i] = SPI_ReceiveByte();
                    type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
                }
            }
        }
    }
    SDCardType = type;
    SD_CS_HIGH();
    SPI_ReceiveByte();

    if (type) DiskStatus &= ~STA_NOINIT;
    else DiskStatus = STA_NOINIT;

    return DiskStatus;
}

DSTATUS SD_disk_status(BYTE pdrv) { return DiskStatus; }

DRESULT SD_disk_read(BYTE pdrv, BYTE* buffer, DWORD sector, UINT count)
{
    if (DiskStatus & STA_NOINIT) return RES_NOTRDY;
    if (!(SDCardType & CT_BLOCK)) sector *= 512;

    SD_CS_LOW();
    DRESULT result = RES_ERROR;

    if (count == 1)
    {
        if (SD_SendCommand(CMD17, sector) == 0 && SD_ReadBlock(buffer, 512)) result = RES_OK;
    }
    else
    {
        if (SD_SendCommand(CMD18, sector) == 0)
        {
            do
            {
                if (!SD_ReadBlock(buffer, 512)) break;
                buffer += 512;
            } while (--count);

            SD_SendCommand(CMD12, 0);
            if (count == 0) result = RES_OK;
        }
    }

    SD_CS_HIGH();
    SPI_ReceiveByte();
    return result;
}

DRESULT SD_disk_write(BYTE pdrv, const BYTE* buffer, DWORD sector, UINT count)
{
    if (DiskStatus & STA_NOINIT) return RES_NOTRDY;
    if (!(SDCardType & CT_BLOCK)) sector *= 512;

    SD_CS_LOW();
    DRESULT result = RES_ERROR;

    if (count == 1)
    {
        if (SD_SendCommand(CMD24, sector) == 0 && SD_WriteBlock(buffer, 0xFE)) result = RES_OK;
    }
    else
    {
        if (SD_SendCommand(CMD25, sector) == 0)
        {
            do
            {
                if (!SD_WriteBlock(buffer, 0xFC)) break;
                buffer += 512;
            } while (--count);

            if (SD_WriteBlock(0, 0xFD)) result = RES_OK;
        }
    }

    SD_CS_HIGH();
    SPI_ReceiveByte();
    return result;
}

DRESULT SD_disk_ioctl(BYTE pdrv, BYTE controlCode, void *buffer)
{
    if (DiskStatus & STA_NOINIT) return RES_NOTRDY;
    DRESULT result = RES_ERROR;

    switch (controlCode)
    {
        case CTRL_SYNC:
            SD_CS_LOW();
            if (SD_WaitReady() == 0xFF) result = RES_OK;
            SD_CS_HIGH();
            break;

        case GET_SECTOR_COUNT:
        {
            uint8_t csd[16];
            SD_CS_LOW();

            if (SD_SendCommand(CMD9, 0) == 0 && SD_ReadBlock(csd, 16))
            {
                if ((csd[0] >> 6) == 1)
                {
                    uint32_t size = csd[9] + ((uint32_t)csd[8] << 8) + 1;
                    *(DWORD*)buffer = size << 10;
                }
                result = RES_OK;
            }
            SD_CS_HIGH();
        } break;

        case GET_SECTOR_SIZE:
            *(WORD*)buffer = 512;
            result = RES_OK;
            break;

        case GET_BLOCK_SIZE:
            *(DWORD*)buffer = 128;
            result = RES_OK;
            break;

        default:
            result = RES_PARERR;
            break;
    }
    return result;
}
