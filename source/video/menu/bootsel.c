#include "video/menu/bootsel.h"

#include <efilib.h>

#include "video/gop.h"
#include "utils/input.h"

#define DEFAULT_COLOR            EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK)
#define ACCENT_COLOR             EFI_TEXT_ATTR(EFI_CYAN, EFI_BLACK)
#define HIGHLIGHT_COLOR          EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY)

#define BOOT_TIMEOUT_CLEARFOOTER -1
#define BOOT_TIMEOUT_DISABLED    -2

EFI_STATUS bootsel_run(UINT8* sel, config_entry_t* entries, UINTN entries_count)
{
	EFI_STATUS status                  = EFIERR(99);
	SIMPLE_TEXT_OUTPUT_INTERFACE* COUT = ST->ConOut;

	UINTN columns                      = 0;
	UINTN rows                         = 0;
	UINTN top                          = 0;

	CHAR16** entry_titles              = NULL;

	CHAR16* clearline                  = NULL;

	CHAR16* footer                     = NULL;
	UINT16 footer_strlen               = 0;

	UINT8 selection                    = 0;
	INT64 boot_timeout                 = 5 * 10;

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
	status = uefi_call_wrapper(COUT->QueryMode, 4, COUT, COUT->Mode->Mode, &columns, &rows);
	if (EFI_ERROR(status))
	{
		// Fallback size
		columns = 80;
		rows    = 25;
	}
	top = COUT->Mode->CursorRow;

	// Draw header
	static UINT16 header_str[] = L"dboot " DBOOT_VERSION;
	gop_printp((columns - StrLen(header_str)) / 2, top, header_str);

	// Draw help
	{
		static const UINTN help_strlen = 58;
		uefi_call_wrapper(COUT->SetCursorPosition, 3, COUT, (columns - help_strlen) / 2, top + 1);

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

	// Create clearline string
	clearline = AllocatePool((columns + 1) * sizeof(CHAR16));
	for (UINTN i = 0; i < columns; i++) clearline[i] = ' ';
	clearline[columns] = '\0';

	// Create entry titles
	entry_titles = AllocatePool(entries_count * sizeof(CHAR16*));
	for (UINTN i = 0; i < entries_count; i++)
	{
		CHAR16** title        = &entry_titles[i];
		config_entry_t* entry = &entries[i];

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

	// Menu loop
	while (TRUE)
	{
		// Draw entries menu
		for (UINTN i = 0; i < entries_count; i++)
		{
			CHAR16* title = entry_titles[i];
			if (title == NULL) continue;

			// Set colors
			if (i == selection) uefi_call_wrapper(COUT->SetAttribute, 2, COUT, HIGHLIGHT_COLOR);
			else uefi_call_wrapper(COUT->SetAttribute, 2, COUT, DEFAULT_COLOR);

			// Print entry title
			gop_printp((columns - MAX_TITLE_LEN) / 2, top + 4 + i, title);
		}

		// Draw footer string
		if (footer) gop_printpc((columns - footer_strlen) / 2, rows - 1, DEFAULT_COLOR, footer);

		// Handle key events
		key_t key = input_get_keypress();
		if (boot_timeout != BOOT_TIMEOUT_DISABLED && key) boot_timeout = BOOT_TIMEOUT_CLEARFOOTER;
		switch (key)
		{
			case KEY(0, 'Q'):
			{
				status = EFI_ABORTED;
				goto end;
			}

			// Move selection up
			case KEY(SCAN_UP, 0):
			{
				if (selection == 0) selection = (entries_count - 1);
				else selection--;
				break;
			}

			// Move selection down
			case KEY(SCAN_DOWN, 0):
			{
				if (selection == (entries_count - 1)) selection = 0;
				else selection++;
				break;
			}

			// Move selection to first entry
			case KEY(SCAN_HOME, 0):
			{
				selection = 0;
				break;
			}

			// Move selection to last entry
			case KEY(SCAN_END, 0):
			{
				selection = (entries_count - 1);
				break;
			}

			// Select current entry
			case KEY(0, CHAR_LINEFEED):
			case KEY(0, CHAR_CARRIAGE_RETURN):
			{
				status = EFI_SUCCESS;
				goto end;
			}
		}

		// Update boot timeout
		if (boot_timeout > 0)
		{
			if (footer) FreePool(footer);
			footer        = PoolPrint(L"Booting in %lld seconds.", (boot_timeout + 10) / 10);
			footer_strlen = StrLen(footer);

			uefi_call_wrapper(BS->Stall, 1, 100 * 1000);
			boot_timeout--;
		}

		// Clear footer and disable boot timeout
		else if (boot_timeout == BOOT_TIMEOUT_CLEARFOOTER)
		{
			if (footer) FreePool(footer);
			footer        = NULL;
			footer_strlen = 0;

			gop_printpc(0, rows - 1, DEFAULT_COLOR, clearline + 1);

			boot_timeout = BOOT_TIMEOUT_DISABLED;
		}

		// Boot into default entry
		else if (boot_timeout == 0)
		{
			status = EFI_SUCCESS;
			goto end;
		}
	}

end:
	if (clearline) FreePool(clearline);
	for (UINTN i = 0; i < entries_count; i++) FreePool(entry_titles[i]);
	if (entry_titles) FreePool(entry_titles);
	if (footer) FreePool(footer);

	*sel = selection;
	return status;
}