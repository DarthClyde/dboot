#include "fs/fs.h"

#include <efilib.h>

static EFI_LOADED_IMAGE* s_loaded_image = NULL;

error_t fs_load_image(EFI_HANDLE image)
{
	EFI_STATUS status =
	    uefi_call_wrapper(BS->OpenProtocol, 6, image, &LoadedImageProtocol, (VOID**)&s_loaded_image,
	                      image, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	if (EFI_ERROR(status)) return ERR_FS_IMGLD_FAIL;
	return ERR_OK;
}

EFI_LOADED_IMAGE* fs_get_image(void)
{
	return s_loaded_image;
}

error_t fs_file_read(file_t* file, UINTN* size, VOID* buffer)
{
	if (!file) return ERR_FS_FILE_INVALID;
	EFI_STATUS status = uefi_call_wrapper(file->handle->Read, 3, file->handle, size, buffer);

	if (EFI_ERROR(status)) return ERR_FS_FILE_READ;
	return ERR_OK;
}

error_t fs_file_readr(file_t* file, UINTN size, VOID* buffer)
{
	if (!file) return ERR_FS_FILE_INVALID;
	EFI_STATUS status = uefi_call_wrapper(file->handle->Read, 3, file->handle, &size, buffer);

	if (EFI_ERROR(status)) return ERR_FS_FILE_READ;
	return ERR_OK;
}

error_t fs_file_readall(file_t* file, VOID** buffer, UINTN* size)
{
	if (!file) return ERR_FS_FILE_INVALID;
	if (*buffer) return ERR_INVALID_PARAM;

	EFI_STATUS status = EFI_SUCCESS;
	error_t error     = ERR_OK;

	*buffer           = NULL;
	*size             = 0;

	// Get file size
	error = fs_file_getsize(file, size);
	if (error) goto end;

	// Allocate buffer for file
	*buffer = mem_alloc_pool(*size);
	if (!*buffer)
	{
		error = ERR_ALLOC_FAIL;
		goto end;
	}

	// Read file content
	status = uefi_call_wrapper(file->handle->Read, 3, file->handle, &(*size), *buffer);
	if (EFI_ERROR(status)) error = ERR_FS_FILE_READ;

end:
	return error;
}

error_t fs_file_setpos(file_t* file, UINTN pos)
{
	if (!file) return ERR_FS_FILE_INVALID;
	EFI_STATUS status = uefi_call_wrapper(file->handle->SetPosition, 2, file->handle, pos);

	if (EFI_ERROR(status)) return ERR_FS_FILE_SETPOS;
	return ERR_OK;
}

error_t fs_file_getsize(file_t* file, UINTN* size)
{
	if (!file) return ERR_FS_FILE_INVALID;

	EFI_FILE_INFO* info;

	info = LibFileInfo(file->handle);
	if (!info) return ERR_FS_FILE_FILEINFO;

	*size = info->FileSize;

	mem_free_pool(info);
	return ERR_OK;
}

error_t fs_file_open(EFI_LOADED_IMAGE* image, CHAR16* path, file_t** file)
{
	EFI_STATUS status = EFI_SUCCESS;
	error_t error     = ERR_OK;

	*file             = NULL;
	file_t* newfile   = NULL;

	// Allocate file
	newfile = AllocateZeroPool(sizeof(file_t));
	if (!newfile)
	{
		error = ERR_ALLOC_FAIL;
		goto end;
	}

	// Open the root volume
	newfile->root = LibOpenRoot(image->DeviceHandle);
	if (!newfile->root)
	{
		error = ERR_FS_ROOTLD_FAIL;
		goto end;
	}

	// Open the file
	status = uefi_call_wrapper(newfile->root->Open, 5, newfile->root, &newfile->handle, path,
	                           EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status)) error = ERR_FS_FILE_READ;

end:
	if (error) fs_file_close(newfile);
	else *file = newfile;

	return error;
}

error_t fs_file_close(file_t* file)
{
	if (!file) return ERR_FS_FILE_INVALID;

	EFI_STATUS status = EFI_SUCCESS;
	error_t error     = ERR_OK;

	// Close
	if (file->handle)
	{
		status = uefi_call_wrapper(file->handle->Close, 1, file->handle);
		if (EFI_ERROR(status)) error = ERR_FS_FILE_CLOSE;
	}
	if (file->root)
	{
		status = uefi_call_wrapper(file->root->Close, 1, file->root);
		if (EFI_ERROR(status)) error = ERR_FS_FILE_CLOSE;
	}

	// Free
	mem_free_pool(file);
	file = NULL;

	return error;
}
