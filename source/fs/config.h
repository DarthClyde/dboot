#ifndef __FS_CONFIG_H__
#define __FS_CONFIG_H__

#include <efi.h>

#define MAX_NAME_LEN 128
#define MAX_PATH_LEN 256
#define MAX_ENTRIES 16
#define CONFIG_FILE_PATH L"\\EFI\\DBOOT.CONF"

typedef enum
{
	ENTRY_TYPE_LINUX,
	ENTRY_TYPE_EFI,

	ENTRY_TYPE_GROUP
} entry_type_t;

typedef struct _ConfigEntry
{
	// Entry name
	CHAR16 name[MAX_NAME_LEN];
	CHAR16 display_name[MAX_NAME_LEN];

	// Entry info
	entry_type_t type;

} config_entry_t;

// Config loader
EFI_STATUS config_load(config_entry_t** entries, UINTN* count, EFI_HANDLE image);
VOID config_debuglog(config_entry_t* entries, UINTN count);

// Config parser
EFI_STATUS config_parse(CHAR8* buffer, UINTN size, config_entry_t** entries, UINTN* count);

#endif