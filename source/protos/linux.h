#ifndef __PROTOS_LINUX_H__
#define __PROTOS_LINUX_H__

#include "hdr.h"

error_t linux_boot(CHAR16* kernel_path, CHAR16* initrd_path, CHAR8* cmdline);

#endif
