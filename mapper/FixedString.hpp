// #pragma once


// template<size_t max_length>
// struct FixedString {
//   FixedString(const char* string, size_t length);
//   operator std::string_view() const;

//   // std::string impl; ???
// };

// // operator ""_cstr ?

#pragma once

#include <string_view>

template <size_t max_length>
struct FixedString {	
	constexpr FixedString(const char* string, size_t length)
		: len(std::min(max_length, length))
	{
		for (size_t i = 0; i < len; ++i)
			str[i] = string[i];
		for (size_t i = len; i < max_length; ++i)
			str[i] = 0;
	}

	constexpr operator std::string_view() const {
		return std::string_view(str, len);
	}
	
	char str[max_length];
	size_t len;
};

constexpr FixedString<256> operator""_cstr(const char* str, size_t len) {
	return FixedString<256>(str, len);
}