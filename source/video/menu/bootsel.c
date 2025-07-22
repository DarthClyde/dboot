#include "video/menu/bootsel.h"

#include <efilib.h>

#include "video/gop.h"
#include "utils/input.h"

#define DEFAULT_COLOR            EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)
#define ACCENT_COLOR             EFI_TEXT_ATTR(EFI_CYAN, EFI_BLACK)
#define HIGHLIGHT_COLOR          EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY)

#define BOOT_TIMEOUT_CLEARFOOTER -1
#define BOOT_TIMEOUT_DISABLED    -2

#define MENU_ITER_START                                   \
	while (s_menu_iter != NULL)                           \
	{                                                     \
		if (s_menu_iter->title == NULL) goto mov_next;    \
		if (s_menu_iter->is_child == TRUE) goto mov_next;

#define MENU_ITER_END                \
mov_next:                            \
	s_menu_iter = s_menu_iter->next; \
	i++;                             \
	}

typedef struct menu_item
{
	CHAR16* title;
	UINTN config_index;

	BOOLEAN is_child;
	struct menu_item* child_next;

	struct menu_item* next;
} menu_item_t;

static UINTN s_columns           = 0;
static UINTN s_rows              = 0;
static UINTN s_top               = 0;

static menu_item_t* s_menu_first = NULL;
static menu_item_t* s_menu_iter  = NULL;

static CHAR16* s_clearline       = NULL;

static CHAR16* s_footer          = NULL;
static UINT16 s_footer_strlen    = 0;

static UINT8 s_selected          = 3;
static UINT8 s_sel_index_min     = 0;
static UINT8 s_sel_index_max     = 0;

static INT64 s_boot_timeout      = 5 * 10;

inline static VOID draw_static_elements(void)
{
	// Draw header
	static UINT16 header_str[] = L"dboot " DBOOT_VERSION;
	gop_printp((s_columns - StrLen(header_str)) / 2, s_top, header_str);

	// Draw help
	{
		static const UINTN help_strlen = 58;
		uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut,
		                  (s_columns - help_strlen) / 2, s_top + 1);

		gop_printc(ACCENT_COLOR, L"UP/DOWN");
		gop_printc(DEFAULT_COLOR, L" - Move");
		gop_print(L"    ");
		gop_printc(ACCENT_COLOR, L"ENTER");
		gop_printc(DEFAULT_COLOR, L" - Select");
		gop_print(L"    ");
		gop_printc(ACCENT_COLOR, L"E");
		gop_printc(DEFAULT_COLOR, L" - Edit");
		gop_print(L"    ");
		gop_printc(ACCENT_COLOR, L"C");
		gop_printc(DEFAULT_COLOR, L" - Custom");
	}
}

inline static VOID create_item_title(CHAR16** title, config_entry_t* entry)
{
	// Allocate title string
	*title = AllocatePool((MAX_TITLE_LEN + 1) * sizeof(CHAR16));

	// Fill title with whitespace
	for (UINTN j = 0; j < MAX_TITLE_LEN; j++)
	{
		(*title)[j] = ' ';
	}

	// If entry is a group, add tag for it
	if (entry->type == ENTRY_TYPE_GROUP)
	{
		StrnCpy(&(*title)[0], L"[+]", 3);
	}

	// Copy entry name to title
	UINTN name_len = StrLen(entry->name);
	if (name_len > MAX_NAME_LEN) name_len = MAX_NAME_LEN + 3;
	UINTN offset = (MAX_TITLE_LEN - name_len) / 2;

	for (UINTN j = 0; j < name_len; j++)
	{
		// Add '...' if (j + 1 > max length)
		if (j >= MAX_NAME_LEN)
		{
			StrnCpy(&(*title)[j + offset], L"...", 3);
			break;
		}

		(*title)[j + offset] = entry->name[j];
	}

	// Add string terminator
	(*title)[MAX_TITLE_LEN] = '\0';
}

inline static VOID populate_item(menu_item_t* item, config_entry_t* entry, UINTN i)
{
	create_item_title(&item->title, entry);
	item->config_index = i;

	item->child_next   = NULL;
	item->is_child     = FALSE;

	item->next         = NULL;
}

