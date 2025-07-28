#ifndef __FS_FS_H__
#define __FS_FS_H__

#include <efi.h>

typedef struct file
{
	EFI_FILE_HANDLE handle;
	EFI_FILE_HANDLE root;
} file_t;

EFI_STATUS fs_load_image(EFI_HANDLE image);
EFI_LOADED_IMAGE* fs_get_image(void);

EFI_STATUS fs_file_read(file_t* file, UINTN* size, VOID** buffer);
EFI_STATUS fs_file_readall(file_t* file, VOID** buffer, UINTN* size);
EFI_STATUS fs_file_setpos(file_t* file, UINTN pos);
EFI_STATUS fs_file_getsize(file_t* file, UINTN* size);

EFI_STATUS fs_file_open(EFI_LOADED_IMAGE* image, CHAR16* path, file_t** file);
EFI_STATUS fs_file_close(file_t* file);

#endif
