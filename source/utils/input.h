#ifndef __UTILS_INPUT_H__
#define __UTILS_INPUT_H__

#include <efi.h>

typedef UINT64 key_t;

#define KEY(SCAN, CHAR)      (((UINT64)(SCAN) << 32) | ((UINT64)(CHAR)))
#define KEY_SCANCODE(PACKED) ((UINT16)((PACKED) >> 32))
#define KEY_UNICODE(PACKED)  ((CHAR16)((PACKED) & 0xFFFF))

key_t input_wait_for_keypress(UINT64 timeout);
key_t input_get_keypress(void);
BOOLEAN input_is_key_pressed(key_t key);

#endif