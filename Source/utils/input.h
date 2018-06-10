#pragma once

#include "type_safe/strong_typedef.hpp"
#include <variant>
#include <utility>
#include <cstdint>

// Keycode vs character

namespace ts = type_safe;

using Keycode = int;

struct Character : ts::strong_typedef<Character, int>,
	ts::strong_typedef_op::equality_comparison<Character>,
	ts::strong_typedef_op::relational_comparison<Character>
{
	using strong_typedef::strong_typedef;
};


using Input = std::variant<Keycode, Character>;
