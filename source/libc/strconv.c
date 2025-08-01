#include "libc/str.h"

INT8 str_to_i8(CHAR16* str)
{
	INT8 num  = 0;
	INT8 sign = 1;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else if (*str == '+')
	{
		str++;
	}

	while (*str)
	{
		if (*str >= '0' && *str <= '9')
		{
			num = num * 10 + (INT8)(*str - '0');
		}
		else break;

		str++;
	}
	return num * sign;
}

INT16 str_to_i16(CHAR16* str)
{
	INT16 num = 0;
	INT8 sign = 1;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else if (*str == '+')
	{
		str++;
	}

	while (*str)
	{
		if (*str >= '0' && *str <= '9')
		{
			num = num * 10 + (INT16)(*str - '0');
		}
		else break;

		str++;
	}
	return num * sign;
}

INT64 str_to_i64(CHAR16* str)
{
	INT64 num = 0;
	INT8 sign = 1;

	if (*str == '-')
	{
		sign = -1;
		str++;
	}
	else if (*str == '+')
	{
		str++;
	}

	while (*str)
	{
		if (*str >= '0' && *str <= '9')
		{
			num = num * 10 + (INT64)(*str - '0');
		}
		else break;

		str++;
	}
	return num * sign;
}

UINT8 str_to_u8(CHAR16* str)
{
	UINT8 num = 0;
	while (*str)
	{
		if (*str >= '0' && *str <= '9')
		{
			num = num * 10 + (UINT8)(*str - '0');
		}
		else break;

		str++;
	}
	return num;
}

UINT16 str_to_u16(CHAR16* str)
{
	UINT16 num = 0;
	while (*str)
	{
		if (*str >= '0' && *str <= '9')
		{
			num = num * 10 + (UINT16)(*str - '0');
		}
		else break;

		str++;
	}
	return num;
}

UINT64 str_to_u64(CHAR16* str)
{
	UINT64 num = 0;
	while (*str)
	{
		if (*str >= '0' && *str <= '9')
		{
			num = num * 10 + (UINT64)(*str - '0');
		}
		else break;

		str++;
	}
	return num;
}
