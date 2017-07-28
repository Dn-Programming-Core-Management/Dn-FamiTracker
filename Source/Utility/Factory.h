/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include <memory>
#include <utility>
#include <type_traits>

namespace FTExt::Utility {

namespace details {

template <std::size_t N, typename... Ts>
struct type_at {
};
template <std::size_t N, typename T, typename... Ts>
struct type_at<N, T, Ts...> {
	using type = typename type_at<N - 1, Ts...>::type;
};
template <typename T, typename... Ts>
struct type_at<1, T, Ts...> {
	using type = T;
};

template <std::size_t N, typename T, typename... Us>
struct type_or {
	using type = T;
};
template <std::size_t N, typename T, typename U, typename... Us>
struct type_or<N, T, U, Us...> {
	using type = typename type_or<N - 1, T, Us...>::type;
};
template <typename T, typename U, typename... Us>
struct type_or<1, T, U, Us...> {
	using type = U;
};

template <std::size_t N, typename T, typename... Us>
struct index_of : public std::integral_constant<std::size_t, 0> {
};
template <std::size_t N, typename T, typename U, typename... Us>
struct index_of<N, T, U, Us...> : public index_of<N + 1, T, Us...> {
};
template <std::size_t N, typename T, typename... Us>
struct index_of<N, T, T, Us...> : public std::integral_constant<std::size_t, N> {
};

} // namespace details

template <typename... Ts>
struct Indexer {
	static constexpr std::size_t Size = sizeof...(Ts);
	static constexpr std::size_t None = 0;
	template <std::size_t N>
	using TypeAt = typename details::type_at<N, Ts...>::type;
	template <std::size_t N, typename Or>
	using TypeOr = typename details::type_or<N, Or, Ts...>::type;
	template <typename U>
	using IndexOf = details::index_of<1, U, Ts...>;
};

namespace details {

template <typename Ret, typename... Args>
std::unique_ptr<Ret> make_impl(std::size_t id, Args&&... args) {
	return nullptr;
}
template <typename Ret, typename T, typename... Ts, typename... Args>
std::unique_ptr<Ret> make_impl(std::size_t id, Args&&... args) {
	return id == 1 ? std::unique_ptr<Ret>(std::make_unique<T>(std::forward<Args>(args)...)) :
		make_impl<Ret, Ts...>(id - 1, std::forward<Args>(args)...);
}

} // namespace details

template <typename Ret, typename Ind>
struct Factory;
template <typename Ret, typename... Ts>
struct Factory<Ret, Indexer<Ts...>> {
//	static_assert(std::conjunction<std::is_convertible<Ts, Ret>...>::value,
//		"Factory product types must be convertible to return type");
	using index_t = Indexer<Ts...>;

private:
	template <typename Ptr>
	struct HandleBase {
		explicit operator bool() const noexcept {
			return static_cast<bool>(data_);
		}
		template <typename T>
		T *GetData() noexcept {
			return id_ == index_t::template IndexOf<T> ? static_cast<T *>(data_.get()) : nullptr;
		}
		template <typename T>
		const T *GetData() const noexcept {
			return const_cast<HandleBase *>(this)->GetData<T>();
		}
		Ret &operator*() const {
			return *data_;
		}
		Ret *operator->() const noexcept {
			return data_.get();
		}
		operator Ret *() noexcept {
			return data_.get();
		}
		operator const Ret *() const noexcept {
			return data_.get();
		}
		std::size_t GetIndex() const noexcept {
			return id_;
		}
		HandleBase(std::unique_ptr<Ret> &&data, std::size_t id) noexcept :
			data_(std::move(data)), id_(id)
		{
		}
		
	protected:
		Ptr data_;
		const std::size_t id_;
	};

public:
	struct Handle : public HandleBase<std::unique_ptr<Ret>> {
		Ret *Release() noexcept {
			return data_.release();
		}
	private:
		using HandleBase<std::unique_ptr<Ret>>::HandleBase;
		friend struct Factory;
	};
	struct SharedHandle : public HandleBase<std::shared_ptr<Ret>> {
		SharedHandle(Handle &&hnd) noexcept :
			HandleBase<std::shared_ptr<Ret>>(std::move(hnd.data_), hnd.id_)
		{
		}
	private:
		using HandleBase<std::shared_ptr<Ret>>::HandleBase;
		friend struct Factory;
	};

public:
	template <typename... Args>
	static Handle Make(std::size_t id, Args&&... args) {
		return Handle {details::make_impl<Ret, Ts...>(id, std::forward<Args>(args)...),
			id > index_t::Size ? 0 : id};
	}
	template <typename... Args>
	static SharedHandle MakeShared(std::size_t id, Args&&... args) {
		return SharedHandle {details::make_impl<Ret, Ts...>(id, std::forward<Args>(args)...),
			id > index_t::Size ? 0 : id};
	}
};

} // namespace FTExt::Utility
