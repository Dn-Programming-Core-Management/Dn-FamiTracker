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

#include "stdafx.h" // new
#include <memory>
#include <functional>
#include <unordered_map>

template <typename Key, typename Base>
class CFactory
{
public:
	using FuncType = std::shared_ptr<std::function<Base*()>>; // std::shared_ptr<Base* (*) ()>;
	CFactory();
	virtual ~CFactory();

	template <typename T, typename... Arg> void AddProduct(Key Index, Arg&&... x);
	void RemoveProduct(Key Index);
	Base *Produce(Key Type) const;

protected:
	template <typename T, typename... Arg> static FuncType MakeCtor(Arg&&... x);

protected:
	std::unordered_map<Key, FuncType> m_pMakeFunc;
};

template<typename Key, typename Base>
inline CFactory<Key, Base>::CFactory()
{
}

template<typename Key, typename Base>
inline CFactory<Key, Base>::~CFactory()
{
}

template<typename Key, typename Base>
template<typename T, typename... Arg>
inline void CFactory<Key, Base>::AddProduct(Key Index, Arg&&... x)
{
	if (m_pMakeFunc.find(Index) == m_pMakeFunc.end())
		m_pMakeFunc[Index] = MakeCtor<T>(std::forward<Arg>(x)...);
}

template<typename Key, typename Base>
inline void CFactory<Key, Base>::RemoveProduct(Key Index)
{
	m_pMakeFunc.erase(Index);
}

template<typename Key, typename Base>
inline Base *CFactory<Key, Base>::Produce(Key Index) const
{
	auto it = m_pMakeFunc.find(Index);
	if (it != m_pMakeFunc.end()) return (*it->second.get())();
	return nullptr;
}

template<typename Key, typename Base>
template<typename T, typename... Arg>
inline typename CFactory<Key, Base>::FuncType CFactory<Key, Base>::MakeCtor(Arg&&... x)
{
	return std::make_shared<std::function<Base*()>>(std::bind(
		[] (auto&&... y) -> Base* { return new T {y...}; },
		std::forward<Arg>(x)...
	));
}
