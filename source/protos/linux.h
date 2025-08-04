#ifndef __PROTOS_LINUX_H__
#define __PROTOS_LINUX_H__

#include "hdr.h"
#include "fs/path.h"

error_t linux_boot(file_path_t* kernel_path, file_path_t* initrd_path, CHAR8* cmdline);

#endif
