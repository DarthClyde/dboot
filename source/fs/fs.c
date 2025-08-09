#include "fs/fs.h"
#include "efibind.h"

#include <efilib.h>

static partition_t* s_current_part = NULL;

inline static EFI_STATUS connect_all_controllers()
{
	// TODO: there is probably a better way to do this
	EFI_STATUS status;

	EFI_HANDLE* handle_buffer = NULL;
	UINTN handle_count        = 0;

	// Get all handles in the system
	status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, AllHandles, NULL, NULL, &handle_count,
	                           &handle_buffer);
	if (EFI_ERROR(status)) return status;

	// Try to connect each controller
	for (UINTN i = 0; i < handle_count; i++)
		uefi_call_wrapper(BS->ConnectController, 4, handle_buffer[i], NULL, NULL, TRUE);

	if (handle_buffer) mem_free_pool(handle_buffer);
	return EFI_SUCCESS;
}

error_t fs_init(EFI_HANDLE bootpart)
{
	EFI_STATUS status = EFI_SUCCESS;
	error_t error     = ERR_OK;

	// Start all controllers
	status = connect_all_controllers();
	if (EFI_ERROR(status)) return ERR_UNKNOWN;

	// Initialize partition table
	error = part_init(bootpart);
	if (error) return error;

	// Set boot partition as default
	error = fs_setpart(PART_TYPE_BOOT, NULL);
	if (error) return error;

	return ERR_OK;
}

error_t fs_setpart(part_type_t type, CHAR16* mod)
{
	switch (type)
	{
		// Use boot partition
		case PART_TYPE_BOOT:
		{
			s_current_part = part_get_boot();
			break;
		}

		// Use partition from GUID
		case PART_TYPE_GUID:
		{
			toupper(mod);
			s_current_part = part_get_from_guid(mod);
			break;
		}

		default: return ERR_FS_PARTLD_FAIL;
	}

	return ERR_OK;
}

partition_t* fs_getpart(void)
{
	return s_current_part;
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

error_t fs_file_open(CHAR16* path, file_t** file)
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

	// Open the partition
	newfile->part = LibOpenRoot(s_current_part->handle);
	if (!newfile->part)
	{
		error = ERR_FS_PARTLD_FAIL;
		goto end;
	}

	// Open the file
	status = uefi_call_wrapper(newfile->part->Open, 5, newfile->part, &newfile->handle, path,
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
	if (file->part)
	{
		status = uefi_call_wrapper(file->part->Close, 1, file->part);
		if (EFI_ERROR(status)) error = ERR_FS_FILE_CLOSE;
	}

	// Free
	mem_free_pool(file);
	file = NULL;

	return error;
}
