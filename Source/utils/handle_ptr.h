#pragma once

#include <memory>
#include <windef.h>

struct CloseHandleT {
	void operator()(HANDLE handle) {
		if (handle) {
			CloseHandle(handle);
		}
	}
};

/// HANDLE is void*, so unique_ptr<void, CloseHandleT> is a RAII HANDLE.
using HandlePtr = std::unique_ptr<void, CloseHandleT>;
