/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
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
