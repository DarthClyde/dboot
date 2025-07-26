#include "fs/fs.h"

#include <efilib.h>

static EFI_LOADED_IMAGE* s_loaded_image = NULL;

EFI_STATUS fs_load_image(EFI_HANDLE image)
{
	EFI_STATUS status = EFIERR(99);

	status =
	    uefi_call_wrapper(BS->OpenProtocol, 6, image, &LoadedImageProtocol, (VOID**)&s_loaded_image,
	                      image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to load root image.\n");
		goto end;
	}

	if (s_loaded_image) status = EFI_SUCCESS;

end:
	return status;
}

EFI_LOADED_IMAGE* fs_get_image(void)
{
	return s_loaded_image;
}

EFI_STATUS fs_load_file(CHAR16* path, VOID** buffer, UINTN* size)
{
	EFI_STATUS status        = EFIERR(99);

	EFI_FILE_HANDLE root     = NULL;
	EFI_FILE_HANDLE file     = NULL;

	EFI_FILE_INFO* file_info = NULL;

	*buffer                  = NULL;
	*size                    = 0;

	// Open the root volume
	root = LibOpenRoot(s_loaded_image->DeviceHandle);
	if (root == NULL)
	{
		Print(L"Failed to open root volume\n");
		status = EFI_LOAD_ERROR;
		goto end;
	}

	// Open the file
	status = uefi_call_wrapper(root->Open, 5, root, &file, path, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to open file: %d\n", status);
		goto end;
	}

	// Get file information
	file_info = LibFileInfo(file);
	if (!file_info)
	{
		Print(L"Failed to get file info\n");
		status = EFI_COMPROMISED_DATA;
		goto end;
	}
	*size = file_info->FileSize;

	// Allocate buffer for file
	*buffer = AllocatePool(*size);
	if (!*buffer)
	{
		Print(L"Failed to allocate buffer for file: %d\n", status);
		goto end;
	}

	// Read file content
	status = uefi_call_wrapper(file->Read, 3, file, &(*size), *buffer);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to read file: %d\n", status);
		goto end;
	}

end:
	if (file_info) FreePool(file_info);
	if (file) uefi_call_wrapper(file->Close, 1, file);
	if (root) uefi_call_wrapper(root->Close, 1, root);

	return status;
}
