#ifndef __VIDEO_BOOTSEL_H__
#define __VIDEO_BOOTSEL_H__

#include <efi.h>
#include "fs/config.h"

#define MAX_TITLE_LEN        50
#define MAX_NAME_LEN         (MAX_TITLE_LEN / 2)

#define BOOTSEL_RET_ERROR    -1
#define BOOTSEL_RET_SHUTDOWN -99

EFI_STATUS bootsel_run(UINT8* sel, config_entry_t* entries, UINTN entries_count);

#endif