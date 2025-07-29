#ifndef __PROTOS_BOOT_H__
#define __PROTOS_BOOT_H__

#include <efi.h>
#include "fs/config.h"

EFI_STATUS boot_boot(config_entry_t* config);

#endif
