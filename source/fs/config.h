#ifndef __FS_CONFIG_H__
#define __FS_CONFIG_H__

#include "hdr.h"

#define MAX_ENTRIES      16
#define CONFIG_FILE_PATH L"\\EFI\\DBOOT\\DBOOT.CONF"

typedef enum : UINT8
{
	ENTRY_TYPE_UNKNOWN = 0,

	ENTRY_TYPE_LINUX,
	ENTRY_TYPE_CHAINLD,

	ENTRY_TYPE_GROUP,
} entry_type_t;

typedef struct config_entry
{
	// Entry info
	CHAR16* ident;
	CHAR16* name;
	CHAR16* parent_name;

	// Entry options
	entry_type_t type;

	// Protocol: Linux
	CHAR16* kernel_path;
	CHAR16* module_path;
	CHAR8* cmdline;

	// Protocol: Chainload
	CHAR16* efi_path;
} config_entry_t;

typedef struct config_global
{
	CHAR16* default_entry;
	INT64 timeout;
} config_global_t;

error_t config_load(config_entry_t** entries, UINTN* count, config_global_t** global);
error_t config_parse(CHAR8* buffer, UINTN size, config_entry_t** entries, UINTN* count,
                     config_global_t** global);

// Debug
#ifdef DB_DEBUG
VOID config_debuglog(config_entry_t* entries, UINTN count);
#endif

#endif
