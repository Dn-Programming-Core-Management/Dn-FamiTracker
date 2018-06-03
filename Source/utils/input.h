#pragma once

#include "type_safe/strong_typedef.hpp"
#include <variant>
#include <utility>
#include <cstdint>

// Keycode vs character

using Keycode = int64_t;// int

namespace input {
	using namespace type_safe;
	struct Character : strong_typedef<Character, int>,
		strong_typedef_op::equality_comparison<Character>,
		strong_typedef_op::relational_comparison<Character>
	{
		using strong_typedef::strong_typedef;
	};
}
using input::Character;


// Input class
using Input = std::variant<Keycode, Character>;

template<typename T, typename Variant>
bool is(Variant v) {
	return std::holds_alternative<T>(
		std::forward<Variant>(v)
	);
}

using std::get;
using std::get_if;



//class Input {
//private:
//	union {
//		Keycode keycode;
//		Character character;
//	};

//	enum InputType {
//		KEYCODE,
//		CHARACTER
//	};
//	InputType type;
//	
//public:
//	Input(Keycode k) {
//		keycode = k;
//		type = KEYCODE;
//	}
//	Input(Character c) {
//		character = c;
//		type = CHARACTER;
//	}

//	bool isChar() {
//		return type == CHARACTER;
//	}
//	Character getChar() {
//		return character;
//	}

//	bool isKey() {
//		return type == KEYCODE;
//	}
//	Keycode getKey() {
//		return keycode;
//	}
//};

