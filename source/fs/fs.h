#ifndef __FS_FS_H__
#define __FS_FS_H__

#include <efi.h>

EFI_STATUS fs_load_image(EFI_HANDLE image);
EFI_LOADED_IMAGE* fs_get_image(void);

EFI_STATUS fs_load_file(CHAR16* path, VOID** buffer, UINTN* size);

#endif
