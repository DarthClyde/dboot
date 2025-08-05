#ifndef __FS_PATH_H__
#define __FS_PATH_H__

#include "hdr.h"

typedef enum part_type
{
	PART_TYPE_UNKNOWN = 0,

	PART_TYPE_BOOT,
	PART_TYPE_GUID
} part_type_t;

typedef struct file_path
{
	part_type_t type;
	CHAR16* mod;

	CHAR16* path;
} file_path_t;

error_t path_parse(CHAR16* path_raw, file_path_t** file_path);
VOID path_free(file_path_t* file_path);

#endif
