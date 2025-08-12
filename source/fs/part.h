#ifndef __FS_PART_H__
#define __FS_PART_H__

#include "hdr.h"

#define GUID_LEN 37

typedef struct partition
{
	EFI_HANDLE handle;
	UINT8 sigtype;

	UINT32 partnum;
	CHAR16 guid[GUID_LEN];
} partition_t;

error_t part_init(EFI_HANDLE bootpart);
#ifdef DB_DEBUG
VOID part_table_debuglog(void);
#endif

partition_t* part_get_boot(void);
EFI_HANDLE part_get_boot_rawhndl(void);

partition_t* part_get_from_guid(CHAR16* guid);
partition_t* part_get_from_hndl(EFI_HANDLE hndl);

#endif
