#include "video/gop.h"

#include <efilib.h>

static EFI_GRAPHICS_OUTPUT_PROTOCOL* s_gop              = NULL;

static EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* s_gop_info = NULL;
static UINTN s_gop_info_size                            = 0;

error_t gop_init(void)
{
	EFI_STATUS status = EFI_SUCCESS;
	EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

	// Get the GOP protocol
	status = uefi_call_wrapper(BS->LocateProtocol, 3, &gop_guid, NULL, (void**)&s_gop);
	if (EFI_ERROR(status)) return ERR_VID_GOP_LOCATE;

	// Query the GOP mode info
	UINT32 modeNum = 0;
	if (s_gop->Mode != NULL) modeNum = s_gop->Mode->Mode;

	status = uefi_call_wrapper(s_gop->QueryMode, 4, s_gop, modeNum, &s_gop_info_size, &s_gop_info);
	if (status == EFI_NOT_STARTED) status = uefi_call_wrapper(s_gop->SetMode, 2, s_gop, 0);
	if (EFI_ERROR(status)) return ERR_VID_GOP_QUERY;

	return ERR_OK;
}

BOOLEAN gop_isactive(void)
{
	return s_gop != NULL;
}

#define printfmt()       \
	va_list args;        \
	va_start(args, fmt); \
	VPrint(fmt, args);   \
	va_end(args);

VOID gop_print(CHAR16* fmt, ...)
{
	printfmt();
}

VOID gop_printp(UINTN x, UINTN y, CHAR16* fmt, ...)
{
	uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, x, y);
	printfmt();
}

VOID gop_printc(UINTN color, CHAR16* fmt, ...)
{
	uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut, color);
	printfmt();
}

VOID gop_printpc(UINTN x, UINTN y, UINTN color, CHAR16* fmt, ...)
{
	uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, x, y);
	uefi_call_wrapper(ST->ConOut->SetAttribute, 2, ST->ConOut, color);
	printfmt();
}

#undef printfmt
