/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/


#pragma once

#include <type_traits>

/*!
	\brief Simple class for integer range generator. A single iteration object is stateless, so
	every use in a range-for loop always iterates through all values.
*/
template <class T>
struct CIntRange {
private:
	struct it;
public:
	constexpr CIntRange() = default;
	/*constexpr*/ CIntRange(T b, T e) : _b(b), _e(++e) { }
	constexpr T operator[](size_t x) const {
		return _b + x;
	}
	template <class _T>
	friend constexpr bool operator==(const CIntRange<_T> &lhs, const CIntRange<_T> &rhs) {
		return lhs._b == rhs._b && lhs._e == rhs._e;
	}
	template <class _T>
	friend constexpr bool operator!=(const CIntRange<_T> &lhs, const CIntRange<_T> &rhs) {
		return lhs._b != rhs._b || lhs._e != rhs._e;
	}
	template <class _T> // subset of
	friend constexpr bool operator<=(const CIntRange<_T> &lhs, const CIntRange<_T> &rhs) {
		return lhs._b >= rhs._b && lhs._e <= rhs._e;
	}
	template <class _T> // superset of
	friend constexpr bool operator>=(const CIntRange<_T> &lhs, const CIntRange<_T> &rhs) {
		return lhs._b <= rhs._b && lhs._e >= rhs._e;
	}
	template <class _T> // strict subset of
	friend constexpr bool operator<(const CIntRange<_T> &lhs, const CIntRange<_T> &rhs) {
		return lhs <= rhs && lhs != rhs;
	}
	template <class _T> // strict superset of
	friend constexpr bool operator>(const CIntRange<_T> &lhs, const CIntRange<_T> &rhs) {
		return lhs >= rhs && lhs != rhs;
	}
	template <class _T> // intersection
	friend /*constexpr*/ CIntRange<_T> operator&(const CIntRange<_T> &lhs, const CIntRange<_T> &rhs) {
		return CIntRange<_T>(lhs._b > rhs._b ? lhs._b : rhs._b,
							 lhs._e < rhs._e ? lhs._e : rhs._e);
	}
	/*constexpr*/ CIntRange &operator+=(T rhs) {
		_b += rhs;
		_e += rhs;
		return *this;
	}
	/*constexpr*/ CIntRange &operator-=(T rhs) {
		_b -= rhs;
		_e -= rhs;
		return *this;
	}
	template <class _T> // shift
	friend /*constexpr*/ CIntRange<_T> operator+(const CIntRange<_T> &lhs, T rhs) {
		CIntRange<_T> r {lhs};
		r += rhs;
		return r;
	}
	template <class _T> // shift
	friend /*constexpr*/ CIntRange<_T> operator-(const CIntRange<_T> &lhs, T rhs) {
		CIntRange<_T> r {lhs};
		r -= rhs;
		return r;
	}
	template <class _T> // shift
	friend /*constexpr*/ CIntRange<_T> operator+(T lhs, const CIntRange<_T> &rhs) {
		return rhs + lhs;
	}
	template <class _T> // shift
	friend /*constexpr*/ CIntRange<_T> operator-(T lhs, const CIntRange<_T> &rhs) {
		return rhs - lhs;
	}
	/*constexpr*/ CIntRange &operator++() {
		++_b;
		++_e;
		return *this;
	}
	/*constexpr*/ CIntRange &operator--() {
		--_b;
		--_e;
		return *this;
	}
	CIntRange operator++(int) {
		return CIntRange<T>(_b++, _e++);
	}
	CIntRange operator--(int) {
		return CIntRange<T>(_b--, _e--);
	}
	constexpr it begin() const { return it(_b); }
	constexpr it end() const { return it(_e); }
	T _b, _e;
private:
	struct it {
		explicit constexpr it(T x) : _x(x) { }
		constexpr T operator *() const { return _x; }
		it &operator++() { ++_x; return *this; }
		constexpr bool operator!=(const it &other) const { return _x != other._x; }
	private:
		T _x;
	};
};

template <class T, class U>
constexpr CIntRange<std::common_type_t<T, U>>
make_int_range(T b, U e) {
	return CIntRange<std::common_type_t<T, U>>(b, e);
}
