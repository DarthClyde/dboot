#ifndef __FS_CONFIG_H__
#define __FS_CONFIG_H__

#include <efi.h>

#define MAX_IDENT_LEN    128
#define MAX_PATH_LEN     256
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
	CHAR16 ident[MAX_IDENT_LEN];
	CHAR16 name[MAX_IDENT_LEN];
	CHAR16 parent_name[MAX_IDENT_LEN];

	// Entry options
	entry_type_t type;
} config_entry_t;

EFI_STATUS config_load(config_entry_t** entries, UINTN* count, EFI_HANDLE image);
EFI_STATUS config_parse(CHAR8* buffer, UINTN size, config_entry_t** entries, UINTN* count);

// Debug
#ifdef DB_DEBUG
VOID config_debuglog(config_entry_t* entries, UINTN count);
#endif

#endif