#ifndef __PROTOS_CHAINLOAD_H__
#define __PROTOS_CHAINLOAD_H__

#include "hdr.h"
#include "fs/path.h"

error_t chainload_boot(path_t* efi_path);

#endif
