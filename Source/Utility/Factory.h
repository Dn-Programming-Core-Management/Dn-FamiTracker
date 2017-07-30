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

template <std::size_t I, typename T>
struct biassoc_pair { };

template <std::size_t Key, typename Pair>
struct biassoc_found_l : public std::false_type { };
template <std::size_t Key, typename T>
struct biassoc_found_l<Key, biassoc_pair<Key, T>> : public std::true_type {
	using result_t = T;
};

template <typename Key, typename Pair>
struct biassoc_found_r : public std::false_type { };
template <typename Key, std::size_t I>
struct biassoc_found_r<Key, biassoc_pair<I, Key>> : public std::true_type {
	using result_t = std::integral_constant<std::size_t, I>;
};

template <typename...> // snifae version of std::disjunction
struct or_t { };
template <typename B1, typename... Bn>
struct or_t<B1, Bn...> : public std::conditional_t<bool(B1::value), B1, or_t<Bn...>> { };

template <std::size_t I, typename... Pairs>
using biassoc_find_index = typename or_t<biassoc_found_l<I, Pairs>...>::result_t;
template <typename T, typename... Pairs>
using biassoc_find_type = typename or_t<biassoc_found_r<T, Pairs>...>::result_t;

template <typename... Pairs>
struct indexer_impl {
	static constexpr std::size_t Size = sizeof...(Pairs);
	template <std::size_t N>
	using TypeAt = biassoc_find_index<N, Pairs...>;
	template <std::size_t N, typename Def>
	using TypeOr = biassoc_find_index<N, Pairs..., biassoc_pair<N, Def>>;
	template <typename T>
	using IndexOf = biassoc_find_type<T, Pairs...>;
	template <typename T, std::size_t Def>
	using IndexOr = biassoc_find_type<T, Pairs..., biassoc_pair<Def, T>>;
};

template <>
struct indexer_impl<> {
	static constexpr std::size_t Size = 0;
	template <std::size_t N, typename Def>
	using TypeOr = Def;
	template <typename T, std::size_t Def>
	using IndexOr = std::integral_constant<std::size_t, Def>;
};

template <typename Ind, typename... Ts>
struct make_indexer;
template <std::size_t... Is, typename... Ts>
struct make_indexer<std::index_sequence<Is...>, Ts...> :
	public indexer_impl<biassoc_pair<Is + 1, Ts>...>
{
};

template <typename Ret, typename... Args>
std::unique_ptr<Ret> make_impl(indexer_impl<>, std::size_t id, Args&&... args) {
	return nullptr;
}
template <typename Ret, typename... Args, std::size_t I, typename T, typename... Ts>
std::unique_ptr<Ret> make_impl(indexer_impl<biassoc_pair<I, T>, Ts...>, std::size_t id, Args&&... args) {
	return id == I ?
		std::unique_ptr<Ret>(std::make_unique<T>(std::forward<Args>(args)...)) :
		make_impl<Ret>(indexer_impl<Ts...> { }, id, std::forward<Args>(args)...);
}

} // namespace details

template <typename... Ts>
struct Indexer : public details::make_indexer<std::index_sequence_for<Ts...>, Ts...> {
	static constexpr std::size_t None = 0;
};

template <typename Ret, typename Ind>
struct Factory {
	using index_t = Ind;

private:
	template <typename Ptr>
	struct HandleBase {
		explicit operator bool() const noexcept {
			return static_cast<bool>(data_);
//			return static_cast<bool>(data_ && id_ != index_t::None);
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
		auto &&obj = details::make_impl<Ret>(index_t { }, id, std::forward<Args>(args)...);
		return Handle {std::move(obj), obj ? id : index_t::None};
	}
	template <typename... Args>
	static SharedHandle MakeShared(std::size_t id, Args&&... args) {
		auto &&obj = details::make_impl<Ret>(index_t { }, id, std::forward<Args>(args)...);
		return SharedHandle {std::move(obj), obj ? id : index_t::None};
	}
};

} // namespace FTExt::Utility
