#ifndef __LIBC_STR_H__
#define __LIBC_STR_H__

#include <efi.h>

INTN strcmp(CHAR16* s1, CHAR16* s2);
VOID strcpy(CHAR16* dst, CHAR16* src);
VOID strcpys(CHAR16* dst, CHAR16* src, UINTN len);
UINTN strlen(CHAR16* str);
UINTN strlens(CHAR16* str, UINTN len);

CHAR16* strchr(CHAR16* str, CHAR16 ch);

INTN strcmp_ascii(CHAR8* s1, CHAR8* s2);
UINTN strlen_ascii(CHAR8* str);

INT8 str_to_i8(CHAR16* str);
INT16 str_to_i16(CHAR16* str);
INT64 str_to_i64(CHAR16* str);

UINT8 str_to_u8(CHAR16* str);
UINT16 str_to_u16(CHAR16* str);
UINT64 str_to_u64(CHAR16* str);

#endif
