#include "utils/error.h"

#define ERR(CODE, MSG) MSG,
static const CHAR16* const error_messages[] = {
#include "utils/error_codes.h"
};
#undef ERR

const CHAR16* error_tostring(error_t code)
{
	if (code >= 0 && code < __ERR_COUNT) return error_messages[code];
	else return L"Unknown error code";
}
