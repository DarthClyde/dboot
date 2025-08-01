#include "fs/config.h"
#include "fs/fs.h"

#include <efilib.h>

error_t config_load(config_entry_t** entries, UINTN* count, config_global_t* global)
{
	error_t error     = ERR_OK;

	file_t* file      = NULL;

	CHAR8* buffer     = NULL;
	UINTN buffer_size = 0;

	*entries          = NULL;
	*count            = 0;
	global            = NULL;

	// Read the config file
	error = fs_file_open(fs_get_image(), CONFIG_FILE_PATH, &file);
	ERR_CHECK(error, END);
	error = fs_file_readall(file, (VOID**)&buffer, &buffer_size);
	ERR_CHECK(error, END);

	// Parse the config file
	error = config_parse(buffer, buffer_size, entries, count, global);
	ERR_CHECK(error, END);

end:
	if (buffer) uefi_call_wrapper(BS->FreePool, 1, buffer);
	if (file) fs_file_close(file);

	return error;
}

error_t config_parse(CHAR8* buffer, UINTN size, config_entry_t** entries, UINTN* count,
                     config_global_t* global)
{
	EFI_STATUS status              = EFI_SUCCESS;

	CHAR8* lstart                  = buffer;
	CHAR8* lend                    = NULL;
	const CHAR8* fend              = buffer + size;
	BOOLEAN is_global              = TRUE;

	UINTN entry_count              = 0;
	config_entry_t* config_entries = NULL;
	config_entry_t* current_entry  = NULL;
	config_global_t* config_global = NULL;

	// Allocate entry list
	status = uefi_call_wrapper(BS->AllocatePool, 3, PoolAllocationType,
	                           MAX_ENTRIES * sizeof(config_entry_t), (VOID**)&config_entries);
	if (EFI_ERROR(status)) return ERR_ALLOC_FAIL;

	// Allocate global config
	status = uefi_call_wrapper(BS->AllocatePool, 3, PoolAllocationType, sizeof(config_entry_t),
	                           (VOID**)&config_global);
	if (EFI_ERROR(status)) return ERR_ALLOC_FAIL;

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

		// Detect global header
		if (*lstart == '[' && *(lstart + 1) == ']')
		{
			current_entry = NULL;
			is_global     = TRUE;
			goto mov_next;
		}

		// Parse header
		if (*lstart == '[' && *(lend - 1) == ']')
		{
			// Check that current entry is not > max entries
			if (entry_count > MAX_ENTRIES) break;

			// Get and zero current entry
			current_entry = &config_entries[entry_count];
			ZeroMem(current_entry, sizeof(config_entry_t));
			is_global = FALSE;

			// Set temporary entry identifier
			{
				UINTN length         = lend - lstart - 2;
				current_entry->ident = AllocatePool((length + 1) * sizeof(CHAR16));

				for (UINTN i = 0; i < length; i++)
					current_entry->ident[i] = (CHAR16)(lstart[i + 1]);
				current_entry->ident[length] = '\0';
			}

			// Set entry name and parent name
			{
				CHAR16* last_slash = NULL;
				CHAR16* current    = current_entry->ident;

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
					length                     = StrLen(current_entry->ident) - length;
					current_entry->parent_name = AllocatePool((length + 1) * sizeof(CHAR16));
					StrnCpy(current_entry->parent_name, current_entry->ident, length);
					current_entry->parent_name[length] = '\0';
				}

				// Entry is standalone
				else
				{
					// Set entry name
					UINTN length        = StrLen(current_entry->ident);
					current_entry->name = AllocatePool((length + 1) * sizeof(CHAR16));
					StrCpy(current_entry->name, current_entry->ident);
					current_entry->name[length] = '\0';

					// Set parent name
					current_entry->parent_name = NULL;
				}
			}

			entry_count++;
			goto mov_next;
		}

		// Parse content below header
		if (current_entry || is_global)
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

				if (is_global)
				{
					// Extract key: default
					if (CompareMem(lstart, "default", 7) == 0)
					{
						global->default_entry = AllocatePool((val_len + 1) * sizeof(CHAR16));
						for (UINTN i = 0; i < val_len; i++)
							global->default_entry[i] = (CHAR16)(leql + 1)[i];
						global->default_entry[val_len] = '\0';
					}
				}
				else
				{

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
						current_entry->cmdline = AllocatePool((val_len + 1) * sizeof(CHAR8));
						for (UINTN i = 0; i < val_len; i++)
							current_entry->cmdline[i] = (CHAR8)(leql + 1)[i];
						current_entry->cmdline[val_len] = '\0';
					}
				}
			}
		}

mov_next:
		// Move to next line
		lstart = lend;
		while (lstart < fend && *lstart != '\n' && *lstart != '\r') lstart++;
	}

	*entries = config_entries;
	*count   = entry_count;
	global   = config_global;

	return ERR_OK;
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
