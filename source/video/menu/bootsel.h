#ifndef __VIDEO_BOOTSEL_H__
#define __VIDEO_BOOTSEL_H__

#include "hdr.h"
#include "fs/config.h"

#define MAX_TITLE_LEN        50
#define MAX_NAME_LEN         (MAX_TITLE_LEN / 2)

#define BOOTSEL_RET_UNKNOWN  0
#define BOOTSEL_RET_BOOT     1
#define BOOTSEL_RET_SHUTDOWN 100
#define BOOTSEL_RET_TOFWUI   101
#define BOOTSEL_RET_ERROR    UINT8_MAX

UINT8 bootsel_run(UINTN* sel, config_entry_t* entries, UINTN entries_count);

#ifdef DB_DEBUG
VOID bootsel_debuglog(config_entry_t* entries, UINTN entries_count);
#endif

#endif
