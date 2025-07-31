// List of error codes and their descriptions

ERR(ERR_OK, L"No error")
ERR(ERR_UNKNOWN, L"Unknown error")

ERR(ERR_INVALID_PARAM, L"Invalid parameter passes to function")
ERR(ERR_ALLOC_FAIL, L"Failed to allocate memory")

ERR(ERR_VID_GOP_LOCATE, L"Failed to locate the GOP protocol")
ERR(ERR_VID_GOP_QUERY, L"Failed to query GOP mode info")

ERR(ERR_FS_IMGLD_FAIL, L"Failed to load image")
ERR(ERR_FS_ROOTLD_FAIL, L"Failed to open root volume")
ERR(ERR_FS_FILE_INVALID, L"Unknown or invalid file ptr")
ERR(ERR_FS_FILE_OPEN, L"Failed to open file")
ERR(ERR_FS_FILE_CLOSE, L"Failed to close file")
ERR(ERR_FS_FILE_READ, L"Failed to read file")
ERR(ERR_FS_FILE_SETPOS, L"Failed to set file position")
ERR(ERR_FS_FILE_FILEINFO, L"Failed to get file info")

ERR(ERR_BOOT_UNSUPPORTED, L"Unsupported boot protocol")
ERR(ERR_BOOT_FAIL_LINUX, L"Failed to boot using 'Linux' protocol")
