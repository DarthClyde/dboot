#ifndef __PROTOS_LINUX_H__
#define __PROTOS_LINUX_H__

#include <efi.h>

EFI_STATUS linux_boot(CHAR16* kernel_path, CHAR16* module_path, CHAR16* cmdline);

#endif
