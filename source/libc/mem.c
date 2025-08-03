#include "libc/mem.h"

VOID memzero(VOID* buf, UINTN size)
{
	memset(buf, size, (INTN)0);
}

VOID memset(VOID* buf, UINTN size, INT8 val)
{
	if (!buf) return;

	while (size)
	{
		*((INT8*)buf++) = val;
		size--;
	}
}

VOID memcpy(VOID* dst, VOID* src, UINTN len)
{
	if (!dst || !src || len == 0) return;

	while (len)
	{
		*((INT8*)dst++) = *((INT8*)src++);
		len--;
	}
}

INTN memcmp(VOID* s1, VOID* s2, UINTN len)
{
	while (len)
	{
		INTN diff = *((UINT8*)s1) - *((UINT8*)s2);
		if (diff != 0) return diff;

		s1++;
		s2++;
		len--;
	}
	return 0;
}