inline static VOID create_menu_items(config_entry_t* entries, UINTN entries_count)
{
	menu_item_t* last_item  = NULL;
	menu_item_t* last_child = NULL;

	// Create all menu items
	for (UINTN i = 0; i < entries_count; i++)
	{
		// Skip entries that are children
		if (CompareMem(entries[i].name, entries[i].parent_name, StrLen(entries[i].name)) != 0)
			continue;

		// Create current item
		s_menu_iter = AllocatePool(sizeof(menu_item_t));
		populate_item(s_menu_iter, &entries[i], i);

		// Handle groups
		if (entries[i].type == ENTRY_TYPE_GROUP)
		{
			// Find children of group
			CHAR16* parent_name   = entries[i].name;
			UINTN parent_name_len = StrLen(parent_name);
			for (UINTN j = 0; j < entries_count; j++)
			{
				if (j == i) continue;

				if (CompareMem(parent_name, entries[j].parent_name, parent_name_len) == 0)
				{
					// Create child
					menu_item_t* child = AllocatePool(sizeof(menu_item_t));
					populate_item(child, &entries[j], j);
					child->is_child = TRUE;

					if (s_menu_iter->child_next == NULL) s_menu_iter->child_next = child;
					if (last_child) last_child->child_next = child;

					last_child = child;
				}
			}
		}

		// Link last item to current
		if (!s_menu_first) s_menu_first = s_menu_iter;
		if (last_item) last_item->next = s_menu_iter;
		last_item = s_menu_iter;

		// Stop if at end of entries
		if ((i + 1) == entries_count) break;
	}

	// Set iterator to first item
	s_menu_iter = s_menu_first;
}

inline static menu_item_t* get_selected_item(UINT8 index)
{
	UINTN i                     = 0;
	menu_item_t* prev_menu_iter = s_menu_iter;

	// Iterate in the same way as the drawing loop to ensure index match
	s_menu_iter = s_menu_first;
	MENU_ITER_START
	{
		if (i == index) break;
	}
	MENU_ITER_END

	// Check that index was actually found
	if (i != index) return NULL;

	// Restore previous iterator and return
	menu_item_t* item = s_menu_iter;
	s_menu_iter       = prev_menu_iter;
	return item;
}

