// List of error codes and their descriptions

ERR(ERR_OK, L"No error")
ERR(ERR_UNKNOWN, L"Unknown error")

ERR(ERR_HANDLLOC_FAIL, L"Failed to locate handle for protocol")
ERR(ERR_INVALID_PARAM, L"Invalid parameter passed to function")
ERR(ERR_ALLOC_FAIL, L"Failed to allocate memory")

ERR(ERR_VID_GOP_LOCATE, L"Failed to locate the GOP protocol")
ERR(ERR_VID_GOP_QUERY, L"Failed to query GOP mode info")

ERR(ERR_FS_PARTLD_FAIL, L"Failed to load partition")
ERR(ERR_FS_FILE_INVALID, L"Unknown or invalid file ptr")
ERR(ERR_FS_FILE_SETPART, L"Failed to set the requested partition")
ERR(ERR_FS_FILE_OPEN, L"Failed to open file")
ERR(ERR_FS_FILE_CLOSE, L"Failed to close file")
ERR(ERR_FS_FILE_READ, L"Failed to read file")
ERR(ERR_FS_FILE_SETPOS, L"Failed to set file position")
ERR(ERR_FS_FILE_FILEINFO, L"Failed to get file info")

ERR(ERR_PATH_NOPTYPE, L"Failed to find the file partition type")
ERR(ERR_PATH_NOPATH, L"Failed to find the file path")

ERR(ERR_BOOT_UNSUPPORTED, L"Unsupported boot protocol")
ERR(ERR_BOOT_FAIL_LINUX, L"Failed to boot using 'Linux' protocol")
