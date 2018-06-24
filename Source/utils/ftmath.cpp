#include "ftmath.h"

std::optional<long> parseLong(CString str) {
	char *endptr;
	const int radix = 10;

	long out;
	out = _tcstol(str, &endptr, radix);
	if (*endptr == '\0') {
		return out;
	}
	return {};
}

