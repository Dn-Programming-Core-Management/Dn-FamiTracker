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

#include <iterator>
#include <limits>
#include <type_traits>

namespace details {

template <typename T>
using data_func_t = decltype(std::declval<T>().data());
template <typename T>
using size_func_t = decltype(std::declval<T>().size());
template <typename T, typename ValT, typename = void>
struct is_view_like : std::false_type { };
template <typename T, typename ValT>
struct is_view_like<T, ValT, std::void_t<data_func_t<T>, size_func_t<T>>> :
	std::bool_constant<std::is_convertible_v<data_func_t<T>, const ValT *> &&
		std::is_convertible_v<size_func_t<T>, std::size_t>> { };
/*
template <typename T, typename ValT>
concept ViewLike = requires (T x) {
	{ x.data() } -> const ValT *;
	{ x.size() } -> std::size_t;
};
*/

} // namespace details

#define REQUIRES_ViewLike(T, ValT) \
	std::enable_if_t<details::is_view_like<T, ValT>::value, int> = 0

template <typename T>
class array_view {
public:
	using value_type = T;
	using pointer = T *;
	using const_pointer = const T *;
	using reference = T &;
	using const_reference = const T &;
	using iterator = const_pointer;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<const_pointer>;
	using const_reverse_iterator = std::reverse_iterator<const_pointer>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	static constexpr auto npos = (size_type)-1;

	constexpr array_view() noexcept = default;
	constexpr array_view(const_pointer data, std::size_t sz) noexcept :
		data_(data), size_(sz) { }
	template <typename ValT, std::size_t N,
		std::enable_if_t<std::is_convertible_v<ValT *, const value_type *>, int> = 0>
	constexpr array_view(const ValT (&data)[N]) noexcept :
		data_(data), size_(N) { }
	template <typename U, REQUIRES_ViewLike(U, value_type)>
	constexpr array_view(const U &arr) noexcept :
		data_(arr.data()), size_(arr.size()) { }

	constexpr array_view(const array_view &) noexcept = default;
	constexpr array_view(array_view &&) noexcept = default;
	constexpr array_view &operator=(const array_view &) noexcept = default;
	constexpr array_view &operator=(array_view &&) noexcept = default;
	~array_view() noexcept = default;

	constexpr void swap(array_view &other) noexcept {
		std::swap(data_, other.data_);
		std::swap(size_, other.size_);
	}

	constexpr const_reference operator[](size_type pos) const {
		return data_[pos];
	}
	constexpr const_reference at(size_type pos) const {
		if (pos >= size())
			throw std::out_of_range {"array_view::at() out of range"};
		return operator[](pos);
	}
	constexpr const_reference front() const {
		return operator[](0);
	}
	constexpr const_reference back() const {
		return operator[](size() - 1);
	}
	constexpr const_pointer data() const noexcept {
		return data_;
	}

	constexpr array_view<unsigned char> as_bytes() const noexcept {
		return {reinterpret_cast<const unsigned char *>(data()),
			sizeof(value_type) * size()};
	}

	constexpr size_type size() const noexcept {
		return size_;
	}
	constexpr size_type length() const noexcept {
		return size_;
	}
	constexpr size_type max_size() const noexcept {
		return std::numeric_limits<size_type>::max() / sizeof(value_type);
	}
	constexpr bool empty() const noexcept {
		return size_ == 0u;
	}

	constexpr void clear() noexcept {
		*this = array_view { };
	}
	constexpr void remove_front(size_type count) noexcept {
		difference_type offs = (count < size()) ? count : size();
		data_ += offs;
		size_ -= offs;
	}
	constexpr void pop_front() noexcept {
		remove_front(1u);
	}
	constexpr void remove_back(size_type count) noexcept {
		size_ -= (count < size()) ? count : size();
	}
	constexpr void pop_back() noexcept {
		remove_back(1u);
	}

	constexpr const_iterator begin() const noexcept {
		return const_iterator {data_};
	}
	constexpr const_iterator cbegin() const noexcept {
		return const_iterator {data_};
	}
	constexpr const_iterator end() const noexcept {
		return const_iterator {data_ + size_};
	}
	constexpr const_iterator cend() const noexcept {
		return const_iterator {data_ + size_};
	}

	const_reverse_iterator rbegin() const noexcept {
		return const_reverse_iterator {end()};
	}
	const_reverse_iterator crbegin() const noexcept {
		return const_reverse_iterator {cend()};
	}
	const_reverse_iterator rend() const noexcept {
		return const_reverse_iterator {begin()};
	}
	const_reverse_iterator crend() const noexcept {
		return const_reverse_iterator {cbegin()};
	}

	size_type copy(pointer dest, size_type count, size_type pos = 0) const {
		if (pos > size())
			throw std::out_of_range {"pos cannot be larger than size()"};
		if (count > size() - pos)
			count = size() - pos;
		for (size_type i = 0; i < count; ++i)
			*dest++ = data_[pos++];
		return count;
	}
	template <typename U, REQUIRES_ViewLike(U, value_type)>
	size_type copy(U &dest, size_type pos = 0) const {
		return copy(dest.data(), dest.size(), pos);
	}

