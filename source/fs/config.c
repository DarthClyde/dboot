#include "fs/config.h"
#include "fs/fs.h"

#include <efilib.h>

error_t config_load(config_entry_t** entries, UINTN* count, config_global_t** global)
{
	error_t error     = ERR_OK;

	file_t* file      = NULL;

	CHAR8* buffer     = NULL;
	UINTN buffer_size = 0;

	*entries          = NULL;
	*count            = 0;
	*global           = NULL;

	// Read the config file
	error = fs_setpart(PART_TYPE_BOOT, NULL);
	ERR_CHECK(error, END);
	error = fs_file_open(CONFIG_FILE_PATH, &file);
	ERR_CHECK(error, END);
	error = fs_file_readall(file, (VOID**)&buffer, &buffer_size);
	ERR_CHECK(error, END);

	// Parse the config file
	error = config_parse(buffer, buffer_size, entries, count, global);
	ERR_CHECK(error, END);

end:
	mem_free_pool(buffer);
	fs_file_close(file);

	return error;
}

#define EXTRACT_STR(TYPE, VAR)                                            \
	{                                                                     \
		VAR = mem_alloc_pool((val_len + 1) * sizeof(TYPE));               \
		for (UINTN i = 0; i < val_len; i++) VAR[i] = (TYPE)(leql + 1)[i]; \
		VAR[val_len] = '\0';                                              \
	}

inline static VOID set_default_global(config_global_t* global)
{
	if (!global) return;

	global->default_entry = NULL;
	global->timeout       = 5;
}

error_t config_parse(CHAR8* buffer, UINTN size, config_entry_t** entries, UINTN* count,
                     config_global_t** global)
{
	CHAR8* lstart                  = buffer;
	CHAR8* lend                    = NULL;
	const CHAR8* fend              = buffer + size;
	BOOLEAN is_global              = TRUE;

	UINTN entry_count              = 0;
	CHAR16* strbuf                 = NULL;
	config_entry_t* config_entries = NULL;
	config_entry_t* current_entry  = NULL;
	config_global_t* config_global = NULL;

	// Allocate entry list
	config_entries = mem_alloc_zpool(MAX_ENTRIES * sizeof(config_entry_t));
	if (!config_entries) return ERR_ALLOC_FAIL;

	// Allocate global config
	config_global = mem_alloc_pool(sizeof(config_global_t));
	if (!config_global) return ERR_ALLOC_FAIL;
	set_default_global(config_global);

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
			memzero(current_entry, sizeof(config_entry_t));
			is_global = FALSE;

			// Set temporary entry identifier
			{
				UINTN length         = lend - lstart - 2;
				current_entry->ident = mem_alloc_pool((length + 1) * sizeof(CHAR16));

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
					UINTN length        = strlen(last_slash);
					current_entry->name = mem_alloc_pool((length + 1) * sizeof(CHAR16));
					strcpy(current_entry->name, last_slash + 1);
					current_entry->name[length] = '\0';

					// Set parent name
					length                     = strlen(current_entry->ident) - length;
					current_entry->parent_name = mem_alloc_pool((length + 1) * sizeof(CHAR16));
					strcpys(current_entry->parent_name, current_entry->ident, length);
					current_entry->parent_name[length] = '\0';
				}

				// Entry is standalone
				else
				{
					// Set entry name
					UINTN length        = strlen(current_entry->ident);
					current_entry->name = mem_alloc_pool((length + 1) * sizeof(CHAR16));
					strcpy(current_entry->name, current_entry->ident);
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

				// Extract global keys
				if (is_global)
				{
					// Extract key: default
					if (memcmp(lstart, "default", 7) == 0)
						EXTRACT_STR(CHAR16, config_global->default_entry)

					// Extract key: timeout
					else if (memcmp(lstart, "timeout", 7) == 0)
					{
						EXTRACT_STR(CHAR16, strbuf)

						if (strcmp(strbuf, L"false") == 0) config_global->timeout = -1;
						else config_global->timeout = str_to_i64(strbuf);

						mem_free_pool(strbuf);
					}

					goto mov_next;
				}

				// Extract key: type
				if (memcmp(lstart, "type", 4) == 0)
				{
					if (memcmp(leql + 1, "group", 5) == 0)
					{
						current_entry->type = ENTRY_TYPE_GROUP;
					}
					else if (memcmp(leql + 1, "linux", 5) == 0)
					{
						current_entry->type = ENTRY_TYPE_LINUX;
					}
					else if (memcmp(leql + 1, "chainload", 9) == 0)
					{
						current_entry->type = ENTRY_TYPE_CHAINLD;
					}
				}

				// Extract protocol specific keys
				switch (current_entry->type)
				{
					case ENTRY_TYPE_LINUX:
					{
						// Extract key: kernel
						if (memcmp(lstart, "kernel", 6) == 0)
							EXTRACT_STR(CHAR16, current_entry->kernel_path)

						// Extract key: module
						else if (memcmp(lstart, "module", 6) == 0)
							EXTRACT_STR(CHAR16, current_entry->module_path)

						// Extract key: cmdline
						else if (memcmp(lstart, "cmdline", 7) == 0)
							EXTRACT_STR(CHAR8, current_entry->cmdline)

						break;
					}

					case ENTRY_TYPE_CHAINLD:
					{
						// Extract key: efiapp
						if (memcmp(lstart, "efiapp", 6) == 0)
							EXTRACT_STR(CHAR16, current_entry->efi_path)

						break;
					}

					default: break;
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
	*global  = config_global;

	return ERR_OK;
}

#undef EXTRACT_STR

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
