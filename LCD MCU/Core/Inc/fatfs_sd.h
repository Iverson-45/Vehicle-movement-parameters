#ifndef FATFS_SD_H_
#define FATFS_SD_H_

#include "integer.h"
#include "diskio.h"
#include "stm32f3xx_hal.h"

// SD Card Commands
#define CMD0     (0x40+0)
#define CMD1     (0x40+1)
#define CMD8     (0x40+8)
#define CMD9     (0x40+9)
#define CMD10    (0x40+10)
#define CMD12    (0x40+12)
#define CMD16    (0x40+16)
#define CMD17    (0x40+17)
#define CMD18    (0x40+18)
#define CMD23    (0x40+23)
#define CMD24    (0x40+24)
#define CMD25    (0x40+25)
#define CMD41    (0x40+41)
#define CMD55    (0x40+55)
#define CMD58    (0x40+58)

// Card Type Definitions
#define CT_MMC      0x01
#define CT_SD1      0x02
#define CT_SD2      0x04
#define CT_SDC      0x06
#define CT_BLOCK    0x08

DSTATUS SD_disk_initialize(BYTE pdrv);
DSTATUS SD_disk_status(BYTE pdrv);
DRESULT SD_disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count);
DRESULT SD_disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count);
DRESULT SD_disk_ioctl(BYTE pdrv, BYTE cmd, void* buff);

#endif
