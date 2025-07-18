#include <efi.h>
#include <efilib.h>

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
	InitializeLib(ImageHandle, SystemTable);

	uefi_call_wrapper(ST->ConOut->Reset, 2, ST->ConOut, FALSE);
	uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);

	Print(L"Hello from UEFI\n\n");

	// Print system information
	Print(L"UEFI Firmware Vendor: %s\n", ST->FirmwareVendor);
	Print(L"UEFI Firmware Revision: %d.%d\n",
		ST->FirmwareRevision >> 16,
		ST->FirmwareRevision & 0xFFFF
	);
	Print(L"UEFI Specification Revision: %d.%d\n",
		ST->Hdr.Revision >> 16,
		ST->Hdr.Revision & 0xFFFF
	);

	return EFI_SUCCESS;
}
