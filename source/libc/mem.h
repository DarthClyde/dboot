#ifndef __LIBC_MEM_H__
#define __LIBC_MEM_H__

#include <efi.h>

VOID memzero(VOID* buf, UINTN size);
VOID memset(VOID* buf, UINTN size, INT8 val);
VOID memcpy(VOID* dst, VOID* src, UINTN len);
INTN memcmp(VOID* s1, VOID* s2, UINTN len);

VOID* mem_alloc_pool(UINTN size);
VOID* mem_alloc_zpool(UINTN size);
EFI_STATUS mem_alloc_pages(EFI_ALLOCATE_TYPE type, EFI_MEMORY_TYPE memtype, UINTN pages,
                           EFI_PHYSICAL_ADDRESS* addr);

VOID mem_free_pool(VOID* mem);
VOID mem_free_pages(UINTN pages, EFI_PHYSICAL_ADDRESS addr);

#endif
