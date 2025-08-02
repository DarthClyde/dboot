#include "video/menu/bootsel.h"

#include <efilib.h>

#include "video/gop.h"
#include "utils/input.h"

#define DEFAULT_COLOR            EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)
#define ACCENT_COLOR             EFI_TEXT_ATTR(EFI_CYAN, EFI_BLACK)
#define HIGHLIGHT_COLOR          EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY)

#define BOOT_TIMEOUT_CLEARFOOTER -1
#define BOOT_TIMEOUT_DISABLED    -2

#define GROUP_DEPTH_NONE         -1

typedef struct menu_item
{
	CHAR16* title;
	UINTN config_index;

	INTN group_depth;
	BOOLEAN is_expanded;

	BOOLEAN is_child;
	struct menu_item* parent;

	struct menu_item* prev;
	struct menu_item* next;
} menu_item_t;

static UINTN s_columns           = 0;
static UINTN s_rows              = 0;
static UINTN s_top               = 0;

static menu_item_t* s_menu_first = NULL;
static menu_item_t* s_menu_last  = NULL;
static menu_item_t* s_menu_iter  = NULL;
static menu_item_t* s_selected   = NULL;

static CHAR16* s_clearline       = NULL;

static CHAR16* s_footer          = NULL;
static UINT16 s_footer_strlen    = 0;

static INT64 s_boot_timeout      = 5 * 10;

