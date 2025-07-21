#include "fs/config.h"

#include <efilib.h>

inline static UINTN get_file_size(EFI_FILE_HANDLE file)
{
	UINTN size;
	EFI_FILE_INFO* info;

	info = LibFileInfo(file);
	size = info->FileSize;

	FreePool(info);
	return size;
}

inline static VOID get_name_from_ident(CHAR16* ident, CHAR16* name)
{
	CHAR16* last_slash = NULL;
	CHAR16* current    = ident;

	// Find last slash in identifier
	while (*current)
	{
		if (*current == '/') last_slash = current;
		current++;
	}

	// Extract name
	if (last_slash) StrCpy(name, last_slash + 1);
	else StrCpy(name, ident);
}

EFI_STATUS config_load(config_entry_t** entries, UINTN* count, EFI_HANDLE image)
{
	EFI_STATUS status           = EFIERR(99);

	EFI_FILE_HANDLE volume      = NULL;
	EFI_FILE_HANDLE config_file = NULL;

	CHAR8* buffer               = NULL;
	UINTN buffer_size           = 0;

	*entries                    = NULL;
	*count                      = 0;

	// Open the root volume
	volume = LibOpenRoot(image);
	if (volume == NULL)
	{
		Print(L"Failed to open root volume\n");
		return EFI_LOAD_ERROR;
	}

	// Open the config file
	status = uefi_call_wrapper(volume->Open, 5, volume, &config_file, CONFIG_FILE_PATH,
	                           EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to open config file: %d\n", status);
		goto end;
	}

	// Get file information
	buffer_size = get_file_size(config_file);
	if (buffer_size == 0)
	{
		Print(L"Config file contains no data\n");
		goto end;
	}

	// Allocate buffer for file
	status = uefi_call_wrapper(BS->AllocatePool, 3, PoolAllocationType, buffer_size + 1,
	                           (VOID**)&buffer);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to allocate buffer for config file: %d\n", status);
		goto end;
	}

	// Read file content
	status = uefi_call_wrapper(config_file->Read, 3, config_file, &buffer_size, (VOID*)buffer);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to read config file: %d\n", status);
		goto end;
	}
	buffer[buffer_size] = '\0';

	// Parse the config file
	status = config_parse(buffer, buffer_size, entries, count);

end:
	if (buffer) uefi_call_wrapper(BS->FreePool, 1, buffer);
	if (config_file) uefi_call_wrapper(config_file->Close, 1, config_file);
	if (volume) uefi_call_wrapper(volume->Close, 1, volume);

	return status;
}

VOID config_debuglog(config_entry_t* entries, UINTN count)
{
	Print(L"---- PARSED '%s' ----\n", CONFIG_FILE_PATH);

	for (UINTN i = 0; i < count; i++)
	{
		if (entries[i].type == ENTRY_TYPE_GROUP) continue;

		Print(L"%s\n", entries[i].ident);
		Print(L"  Type: %d\n", entries[i].type);
	}

	Print(L"---- END CONFIG DEBUG LOG ----\n\n");
}

EFI_STATUS config_parse(CHAR8* buffer, UINTN size, config_entry_t** entries, UINTN* count)
{
	EFI_STATUS status              = EFIERR(99);

	CHAR8* lstart                  = buffer;
	CHAR8* lend                    = NULL;
	const CHAR8* fend              = buffer + size;

	UINTN entry_count              = 0;
	config_entry_t* config_entries = NULL;
	config_entry_t* current_entry  = NULL;

	// Allocate entry list
	status = uefi_call_wrapper(BS->AllocatePool, 3, PoolAllocationType,
	                           MAX_ENTRIES * sizeof(config_entry_t), (VOID**)&config_entries);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to allocate buffer for entries array: %d\n", status);
		goto end;
	}

	// Process config file
	while (lstart < fend)
	{
		// Find end of current line
		lend = lstart;
		while (lend < fend && *lend != '\n' && *lend != '\r') lend++;

		// Skip empty line
		if (lstart == lend)
		{
			lstart = lend + 1;
			continue;
		}

		// Parse header
		if (*lstart == '[' && *(lend - 1) == ']')
		{
			current_entry = &config_entries[entry_count];
			ZeroMem(current_entry, sizeof(config_entry_t));

			// Set entry identifier
			{
				UINTN length = lend - lstart - 2;
				if (length > MAX_IDENT_LEN)
				{
					// Skip entries with identifiers longer than the max to prevent issues when
					// parsing for groups
					continue;
				}

				for (UINTN i = 0; i < length; i++)
					current_entry->ident[i] = (CHAR16)(lstart[i + 1]);
				current_entry->ident[length] = '\0';
			}

			// Set entry name
			{
				get_name_from_ident(current_entry->ident, current_entry->name);
			}

			entry_count++;
		}

		// Parse content below header
		if (current_entry)
		{
			// Find value after '='
			CHAR8* leql = lstart;
			while (leql < lend && *leql != '=') leql++;

			// Extract content
			if (leql < lend)
			{
				UINTN val_len = lend - leql - 1;

				// Skip spaces and tabs
				while (val_len > 0 && (*(leql + 1) == ' ' || *(leql + 1) == '\t'))
				{
					leql++;
					val_len--;
				}

				// Extract key: type
				if (CompareMem(lstart, "type", 4) == 0)
				{
					if (CompareMem(leql + 1, "group", 5) == 0)
						current_entry->type = ENTRY_TYPE_GROUP;
					else if (CompareMem(leql + 1, "linux", 5) == 0)
						current_entry->type = ENTRY_TYPE_LINUX;
					else if (CompareMem(leql + 1, "efi", 3) == 0)
						current_entry->type = ENTRY_TYPE_EFI;
				}
			}
		}

		// Move to next line
		lstart = lend;
		while (lstart < fend && *lstart != '\n' && *lstart != '\r') lstart++;
	}

	*entries = config_entries;
	*count   = entry_count;
	status   = EFI_SUCCESS;

end:
	return status;
}