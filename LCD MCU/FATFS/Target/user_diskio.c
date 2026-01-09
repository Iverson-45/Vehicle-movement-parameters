#include "user_diskio.h"
#include "fatfs_sd.h"

DSTATUS USER_initialize (BYTE pdrv) { return SD_disk_initialize(pdrv); }
DSTATUS USER_status (BYTE pdrv) { return SD_disk_status(pdrv); }
DRESULT USER_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count) { return SD_disk_read(pdrv, buff, sector, count); }

#if _USE_WRITE == 1
DRESULT USER_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
  return SD_disk_write(pdrv, buff, sector, count);
}
#endif

#if _USE_IOCTL == 1
DRESULT USER_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
  return SD_disk_ioctl(pdrv, cmd, buff);
}
#endif

Diskio_drvTypeDef USER_Driver = {
  USER_initialize, USER_status, USER_read,
#if _USE_WRITE
  USER_write,
#endif
#if _USE_IOCTL == 1
  USER_ioctl,
#endif
};
