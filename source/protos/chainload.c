#include "protos/chainload.h"

#include <efilib.h>

#include "fs/fs.h"

error_t chainload_boot(path_t* efi_path)
{
	EFI_STATUS status     = EFI_SUCCESS;
	error_t error         = ERR_OK;
	file_t* file          = NULL;

	UINT8* efi_buf        = NULL;
	UINTN efi_size        = 0;
	EFI_HANDLE efi_handle = NULL;

	// Open the EFI application
	Print(L"Loading efiapp: %s\n", efi_path->path);
	error = fs_setpart(efi_path->type, efi_path->mod);
	ERR_CHECK(error, END);
	error = fs_file_open(efi_path->path, &file);
	ERR_CHECK(error, END);

	// Get the EFI app file size
	error = fs_file_getsize(file, &efi_size);
	ERR_CHECK(error, END);

	// Allocate EFI app memory
	efi_buf = mem_alloc_pool(efi_size);
	if (!efi_buf)
	{
		ERR_PRINT_STR(L"Failed to allocate EFI app memory");
		goto end;
	}

	// Read EFI app into allocated memory
	error = fs_file_readr(file, efi_size, (VOID*)efi_buf);
	ERR_CHECK(error, END);
	fs_file_close(file);

	// Load image from EFI buffer
	status = uefi_call_wrapper(BS->LoadImage, 6, FALSE, part_get_boot()->handle, NULL, efi_buf,
	                           efi_size, &efi_handle);
	if (EFI_ERROR(status)) goto end;
	mem_free_pool(efi_buf);

	uefi_call_wrapper(BS->StartImage, 3, efi_handle, NULL, NULL);

end:
	error = ERR_BOOT_FAIL_CHAINLOAD;
	return error;
}
