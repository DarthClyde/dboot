#include "fs/path.h"

#include <efilib.h>

inline static part_type_t str_to_parttype(CHAR16* str)
{
	if (strcmp(str, L"BOOT") == 0) return PART_TYPE_BOOT;
	else if (strcmp(str, L"GUID") == 0) return PART_TYPE_GUID;

	else return PART_TYPE_UNKNOWN;
}

error_t path_parse(CHAR16* path_raw, path_t** path)
{
	if (!path_raw) return ERR_INVALID_PARAM;

	path_t* newpath = NULL;
	error_t error   = ERR_OK;

	CHAR16* typesep = NULL;
	CHAR16* pathsep = NULL;

	CHAR16* strbuf  = NULL;
	UINTN len       = 0;

	*path           = NULL;

	// Allocate file path
	newpath = mem_alloc_zpool(sizeof(path_t));
	if (!newpath) return ERR_ALLOC_FAIL;

	// Find the first ':'
	typesep = strchr(path_raw, ':');
	if (!typesep)
	{
		error = ERR_PATH_NOPTYPE;
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

		newpath->type = str_to_parttype(strbuf);
		if (newpath->type == PART_TYPE_UNKNOWN)
		{
			error = ERR_PATH_NOPTYPE;
			goto end;
		}

		mem_free_pool(strbuf);
	}

	// Extract path partition modifer
	{
		len = pathsep - (typesep + 1);
		if (len > 0)
		{
			newpath->mod = mem_alloc_pool((len + 1) * sizeof(CHAR16));
			strcpys(newpath->mod, typesep + 1, len);
		}
	}

	// Extract path
	{
		len = strlen(pathsep + 1);
		if (len > 0)
		{
			newpath->path = mem_alloc_pool((len + 1) * sizeof(CHAR16));
			strcpys(newpath->path, pathsep + 1, len);
		}
	}

end:
	if (error) mem_free_pool(newpath);
	else *path = newpath;

	return error;
}

VOID path_free(path_t* path)
{
	if (path)
	{
		mem_free_pool(path->mod);
		mem_free_pool(path->path);
		mem_free_pool(path);
	}
}
