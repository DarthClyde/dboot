#include <fs/part.h>

#include <efilib.h>

static EFI_HANDLE s_boot_part_rawhndl = NULL;
static partition_t* s_boot_part       = NULL;

static partition_t* s_part_tbl        = NULL;
static UINTN s_part_tbl_size          = 0;

inline static VOID resize_part_tbl(UINTN size)
{
	// Allocate new table
	partition_t* part_tbl = mem_alloc_zpool(size * sizeof(partition_t));
	if (!part_tbl) return;

	// Copy old data
	for (UINTN i = 0; i < s_part_tbl_size; i++)
	{
		part_tbl[i] = s_part_tbl[i];
	}

	// Free old memory
	mem_free_pool(s_part_tbl);

	s_part_tbl      = part_tbl;
	s_part_tbl_size = size;
}

error_t part_init(EFI_HANDLE bootpart)
{
	EFI_STATUS status            = EFI_SUCCESS;
	error_t error                = ERR_OK;

	EFI_HANDLE* handle_buffer    = NULL;
	UINTN handle_count           = 0;

	EFI_BLOCK_IO* block_io       = NULL;
	EFI_DEVICE_PATH* device_path = NULL;

	// Get information on the bootpart image
	EFI_LOADED_IMAGE* bootimg = NULL;
	uefi_call_wrapper(BS->OpenProtocol, 6, bootpart, &LoadedImageProtocol, (VOID**)&bootimg,
	                  bootpart, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	// Get all devices with the block IO protocol
	status = LibLocateHandle(ByProtocol, &BlockIoProtocol, NULL, &handle_count, &handle_buffer);
	if (EFI_ERROR(status))
	{
		error = ERR_HANDLLOC_FAIL;
		goto end;
	}

	// Iterate through all devices
	for (UINTN i = 0; i < handle_count; i++)
	{
		EFI_HANDLE handle = handle_buffer[i];

		// Get block IO protocol
		status =
		    uefi_call_wrapper(BS->HandleProtocol, 3, handle, &BlockIoProtocol, (VOID**)&block_io);
		if (EFI_ERROR(status) || !block_io) continue;
		if (!block_io->Media->LogicalPartition) continue;

		// Get device path protocol
		device_path = DevicePathFromHandle(handle);
		if (!device_path) continue;

		// Traverse device path nodes:
		//   We are assuming there will only be one valid node per handle.
		//   If there is more than one, only the first will be recognized.
		EFI_DEVICE_PATH_PROTOCOL* node = NULL;
		for (node = device_path; !IsDevicePathEnd(node); node = NextDevicePathNode(node))
		{
			if (DevicePathType(node) != MEDIA_DEVICE_PATH
			    || DevicePathSubType(node) != MEDIA_HARDDRIVE_DP)
				continue;

			HARDDRIVE_DEVICE_PATH* hd = (HARDDRIVE_DEVICE_PATH*)node;

			// Resize partition table
			resize_part_tbl(s_part_tbl_size + 1);
			partition_t* part = &s_part_tbl[s_part_tbl_size - 1];

			// Set partition info
			part->handle  = handle;
			part->partnum = hd->PartitionNumber;
			part->sigtype = hd->SignatureType;
			GuidToString(part->guid, (EFI_GUID*)&hd->Signature);

			break;
		}
	}

end:
	if (!s_boot_part) Print(L"Warning: failed to build partition table\n");

	// Find boot partition
	s_boot_part = part_get_from_hndl(bootimg->DeviceHandle);
	if (!s_boot_part)
	{
		Print(L"Warning: boot partition was not found in device scan\n");
		s_boot_part         = mem_alloc_zpool(sizeof(partition_t));
		s_boot_part->handle = bootimg->DeviceHandle;
	}
	s_boot_part_rawhndl = bootpart;

	mem_free_pool(handle_buffer);
	if (bootimg)
		uefi_call_wrapper(BS->CloseProtocol, 4, bootpart, &LoadedImageProtocol, bootpart, NULL);

	return error;
}

#ifdef DB_DEBUG
VOID part_table_debuglog(void)
{
	if (!s_part_tbl) return;

	Print(L"---- BEGIN PARTITION TABLE ----\n");

	for (UINTN i = 0; i < s_part_tbl_size; i++)
	{
		Print(L"[Part #%d (%d)] ", i, s_part_tbl[i].partnum);

		if (s_part_tbl[i].handle == s_boot_part->handle) Print(L"[BOOT PART] ");
		if (s_part_tbl[i].sigtype == SIGNATURE_TYPE_GUID) Print(L"[GPT]\n");
		else if (s_part_tbl[i].sigtype == SIGNATURE_TYPE_MBR) Print(L"[MBR]\n");
		else Print(L"[UNKNOWN]");

		Print(L"  HNDL: %p", (UINTN)s_part_tbl[i].handle);
		Print(L"  GUID: %s\n", s_part_tbl[i].guid);
	}

	Print(L"---- END PARTITION TABLE ----\n\n");
}
#endif

EFI_HANDLE part_get_boot_rawhndl(void)
{
	return s_boot_part_rawhndl;
}

partition_t* part_get_boot(void)
{
	return s_boot_part;
}

partition_t* part_get_from_guid(CHAR16* guid)
{
	for (UINTN i = 0; i < s_part_tbl_size; i++)
	{
		if (strcmp(s_part_tbl[i].guid, guid) == 0) return &s_part_tbl[i];
	}
	return NULL;
}

partition_t* part_get_from_hndl(EFI_HANDLE hndl)
{
	for (UINTN i = 0; i < s_part_tbl_size; i++)
	{
		if (s_part_tbl[i].handle == hndl) return &s_part_tbl[i];
	}
	return NULL;
}