EFI_STATUS bootsel_run(UINT8* sel, config_entry_t* entries, UINTN entries_count)
{
	EFI_STATUS status                  = EFIERR(99);
	SIMPLE_TEXT_OUTPUT_INTERFACE* COUT = ST->ConOut;
	UINTN i                            = 0;

	if (!gop_isactive()) return BOOTSEL_RET_ERROR;

	// Setup menu
	uefi_call_wrapper(COUT->Reset, 2, COUT, FALSE);
	uefi_call_wrapper(COUT->EnableCursor, 2, COUT, FALSE);
	uefi_call_wrapper(COUT->SetCursorPosition, 3, COUT, 0, 0);
	uefi_call_wrapper(COUT->SetAttribute, 2, COUT, DEFAULT_COLOR);

	// Clear the screen
	uefi_call_wrapper(COUT->OutputString, 2, COUT, L" ");
	uefi_call_wrapper(COUT->ClearScreen, 1, COUT);

	// Query the screen size
	status = uefi_call_wrapper(COUT->QueryMode, 4, COUT, COUT->Mode->Mode, &s_columns, &s_rows);
	if (EFI_ERROR(status))
	{
		// Fallback size
		s_columns = 80;
		s_rows    = 25;
	}
	s_top = COUT->Mode->CursorRow;

	// Create clearline string
	s_clearline = AllocatePool((s_columns + 1) * sizeof(CHAR16));
	for (i = 0; i < s_columns; i++) s_clearline[i] = ' ';
	s_clearline[s_columns] = '\0';

	draw_static_elements();

	create_menu_items(entries, entries_count);

	// Menu loop
	while (TRUE)
	{
		// Draw entries menu
		i = 0;
		MENU_ITER_START
		{
			// Set colors
			uefi_call_wrapper(COUT->SetAttribute, 2, COUT,
			                  i == s_selected ? HIGHLIGHT_COLOR : DEFAULT_COLOR);

			// Print entry title
			gop_printp((s_columns - MAX_TITLE_LEN) / 2, s_top + 4 + i, s_menu_iter->title);
		}
		MENU_ITER_END
		s_sel_index_max = i - 1;
		s_menu_iter     = s_menu_first;

		// Draw footer string
		if (s_footer)
			gop_printpc((s_columns - s_footer_strlen) / 2, s_rows - 1, DEFAULT_COLOR, s_footer);

		// Handle key events
		key_t key = input_get_keypress();
		if (s_boot_timeout != BOOT_TIMEOUT_DISABLED && key)
			s_boot_timeout = BOOT_TIMEOUT_CLEARFOOTER;

		switch (key)
		{
			// Shutdown system
			case KEY(0, 'Q'):
			{
				status = EFI_ABORTED;
				goto end;
			}

			// Move selection up
			case KEY(SCAN_UP, 0):
			{
				if (s_selected == s_sel_index_min) s_selected = s_sel_index_max;
				else s_selected--;
				break;
			}

			// Move selection down
			case KEY(SCAN_DOWN, 0):
			{
				if (s_selected == s_sel_index_max) s_selected = s_sel_index_min;
				else s_selected++;
				break;
			}

			// Move selection to first entry
			case KEY(SCAN_HOME, 0):
			{
				s_selected = s_sel_index_min;
				break;
			}

			// Move selection to last entry
			case KEY(SCAN_END, 0):
			{
				s_selected = s_sel_index_max;
				break;
			}

			// Select current entry
			case KEY(0, CHAR_LINEFEED):
			case KEY(0, CHAR_CARRIAGE_RETURN):
			{
				s_selected = get_selected_item(s_selected)->config_index;
				if (entries[s_selected].type == ENTRY_TYPE_GROUP)
				{
					// TODO: Handle groups
				}
				else
				{
					status = EFI_SUCCESS;
					goto end;
				}
			}
		}

		// Update boot timeout
		if (s_boot_timeout > 0)
		{
			if (s_footer) FreePool(s_footer);
			s_footer        = PoolPrint(L"Booting in %lld seconds.", (s_boot_timeout + 10) / 10);
			s_footer_strlen = StrLen(s_footer);

			uefi_call_wrapper(BS->Stall, 1, 100 * 1000);
			s_boot_timeout--;
		}

		// Clear footer and disable boot timeout
		else if (s_boot_timeout == BOOT_TIMEOUT_CLEARFOOTER)
		{
			if (s_footer) FreePool(s_footer);
			s_footer        = NULL;
			s_footer_strlen = 0;

			gop_printpc(0, s_rows - 1, DEFAULT_COLOR, s_clearline + 1);

			s_boot_timeout = BOOT_TIMEOUT_DISABLED;
		}

		// Boot into default entry
		else if (s_boot_timeout == 0)
		{
			s_selected = get_selected_item(s_selected)->config_index;
			// TODO: Should check that this is not a group
			status = EFI_SUCCESS;
			goto end;
		}
	}

end:
	if (s_clearline) FreePool(s_clearline);
	if (s_footer) FreePool(s_footer);
	// TODO: The menu items list is not freed

	*sel = s_selected;
	return status;
}

// TODO: Put these debug functions behind a DEBUG flag
#if 0
VOID bootsel_debuglog(config_entry_t* entries, UINTN entries_count)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE* COUT = ST->ConOut;

	uefi_call_wrapper(COUT->Reset, 2, COUT, FALSE);
	uefi_call_wrapper(COUT->EnableCursor, 2, COUT, FALSE);
	uefi_call_wrapper(COUT->SetCursorPosition, 3, COUT, 0, 0);
	uefi_call_wrapper(COUT->SetAttribute, 2, COUT, DEFAULT_COLOR);

	uefi_call_wrapper(COUT->OutputString, 2, COUT, L" ");
	uefi_call_wrapper(COUT->ClearScreen, 1, COUT);

	create_menu_items(entries, entries_count);

	menu_item_t* last_real_item = NULL;
	while (s_menu_iter != NULL)
	{
		gop_print(L"---- [%d][%p] %s ----\n", s_menu_iter->config_index, s_menu_iter,
		          entries[s_menu_iter->config_index].name);
		gop_print(L"IsChild: %c\n", s_menu_iter->is_child ? 'y' : 'n');
		gop_print(L"NextChild: %p -- Next: %p\n", s_menu_iter->child_next, s_menu_iter->next);

		if (s_menu_iter->child_next)
		{
			if (!last_real_item) last_real_item = s_menu_iter;
			s_menu_iter = s_menu_iter->child_next;
			continue;
		}
		else if (!s_menu_iter->child_next && last_real_item)
		{
			s_menu_iter    = last_real_item->next;
			last_real_item = NULL;
			continue;
		}

		s_menu_iter = s_menu_iter->next;
	}

	gop_print(L"---- Done ----");
}
#endif