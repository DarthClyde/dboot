#ifndef __VIDEO_GOP_H__
#define __VIDEO_GOP_H__

#include <efi.h>

EFI_STATUS gop_init(void);
BOOLEAN gop_isactive(void);

VOID gop_print(CHAR16* fmt, ...);
VOID gop_printp(UINTN x, UINTN y, CHAR16* fmt, ...);
VOID gop_printc(UINTN color, CHAR16* fmt, ...);
VOID gop_printpc(UINTN x, UINTN y, UINTN color, CHAR16* fmt, ...);

#endif