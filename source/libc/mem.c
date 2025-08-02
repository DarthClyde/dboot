#include "libc/mem.h"

#include <efilib.h>

VOID* mem_alloc_pool(UINTN size)
{
	VOID* mem;
	EFI_STATUS status = uefi_call_wrapper(BS->AllocatePool, 3, PoolAllocationType, size, &mem);
	if (EFI_ERROR(status)) mem = NULL;

	return mem;
}

VOID* mem_alloc_zpool(UINTN size)
{
	VOID* mem = mem_alloc_pool(size);
	if (mem) ZeroMem(mem, size);
	return mem;
}

EFI_STATUS mem_alloc_pages(EFI_ALLOCATE_TYPE type, EFI_MEMORY_TYPE memtype, UINTN pages,
                           EFI_PHYSICAL_ADDRESS* addr)
{
	return uefi_call_wrapper(BS->AllocatePages, 4, type, memtype, pages, addr);
}

VOID mem_free_pool(VOID* mem)
{
	if (mem) uefi_call_wrapper(BS->FreePool, 1, mem);
}

VOID mem_free_pages(UINTN pages, EFI_PHYSICAL_ADDRESS addr)
{
	uefi_call_wrapper(BS->FreePages, 1, addr, pages);
}
