#include "hdr.h"
#include <efilib.h>

#include "fs/fs.h"
#include "fs/config.h"

#include "video/gop.h"
#include "video/menu/bootsel.h"

#include "protos/boot.h"

EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE* systable)
{
	error_t error                  = ERR_OK;

	config_entry_t* config_entries = NULL;
	UINTN config_entries_count     = 0;

	UINTN selected_entry           = UINT64_MAX;
	UINT8 bootsel_retval           = BOOTSEL_RET_UNKNOWN;

	InitializeLib(image, systable);

	// Clear the screen
	uefi_call_wrapper(ST->ConOut->Reset, 2, ST->ConOut, FALSE);
	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	// Print dboot info
	Print(L"dboot v" DB_VERSION " compiled "__DATE__
	      " at "__TIME__);
	Print(L"\n\n");

	// Load root image
	error = fs_load_image(image);
	ERR_CHECK(error, END);

	// Load dboot config
	error = config_load(&config_entries, &config_entries_count);
	if (error)
	{
		ERR_PRINT_STR(L"Failed to load dboot config file");
		goto end;
	}
#ifdef DB_DEBUG
	config_debuglog(config_entries, config_entries_count);
#endif

	// Start the GOP
	error = gop_init();
	ERR_CHECK(error, END);

	// Run and handle boot selector menu
	bootsel_retval = bootsel_run(&selected_entry, config_entries, config_entries_count);
	switch (bootsel_retval)
	{
		case BOOTSEL_RET_SHUTDOWN:
		{
			uefi_call_wrapper(RT->ResetSystem, 4, EfiResetShutdown, EFI_SUCCESS, 0, NULL);
			break;
		}
		case BOOTSEL_RET_TOFWUI:
		{
			EFI_GUID guid = EFI_GLOBAL_VARIABLE;
			UINT64 osind  = 0;
			UINTN size    = 0;

			// Get existing OsIndications variable
			uefi_call_wrapper(RT->GetVariable, 5, L"OsIndications", &guid, NULL, &size, &osind);

			// Set new OsIndications variable
			if (osind) osind |= EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
			else osind = EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
			uefi_call_wrapper(RT->SetVariable, 5, L"OsIndications", &guid,
			                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS
			                      | EFI_VARIABLE_RUNTIME_ACCESS,
			                  sizeof(osind), &osind);

			uefi_call_wrapper(RT->ResetSystem, 4, EfiResetWarm, EFI_SUCCESS, 0, NULL);
			break;
		}

		case BOOTSEL_RET_BOOT:
		{
			uefi_call_wrapper(ST->ConOut->Reset, 2, ST->ConOut, FALSE);
			uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
			uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut, EFI_YELLOW);
			gop_printp(0, 0, L"Booting into entry #%d\n\n", selected_entry);

			// Boot into entry
			error = boot_boot(&config_entries[selected_entry]);
			ERR_CHECK(error, END);
		}

		default: goto end;
	}

end:
	Print(L"\n\nPress any key to exit...\n");
	WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
	return EFI_SUCCESS;
}
