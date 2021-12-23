/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/


#pragma once

#include <string_view>
#include <string>
#include "array_view.h"
#ifdef _WINDOWS
#include "stdafx.h" // windows-specific
#endif

namespace conv {

template <typename CharT>
constexpr std::basic_string_view<CharT> to_sv(const CharT *str) {
	return std::basic_string_view<CharT>(str);
}

template <typename CharT, typename TraitsT, typename AllocT>
constexpr std::basic_string_view<CharT> to_sv(const std::basic_string<CharT, TraitsT, AllocT> &str) noexcept {
	return std::basic_string_view<CharT>(str);
}
template <typename CharT, typename TraitsT, typename AllocT>
std::basic_string_view<CharT> to_sv(std::basic_string<CharT, TraitsT, AllocT> &&) = delete;

template <typename CharT>
constexpr std::basic_string_view<CharT> to_sv(std::basic_string_view<CharT> str) noexcept {
	return str;
}

template <typename CharT>
constexpr std::basic_string_view<CharT> to_sv(array_view<CharT> str) noexcept {
	return std::basic_string_view<CharT>(str.data(), str.size());
}

#ifdef _WINDOWS
template <typename CharT, typename TraitsT>
std::basic_string_view<CharT> to_sv(const ATL::CStringT<CharT, TraitsT> &str) {
	return std::basic_string_view<CharT>(str, str.GetLength());
}
template <typename CharT, typename TraitsT>
std::basic_string_view<CharT> to_sv(ATL::CStringT<CharT, TraitsT> &&str) = delete;
#endif

} // namespace conv
