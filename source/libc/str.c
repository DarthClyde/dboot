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
	memcpy(dst, src, len * sizeof(CHAR16));
	dst[len] = '\0';
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

CHAR16* strchr(CHAR16* str, CHAR16 ch)
{
	while (*str)
	{
		if (*str == ch) return str;
		str++;
	}
	return NULL;
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

CHAR16* toupper(CHAR16* str)
{
	while (*str)
	{
		if (*str >= 'a' && *str <= 'z') *str -= 32;
		str++;
	}
	return str;
}
