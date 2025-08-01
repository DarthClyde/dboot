#ifndef __LIBC_STR_H__
#define __LIBC_STR_H__

#include <efi.h>

INT8 str_to_i8(CHAR16* str);
INT16 str_to_i16(CHAR16* str);
INT64 str_to_i64(CHAR16* str);

UINT8 str_to_u8(CHAR16* str);
UINT16 str_to_u16(CHAR16* str);
UINT64 str_to_u64(CHAR16* str);

#endif
