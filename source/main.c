#include <efi.h>
#include <efilib.h>

#include "fs/fs.h"
#include "fs/config.h"

#include "video/gop.h"
#include "video/menu/bootsel.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* systable)
{
	EFI_STATUS status              = EFIERR(99);

	config_entry_t* config_entries = NULL;
	UINTN config_entries_count     = 0;

	UINTN selected_entry           = UINT64_MAX;

	InitializeLib(image, systable);

	// Clear the screen
	uefi_call_wrapper(ST->ConOut->Reset, 2, ST->ConOut, FALSE);
	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	// Print dboot info
	Print(L"dboot v" DB_VERSION " compiled "__DATE__
	      " at "__TIME__);
	Print(L"\n\n");

	// Load root image
	status = fs_load_image(image);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to load root image.\n");
		goto wait_exit;
	}

	// Load dboot config
	status = config_load(&config_entries, &config_entries_count);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to load dboot config file.\n");
		goto wait_exit;
	}
#ifdef DB_DEBUG
	config_debuglog(config_entries, config_entries_count);
#endif

	// Start the GOP
	status = gop_init();
	if (EFI_ERROR(status))
	{
		Print(L"Failed to start GOP\n");
		goto wait_exit;
	}

	// Display boot selector menu
	status = bootsel_run(&selected_entry, config_entries, config_entries_count);

	// Handle results from boot selector menu
	if (status == EFI_ABORTED)
	{
		uefi_call_wrapper(RT->ResetSystem, 4, EfiResetShutdown, EFI_ABORTED, 0, NULL);
	}
	else if (status == EFI_ABORTED + 100)
	{
		return EFI_ABORTED;
	}
	else if (status == EFI_SUCCESS)
	{
		uefi_call_wrapper(ST->ConOut->Reset, 2, ST->ConOut, FALSE);
		uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
		gop_printpc(0, 0, EFI_YELLOW | EFI_BACKGROUND_BLUE, L"Booting into entry #%d",
		            selected_entry);
		goto wait_exit;
	}

	return EFI_SUCCESS;

wait_exit:
	Print(L"\n\nPress any key to exit...\n");
	WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
	return status;
}
