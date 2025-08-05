#include "fs/path.h"

#include <efilib.h>

inline static part_type_t str_to_parttype(CHAR16* str)
{
	if (strcmp(str, L"BOOT") == 0) return PART_TYPE_BOOT;
	else if (strcmp(str, L"GUID") == 0) return PART_TYPE_GUID;

	else return PART_TYPE_UNKNOWN;
}

error_t path_parse(CHAR16* path_raw, file_path_t** file_path)
{
	if (!path_raw) return ERR_INVALID_PARAM;

	file_path_t* path = NULL;
	error_t error     = ERR_OK;

	CHAR16* typesep   = NULL;
	CHAR16* pathsep   = NULL;

	CHAR16* strbuf    = NULL;
	UINTN len         = 0;

	*file_path        = NULL;

	// Allocate file path
	path = mem_alloc_pool(sizeof(file_path_t));
	if (!path) return ERR_ALLOC_FAIL;

	// Set defaults
	path->type = PART_TYPE_UNKNOWN;
	path->mod  = NULL;
	path->path = NULL;

	// Find the first ':'
	typesep = strchr(path_raw, ':');
	if (!typesep)
	{
		error = ERR_PATH_NODISK;
		goto end;
	}

	// Find the first '/'
	pathsep = strchr(typesep + 1, '/');
	if (!pathsep)
	{
		error = ERR_PATH_NOPATH;
		goto end;
	}

	// Extract path partition type
	{
		len    = typesep - path_raw;
		strbuf = mem_alloc_pool((len + 1) * sizeof(CHAR16));
		strcpys(strbuf, path_raw, len);

		path->type = str_to_parttype(strbuf);
		if (path->type == PART_TYPE_UNKNOWN)
		{
			error = ERR_PATH_NODISK;
			goto end;
		}

		mem_free_pool(strbuf);
	}

	// Extract path partition modifer
	{
		len = pathsep - (typesep + 1);
		if (len > 0)
		{
			path->mod = mem_alloc_pool((len + 1) * sizeof(CHAR16));
			strcpys(path->mod, typesep + 1, len);
		}
	}

	// Extract path
	{
		len = strlen(pathsep + 1);
		if (len > 0)
		{
			path->path = mem_alloc_pool((len + 1) * sizeof(CHAR16));
			strcpys(path->path, pathsep + 1, len);
		}
	}

end:
	if (error) mem_free_pool(path);
	else *file_path = path;

	return error;
}

VOID path_free(file_path_t* file_path)
{
	if (file_path)
	{
		mem_free_pool(file_path->mod);
		mem_free_pool(file_path->path);
		mem_free_pool(file_path);
	}
}
