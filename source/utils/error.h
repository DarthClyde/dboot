#ifndef __UTILS_ERROR_H__
#define __UTILS_ERROR_H__

#include <efi.h>

// Error codes
typedef enum error
{
#define ERR(CODE, MSG) CODE,
#include "utils/error_codes.h"
#undef ERR
	__ERR_COUNT
} error_t;

#define ERR_PRINT(ERR)     Print(__FILE__ L":%d\nError:%d: %s\n", __LINE__, ERR, error_tostring(ERR))
#define ERR_PRINT_STR(STR) Print(__FILE__ L":%d\nError: %s\n", __LINE__, STR)

// Error check macros
#define __ERR_CHECK_(ERR) \
	if ((UINT64)ERR > 0)  \
	{                     \
		ERR_PRINT(ERR);   \
	}
#define __ERR_CHECK_RET(ERR) \
	if ((UINT64)ERR > 0)     \
	{                        \
		ERR_PRINT(ERR);      \
		return ERR;          \
	}                        \
	else return ERR_OK;
#define __ERR_CHECK_END(ERR) \
	if ((UINT64)ERR > 0)     \
	{                        \
		ERR_PRINT(ERR);      \
		goto end;            \
	}

#define ERR_CHECK(ERRC, DO) __ERR_CHECK_##DO(ERRC)

const CHAR16* error_tostring(error_t code);

#endif
