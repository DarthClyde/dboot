#ifndef __FS_FS_H__
#define __FS_FS_H__

#include "hdr.h"
#include "part.h"
#include "path.h"

typedef struct file
{
	EFI_FILE_HANDLE handle;
	EFI_FILE_HANDLE part;
} file_t;

error_t fs_init(EFI_HANDLE image);

error_t fs_setpart(part_type_t type, CHAR16* mod);
partition_t* fs_getpart(void);

error_t fs_file_read(file_t* file, UINTN* size, VOID* buffer);
error_t fs_file_readr(file_t* file, UINTN size, VOID* buffer);
error_t fs_file_readall(file_t* file, VOID** buffer, UINTN* size);
error_t fs_file_setpos(file_t* file, UINTN pos);
error_t fs_file_getsize(file_t* file, UINTN* size);

error_t fs_file_open(CHAR16* path, file_t** file);
error_t fs_file_close(file_t* file);

#endif
