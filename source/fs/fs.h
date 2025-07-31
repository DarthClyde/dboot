#ifndef __FS_FS_H__
#define __FS_FS_H__

#include "hdr.h"

typedef struct file
{
	EFI_FILE_HANDLE handle;
	EFI_FILE_HANDLE root;
} file_t;

error_t fs_load_image(EFI_HANDLE image);
EFI_LOADED_IMAGE* fs_get_image(void);

error_t fs_file_read(file_t* file, UINTN* size, VOID* buffer);
error_t fs_file_readr(file_t* file, UINTN size, VOID* buffer);
error_t fs_file_readall(file_t* file, VOID** buffer, UINTN* size);
error_t fs_file_setpos(file_t* file, UINTN pos);
error_t fs_file_getsize(file_t* file, UINTN* size);

error_t fs_file_open(EFI_LOADED_IMAGE* image, CHAR16* path, file_t** file);
error_t fs_file_close(file_t* file);

#endif
