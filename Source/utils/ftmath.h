#pragma once
#include "stdafx.h"
#include <optional>

// TODO detect errno overflow? honestly it doesn't matter
std::optional<long> parseLong(CString str);
