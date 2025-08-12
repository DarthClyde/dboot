#include "protos/chainload.h"

#include <efilib.h>

#include "fs/fs.h"

error_t chainload_boot(path_t* efi_path)
{
	EFI_STATUS status            = EFI_SUCCESS;
	error_t error                = ERR_OK;

	EFI_HANDLE efi_handle        = NULL;
	EFI_DEVICE_PATH* efi_devpath = NULL;

	// Open the EFI application
	Print(L"Loading efiapp: %s\n", efi_path->path);
	error = fs_setpart(efi_path->type, efi_path->mod);
	ERR_CHECK(error, END);

	// Get the EFI app device path
	efi_devpath = FileDevicePath(fs_getpart()->handle, efi_path->path);
	if (!efi_devpath) goto end;

	// Load image from EFI buffer
	status = uefi_call_wrapper(BS->LoadImage, 6, FALSE, part_get_boot_rawhndl(), efi_devpath, NULL,
	                           0, &efi_handle);
	if (EFI_ERROR(status))
	{
		ERR_PRINT_STR(L"Failed to load the EFI app image");
		goto end;
	}

	// Leaving so soon :(
	uefi_call_wrapper(BS->StartImage, 3, efi_handle, NULL, NULL);

	// Wow... back already
	// ♪ ┏(°.°)┛┗(°.°)┓┗(°.°)┛┏(°.°)┓ ♪
	Print(L"[chainload] Chainloading failed\n");

end:
	mem_free_pool(efi_devpath);

	error = ERR_BOOT_FAIL_CHAINLOAD;
	return error;
}
