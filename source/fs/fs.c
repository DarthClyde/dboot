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

EFI_STATUS fs_file_read(file_t* file, UINTN* size, VOID* buffer)
{
	if (!file) return EFI_INVALID_PARAMETER;
	return uefi_call_wrapper(file->handle->Read, 3, file->handle, size, buffer);
}

EFI_STATUS fs_file_readr(file_t* file, UINTN size, VOID* buffer)
{
	if (!file) return EFI_INVALID_PARAMETER;
	return uefi_call_wrapper(file->handle->Read, 3, file->handle, &size, buffer);
}

EFI_STATUS fs_file_readall(file_t* file, VOID** buffer, UINTN* size)
{
	if (!file) return EFI_INVALID_PARAMETER;
	if (*buffer) return EFI_INVALID_PARAMETER;

	EFI_STATUS status = EFIERR(99);
	*size             = 0;

	// Get file size
	status = fs_file_getsize(file, size);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to get file size: %d\n", status);
		goto end;
	}

	// Allocate buffer for file
	*buffer = AllocatePool(*size);
	if (!*buffer)
	{
		Print(L"Failed to allocate buffer for file: %d\n", status);
		goto end;
	}

	// Read file content
	status = uefi_call_wrapper(file->handle->Read, 3, file->handle, &(*size), *buffer);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to read file: %d\n", status);
		goto end;
	}

end:
	return status;
}

EFI_STATUS fs_file_setpos(file_t* file, UINTN pos)
{
	if (!file) return EFI_INVALID_PARAMETER;
	return uefi_call_wrapper(file->handle->SetPosition, 2, file->handle, pos);
}

EFI_STATUS fs_file_getsize(file_t* file, UINTN* size)
{
	if (!file) return EFI_INVALID_PARAMETER;

	EFI_FILE_INFO* info;

	info = LibFileInfo(file->handle);
	if (!info) return EFI_NOT_FOUND;

	*size = info->FileSize;

	FreePool(info);
	return EFI_SUCCESS;
}

EFI_STATUS fs_file_open(EFI_LOADED_IMAGE* image, CHAR16* path, file_t** file)
{
	EFI_STATUS status = EFIERR(99);

	*file             = NULL;
	file_t* newfile   = NULL;

	// Allocate file
	newfile = AllocatePool(sizeof(file_t));
	if (!newfile)
	{
		Print(L"Failed to allocate file\n");
		status = EFI_BAD_BUFFER_SIZE;
		goto end;
	}

	// Open the root volume
	newfile->root = LibOpenRoot(image->DeviceHandle);
	if (!newfile->root)
	{
		Print(L"Failed to open root volume\n");
		status = EFI_LOAD_ERROR;
		goto end;
	}

	// Open the file
	status = uefi_call_wrapper(newfile->root->Open, 5, newfile->root, &newfile->handle, path,
	                           EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status))
	{
		Print(L"Failed to open file: %d\n", status);
		goto end;
	}

end:
	if (EFI_ERROR(status)) fs_file_close(newfile);
	else *file = newfile;

	return status;
}

EFI_STATUS fs_file_close(file_t* file)
{
	if (!file) return EFI_INVALID_PARAMETER;

	EFI_STATUS status = EFIERR(99);

	// Close and free
	status = uefi_call_wrapper(file->handle->Close, 1, file->handle);
	status += uefi_call_wrapper(file->root->Close, 1, file->root);

	if (!EFI_ERROR(status))
	{
		FreePool(file);
		file = NULL;
	}

	return status;
}