inline static VOID draw_static_elements(void)
{
	// Draw header
	static UINT16 header_str[] = L"dboot " DB_VERSION;
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

inline static VOID setup_defaults(config_global_t* global)
{
	// Boot timeout
	if (global->timeout > 0) s_boot_timeout = global->timeout * 10;
	else s_boot_timeout = BOOT_TIMEOUT_DISABLED;
}

inline static VOID create_item_title(CHAR16** title, config_entry_t* entry)
{
	// Allocate title string
	*title = mem_alloc_pool((MAX_TITLE_LEN + 1) * sizeof(CHAR16));

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

	item->group_depth  = GROUP_DEPTH_NONE;
	item->is_expanded  = FALSE;

	item->is_child     = FALSE;
	item->parent       = NULL;

	item->prev         = NULL;
	item->next         = NULL;
}

inline static VOID create_menu_items(config_entry_t* entries, UINTN entries_count,
                                     config_global_t* global)
{
	menu_item_t* last_item  = NULL;
	menu_item_t* last_child = NULL;

	// Create all menu items
	for (UINTN i = 0; i < entries_count; i++)
	{
		// Skip entries that are children
		if (entries[i].parent_name) continue;

		// Create current item
		s_menu_iter = mem_alloc_pool(sizeof(menu_item_t));
		populate_item(s_menu_iter, &entries[i], i);

		// Check if current item matches default
		if (StrCmp(entries[i].ident, global->default_entry) == 0)
		{
			s_selected = s_menu_iter;
		}

		// Handle groups
		if (entries[i].type == ENTRY_TYPE_GROUP)
		{
			INTN num_children = 0;

			// Find children of group
			for (UINTN j = 0; j < entries_count; j++)
			{
				if (j == i) continue;

				if (StrCmp(entries[i].name, entries[j].parent_name) == 0)
				{
					menu_item_t* child = mem_alloc_pool(sizeof(menu_item_t));
					populate_item(child, &entries[j], j);

					child->is_child = TRUE;
					child->parent   = s_menu_iter;

					// Check if current child matches default
					if (StrCmp(entries[j].ident, global->default_entry) == 0)
					{
						s_selected               = child;
						s_menu_iter->is_expanded = TRUE;
						s_menu_iter->title[1]    = '-';
					}

					// There is a previous child
					if (last_child)
					{
						child->prev      = last_child;
						last_child->next = child;
					}

					// This is the first child
					else
					{
						child->prev       = s_menu_iter;
						s_menu_iter->next = child;
					}

					last_child = child;
					num_children++;
				}
			}

			s_menu_iter->group_depth = num_children;
		}

		// This is the first item
		if (!s_menu_first) s_menu_first = s_menu_iter;

		// Link items
		if (last_item)
		{
			if (!s_menu_iter->prev) s_menu_iter->prev = last_item;
			if (!last_item->next) last_item->next = s_menu_iter;
		}

		// Set last item to current if not a group
		if (s_menu_iter->group_depth == GROUP_DEPTH_NONE) last_item = s_menu_iter;
		// Else: set last item to last child
		else last_item = last_child;

		// Stop if at end of entries
		if ((i + 1) == entries_count) break;
	}

	s_menu_last = s_menu_iter;
	s_menu_iter = s_menu_first;
}

UINT8 bootsel_run(UINTN* sel, config_entry_t* entries, UINTN entries_count, config_global_t* global)
{
	EFI_STATUS status                  = EFI_SUCCESS;
	UINT8 retval                       = BOOTSEL_RET_UNKNOWN;
	SIMPLE_TEXT_OUTPUT_INTERFACE* COUT = ST->ConOut;
	UINTN i                            = 0;

	if (!gop_isactive()) return BOOTSEL_RET_ERROR;

	create_menu_items(entries, entries_count, global);

	setup_defaults(global);

	if (!s_selected) s_selected = s_menu_iter;

redraw:
	// Setup menu
	uefi_call_wrapper(COUT->Reset, 2, COUT, FALSE);
	uefi_call_wrapper(COUT->EnableCursor, 2, COUT, FALSE);
	uefi_call_wrapper(COUT->SetCursorPosition, 3, COUT, 0, 0);
	uefi_call_wrapper(COUT->SetAttribute, 2, COUT, DEFAULT_COLOR);

	// Clear the screen
	uefi_call_wrapper(COUT->OutputString, 2, COUT, L" ");
	uefi_call_wrapper(COUT->ClearScreen, 1, COUT);

	// Query the screen size
	if (s_columns == 0)
	{
		status = uefi_call_wrapper(COUT->QueryMode, 4, COUT, COUT->Mode->Mode, &s_columns, &s_rows);
		if (EFI_ERROR(status))
		{
			// Fallback size
			s_columns = 80;
			s_rows    = 25;
		}

		s_top = COUT->Mode->CursorRow;

		// Create clearline string
		s_clearline = mem_alloc_pool((s_columns + 1) * sizeof(CHAR16));
		for (i = 0; i < s_columns; i++) s_clearline[i] = ' ';
		s_clearline[s_columns] = '\0';
	}

	draw_static_elements();

	// Menu loop
	while (TRUE)
	{
		// Draw entries menu
		i = 0;
		while (s_menu_iter != NULL)
		{
			if (!s_menu_iter->title) goto next;
			if (s_menu_iter->is_child && !s_menu_iter->parent->is_expanded) goto next;

			// Set colors
			uefi_call_wrapper(COUT->SetAttribute, 2, COUT,
			                  s_menu_iter == s_selected ? HIGHLIGHT_COLOR : DEFAULT_COLOR);

			// Print entry title
			gop_printp((s_columns - MAX_TITLE_LEN) / 2, s_top + 4 + i, s_menu_iter->title);
			i++;

next:
			s_menu_iter = s_menu_iter->next;
		}
		s_menu_iter = s_menu_first;

		// Draw footer string
		if (s_footer)
		{
			gop_printpc(0, s_rows - 1, DEFAULT_COLOR, s_clearline + 1);
			gop_printp((s_columns - s_footer_strlen) / 2, s_rows - 1, s_footer);
		}

		// Handle key events
		key_t key = input_get_keypress();
		if (s_boot_timeout != BOOT_TIMEOUT_DISABLED && key)
			s_boot_timeout = BOOT_TIMEOUT_CLEARFOOTER;

		switch (key)
		{
			// Shutdown system
			case KEY(0, 'Q'):
			{
				retval = BOOTSEL_RET_SHUTDOWN;
				goto end;
			}

			// Exit to EFI firmware UI
			case KEY(0, 's'):
			case KEY(0, 'S'):
			{
				retval = BOOTSEL_RET_TOFWUI;
				goto end;
			}

			// Move selection up
			case KEY(SCAN_UP, 0):
			{
				while (TRUE)
				{
					s_selected = s_selected->prev;

					// If at top, move to bottom
					if (!s_selected)
					{
						s_selected = s_menu_last;
						break;
					}

					// Check conditions if selection is a child
					if (s_selected->is_child)
					{
						if (s_selected->parent->is_expanded) break;
						else continue;
					}

					// Selection is valid
					break;
				}
				break;
			}

			// Move selection down
			case KEY(SCAN_DOWN, 0):
			{
				while (TRUE)
				{
					s_selected = s_selected->next;

					// If at bottom, move to top
					if (!s_selected)
					{
						s_selected = s_menu_first;
						break;
					}

					// Check conditions if selection is a child
					if (s_selected->is_child)
					{
						if (s_selected->parent->is_expanded) break;
						else continue;
					}

					// Selection is valid
					break;
				}
				break;
			}

			// Move selection to first entry
			case KEY(SCAN_HOME, 0):
			{
				s_selected = s_menu_first;
				break;
			}

			// Move selection to last entry
			case KEY(SCAN_END, 0):
			{
				s_selected = s_menu_last;
				break;
			}

			// Select current entry
			case KEY(0, CHAR_LINEFEED):
			case KEY(0, CHAR_CARRIAGE_RETURN):
			{
				if (s_selected->group_depth != GROUP_DEPTH_NONE)
				{
					s_selected->is_expanded = !s_selected->is_expanded;

					if (s_selected->is_expanded) s_selected->title[1] = '-';
					else s_selected->title[1] = '+';

					goto redraw;
				}
				else
				{
					retval = BOOTSEL_RET_BOOT;
					goto end;
				}
			}
		}

		// Update boot timeout
		if (s_boot_timeout > 0)
		{
			mem_free_pool(s_footer);
			s_footer        = PoolPrint(L"Booting in %lld seconds...", (s_boot_timeout + 10) / 10);
			s_footer_strlen = StrLen(s_footer);

			uefi_call_wrapper(BS->Stall, 1, 100 * 1000);
			s_boot_timeout--;
		}

		// Clear footer and disable boot timeout
		else if (s_boot_timeout == BOOT_TIMEOUT_CLEARFOOTER)
		{
			mem_free_pool(s_footer);
			s_footer        = NULL;
			s_footer_strlen = 0;

			gop_printpc(0, s_rows - 1, DEFAULT_COLOR, s_clearline + 1);

			s_boot_timeout = BOOT_TIMEOUT_DISABLED;
		}

		// Boot into default entry
		else if (s_boot_timeout == 0)
		{
			// TODO: Should check that this is not a group
			retval = BOOTSEL_RET_BOOT;
			goto end;
		}
	}

end:
	*sel = s_selected->config_index;

	mem_free_pool(s_clearline);
	mem_free_pool(s_footer);

	s_menu_iter = s_menu_last;
	while (s_menu_iter != NULL)
	{
		mem_free_pool(s_menu_iter->title);
		mem_free_pool(s_menu_iter->next);
		s_menu_iter = s_menu_iter->prev;
	}
	mem_free_pool(s_menu_first);

	return retval;
}

#ifdef DB_DEBUG
VOID bootsel_debuglog(config_entry_t* entries, UINTN entries_count, config_global_t* global)
{
	SIMPLE_TEXT_OUTPUT_INTERFACE* COUT = ST->ConOut;

	uefi_call_wrapper(COUT->Reset, 2, COUT, FALSE);
	uefi_call_wrapper(COUT->EnableCursor, 2, COUT, FALSE);
	uefi_call_wrapper(COUT->SetCursorPosition, 3, COUT, 0, 0);
	uefi_call_wrapper(COUT->SetAttribute, 2, COUT, DEFAULT_COLOR);

	uefi_call_wrapper(COUT->OutputString, 2, COUT, L" ");
	uefi_call_wrapper(COUT->ClearScreen, 1, COUT);

	create_menu_items(entries, entries_count, global);

	while (s_menu_iter != NULL)
	{
		gop_print(L"---- [%d][%p] %s ----\n", s_menu_iter->config_index, s_menu_iter,
		          entries[s_menu_iter->config_index].name);
		gop_print(L"Group Depth: %d\n", s_menu_iter->group_depth);
		gop_print(L"IsChild: %c -- Parent: %p\n", s_menu_iter->is_child ? 'y' : 'n',
		          s_menu_iter->parent);
		gop_print(L"Prev: %p -- Next: %p\n", s_menu_iter->prev, s_menu_iter->next);
		s_menu_iter = s_menu_iter->next;
	}

	gop_print(L"---- Done ----");
}
#endif
