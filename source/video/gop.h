#ifndef __VIDEO_GOP_H__
#define __VIDEO_GOP_H__

#include "hdr.h"

error_t gop_init(VOID);
BOOLEAN gop_isactive(VOID);

VOID gop_print(CHAR16* fmt, ...);
VOID gop_printp(UINTN x, UINTN y, CHAR16* fmt, ...);
VOID gop_printc(UINTN color, CHAR16* fmt, ...);
VOID gop_printpc(UINTN x, UINTN y, UINTN color, CHAR16* fmt, ...);

#endif
