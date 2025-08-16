#include "utils/input.h"

#include <efilib.h>

key_t input_wait_for_keypress(UINT64 timeout)
{
	EFI_INPUT_KEY key = {0};

	WaitForSingleEvent(ST->ConIn->WaitForKey, timeout);
	uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 1, ST->ConIn, &key);

	return KEY(key.ScanCode, key.UnicodeChar);
}

key_t input_get_keypress(VOID)
{
	EFI_INPUT_KEY key = {};
	uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 1, ST->ConIn, &key);
	return KEY(key.ScanCode, key.UnicodeChar);
}

BOOLEAN input_is_key_pressed(key_t key)
{
	key_t pressed = input_get_keypress();

	if (pressed == key) return TRUE;
	else return FALSE;
}
