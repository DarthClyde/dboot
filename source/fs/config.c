#include "fs/config.h"
#include "fs/fs.h"

#include <efilib.h>

EFI_STATUS config_load(config_entry_t** entries, UINTN* count)
{
	EFI_STATUS status = EFIERR(99);

	file_t* file      = NULL;

	CHAR8* buffer     = NULL;
	UINTN buffer_size = 0;

	*entries          = NULL;
	*count            = 0;

	// Read the config file
	status = fs_file_open(fs_get_image(), CONFIG_FILE_PATH, &file);
	status += fs_file_readall(file, (VOID**)&buffer, &buffer_size);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to read dboot config file: %d\n", status);
		goto end;
	}

	// Parse the config file
	status = config_parse(buffer, buffer_size, entries, count);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to load parse config file: %d\n", status);
		goto end;
	}

end:
	if (buffer) uefi_call_wrapper(BS->FreePool, 1, buffer);
	if (file) fs_file_close(file);

	return status;
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
			// Check that current entry is not > max entries
			if (entry_count > MAX_ENTRIES) break;

			// Get and zero current entry
			current_entry = &config_entries[entry_count];
			ZeroMem(current_entry, sizeof(config_entry_t));

			// Set temporary entry identifier
			CHAR16* identifier;
			{
				UINTN length = lend - lstart - 2;
				identifier   = AllocatePool((length + 1) * sizeof(CHAR16));

				for (UINTN i = 0; i < length; i++) identifier[i] = (CHAR16)(lstart[i + 1]);
				identifier[length] = '\0';
			}

			// Set entry name and parent name
			{
				CHAR16* last_slash = NULL;
				CHAR16* current    = identifier;

				// Find last slash in identifier
				while (*current)
				{
					if (*current == '/') last_slash = current;
					current++;
				}

				// Entry is a child of a group
				if (last_slash)
				{
					// Set entry name
					UINTN length        = StrLen(last_slash);
					current_entry->name = AllocatePool((length + 1) * sizeof(CHAR16));
					StrCpy(current_entry->name, last_slash + 1);
					current_entry->name[length] = '\0';

					// Set parent name
					length                     = StrLen(identifier) - length;
					current_entry->parent_name = AllocatePool((length + 1) * sizeof(CHAR16));
					StrnCpy(current_entry->parent_name, identifier, length);
					current_entry->parent_name[length] = '\0';
				}

				// Entry is standalone
				else
				{
					// Set entry name
					UINTN length        = StrLen(identifier);
					current_entry->name = AllocatePool((length + 1) * sizeof(CHAR16));
					StrCpy(current_entry->name, identifier);
					current_entry->name[length] = '\0';

					// Set parent name
					current_entry->parent_name = NULL;
				}
			}

			FreePool(identifier);

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

				// Extract key: kernel
				else if (CompareMem(lstart, "kernel", 6) == 0)
				{
					current_entry->kernel_path = AllocatePool((val_len + 1) * sizeof(CHAR16));
					for (UINTN i = 0; i < val_len; i++)
						current_entry->kernel_path[i] = (CHAR16)(leql + 1)[i];
					current_entry->kernel_path[val_len] = '\0';
				}

				// Extract key: module
				else if (CompareMem(lstart, "module", 6) == 0)
				{
					current_entry->module_path = AllocatePool((val_len + 1) * sizeof(CHAR16));
					for (UINTN i = 0; i < val_len; i++)
						current_entry->module_path[i] = (CHAR16)(leql + 1)[i];
					current_entry->module_path[val_len] = '\0';
				}

				// Extract key: cmdline
				else if (CompareMem(lstart, "cmdline", 7) == 0)
				{
					current_entry->cmdline = AllocatePool((val_len + 1) * sizeof(CHAR16));
					for (UINTN i = 0; i < val_len; i++)
						current_entry->cmdline[i] = (CHAR16)(leql + 1)[i];
					current_entry->cmdline[val_len] = '\0';
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

#ifdef DB_DEBUG
VOID config_debuglog(config_entry_t* entries, UINTN count)
{
	Print(L"---- PARSED '%s' ----\n", CONFIG_FILE_PATH);

	for (UINTN i = 0; i < count; i++)
	{
		entry_type_t type = entries[i].type;
		if (type == ENTRY_TYPE_GROUP) continue;

		Print(L"%s/%s\n", entries[i].parent_name, entries[i].name);

		if (type == ENTRY_TYPE_LINUX)
		{
			Print(L"  Type: %d:linux\n", type);
			Print(L"  Kernel: %s\n", entries[i].kernel_path);
			Print(L"  Module: %s\n", entries[i].module_path);
			Print(L"  Cmdline: %s\n", entries[i].cmdline);
		}
	}

	Print(L"---- END CONFIG DEBUG LOG ----\n\n");
}
#endif
