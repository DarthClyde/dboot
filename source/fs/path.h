#ifndef __FS_PATH_H__
#define __FS_PATH_H__

#include "hdr.h"

typedef enum file_disk
{
	FILE_DISK_UNKNOWN = 0,

	FILE_DISK_BOOT,
	FILE_DISK_GUID
} file_disk_t;

typedef struct file_path
{
	file_disk_t disk;
	CHAR16* mod;

	CHAR16* path;
} file_path_t;

error_t path_parse(CHAR16* path_raw, file_path_t** file_path);
VOID path_free(file_path_t* file_path);

#endif
