#include "libc/str.h"

#include "mem.h"

INTN strcmp(CHAR16* s1, CHAR16* s2)
{
	while (*s1)
	{
		if (*s1 != *s2) break;

		s1++;
		s2++;
	}
	return *s1 - *s2;
}

VOID strcpy(CHAR16* dst, CHAR16* src)
{
	while (*src)
	{
		*(dst++) = *(src++);
	}
	*dst = '\0';
}

VOID strcpys(CHAR16* dst, CHAR16* src, UINTN len)
{
	UINTN size = strlens(src, len);
	if (size != len)
	{
		memset(dst + size, (UINTN)((len - size) * sizeof(CHAR16)), (UINT8)'\0');
	}
	memcpy(dst, src, len * sizeof(CHAR16));
}

UINTN strlen(CHAR16* str)
{
	UINTN len = 0;
	while (*str)
	{
		len++;
		str++;
	}
	return len;
}

UINTN strlens(CHAR16* str, UINTN len)
{
	UINTN i;
	for (i = 0; *str && i < len; i++) str++;
	return i;
}

INTN strcmp_ascii(CHAR8* s1, CHAR8* s2)
{
	while (*s1)
	{
		if (*s1 != *s2) break;

		s1++;
		s2++;
	}
	return *s1 - *s2;
}

UINTN strlen_ascii(CHAR8* str)
{
	UINTN len = 0;
	while (*str)
	{
		len++;
		str++;
	}
	return len;
}
