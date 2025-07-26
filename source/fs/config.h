#ifndef __FS_CONFIG_H__
#define __FS_CONFIG_H__

#include <efi.h>

#define MAX_ENTRIES      16
#define CONFIG_FILE_PATH L"\\EFI\\DBOOT.CONF"

typedef enum : UINT8
{
	ENTRY_TYPE_LINUX,
	ENTRY_TYPE_EFI,

	ENTRY_TYPE_GROUP
} entry_type_t;

typedef struct config_entry
{
	// Entry info
	CHAR16* name;
	CHAR16* parent_name;

	// Entry options
	entry_type_t type;

	// Entry config
	CHAR16* kernel_path;
	CHAR16* module_path;
	CHAR16* cmdline;
} config_entry_t;

EFI_STATUS config_load(config_entry_t** entries, UINTN* count);
EFI_STATUS config_parse(CHAR8* buffer, UINTN size, config_entry_t** entries, UINTN* count);

// Debug
#ifdef DB_DEBUG
VOID config_debuglog(config_entry_t* entries, UINTN count);
#endif

#endif