	constexpr array_view subview(size_type pos, size_type count = npos) const {
		if (pos > size())
			throw std::out_of_range {"pos cannot be larger than size()"};
		if (count > size() - pos)
			count = size() - pos;
		return {data_ + pos, count};
	}

	constexpr int compare(const array_view &other) const {
		auto b1 = begin();
		auto b2 = other.begin();
		auto e1 = end();
		auto e2 = other.end();

		while ((b1 != e1) && (b2 != e2)) {
			if (*b1 < *b2)
				return -1;
			if (*b2 < *b1)
				return 1;
			++b1;
			++b2;
		}

		if (b2 != e2)
			return -1;
		if (b1 != e1)
			return 1;
		return 0;
	}

private:
	const_pointer data_ = nullptr;
	size_type size_ = 0u;
};

template <typename T>
constexpr bool operator==(const array_view<T> &lhs, const array_view<T> &rhs) {
	return (lhs.data() == rhs.data() && lhs.size() == rhs.size()) ||
		lhs.compare(rhs) == 0;
}
template <typename T>
constexpr bool operator!=(const array_view<T> &lhs, const array_view<T> &rhs) {
	return (lhs.data() != rhs.data() || lhs.size() != rhs.size()) &&
		lhs.compare(rhs) != 0;
}
template <typename T>
constexpr bool operator<(const array_view<T> &lhs, const array_view<T> &rhs) {
	return lhs.compare(rhs) < 0;
}
template <typename T>
constexpr bool operator>(const array_view<T> &lhs, const array_view<T> &rhs) {
	return lhs.compare(rhs) > 0;
}
template <typename T>
constexpr bool operator<=(const array_view<T> &lhs, const array_view<T> &rhs) {
	return lhs.compare(rhs) <= 0;
}
template <typename T>
constexpr bool operator>=(const array_view<T> &lhs, const array_view<T> &rhs) {
	return lhs.compare(rhs) >= 0;
}

template <typename T, typename ValT, REQUIRES_ViewLike(ValT, T)>
constexpr bool operator==(const array_view<T> &lhs, const ValT &rhs) {
	return lhs == array_view<T> {rhs};
}
template <typename T, typename ValT, REQUIRES_ViewLike(ValT, T)>
constexpr bool operator!=(const array_view<T> &lhs, const ValT &rhs) {
	return lhs != array_view<T> {rhs};
}
template <typename T, typename ValT, REQUIRES_ViewLike(ValT, T)>
constexpr bool operator<(const array_view<T> &lhs, const ValT &rhs) {
	return lhs < array_view<T> {rhs};
}
template <typename T, typename ValT, REQUIRES_ViewLike(ValT, T)>
constexpr bool operator>(const array_view<T> &lhs, const ValT &rhs) {
	return lhs > array_view<T> {rhs};
}
template <typename T, typename ValT, REQUIRES_ViewLike(ValT, T)>
constexpr bool operator<=(const array_view<T> &lhs, const ValT &rhs) {
	return lhs <= array_view<T> {rhs};
}
template <typename T, typename ValT, REQUIRES_ViewLike(ValT, T)>
constexpr bool operator>=(const array_view<T> &lhs, const ValT &rhs) {
	return lhs >= array_view<T> {rhs};
}

template <typename T, std::size_t N>
constexpr bool operator==(const array_view<T> &lhs, const T (&rhs)[N]) {
	return lhs == array_view<T> {rhs};
}
template <typename T, std::size_t N>
constexpr bool operator!=(const array_view<T> &lhs, const T (&rhs)[N]) {
	return lhs != array_view<T> {rhs};
}
template <typename T, std::size_t N>
constexpr bool operator<(const array_view<T> &lhs, const T (&rhs)[N]) {
	return lhs < array_view<T> {rhs};
}
template <typename T, std::size_t N>
constexpr bool operator>(const array_view<T> &lhs, const T (&rhs)[N]) {
	return lhs > array_view<T> {rhs};
}
template <typename T, std::size_t N>
constexpr bool operator<=(const array_view<T> &lhs, const T (&rhs)[N]) {
	return lhs <= array_view<T> {rhs};
}
template <typename T, std::size_t N>
constexpr bool operator>=(const array_view<T> &lhs, const T (&rhs)[N]) {
	return lhs >= array_view<T> {rhs};
}

template <typename T, typename ValT>
constexpr bool operator==(const T &lhs, const array_view<ValT> &rhs) {
	return rhs == lhs;
}
template <typename T, typename ValT>
constexpr bool operator!=(const T &lhs, const array_view<ValT> &rhs) {
	return rhs != lhs;
}
template <typename T, typename ValT>
constexpr bool operator<(const T &lhs, const array_view<ValT> &rhs) {
	return rhs > lhs;
}
template <typename T, typename ValT>
constexpr bool operator>(const T &lhs, const array_view<ValT> &rhs) {
	return rhs < lhs;
}
template <typename T, typename ValT>
constexpr bool operator<=(const T &lhs, const array_view<ValT> &rhs) {
	return rhs >= lhs;
}
template <typename T, typename ValT>
constexpr bool operator>=(const T &lhs, const array_view<ValT> &rhs) {
	return rhs <= lhs;
}



template <>
class array_view<void>;
