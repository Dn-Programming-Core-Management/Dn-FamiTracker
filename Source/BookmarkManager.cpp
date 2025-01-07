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

#include "stdafx.h"
#include <vector>
#include <memory>
#include "Bookmark.h"
#include "BookmarkCollection.h"
#include "BookmarkManager.h"

CBookmarkManager::CBookmarkManager(unsigned Count) :
	m_pCollection(Count)
{
	ClearAll();
}

void CBookmarkManager::ClearAll()
{
	for (size_t i = 0; i < m_pCollection.size(); ++i)
		m_pCollection[i].reset(new CBookmarkCollection());
}

CBookmarkCollection *CBookmarkManager::GetCollection(unsigned Track) const
{
	return Track >= m_pCollection.size() ? nullptr : m_pCollection[Track].get();
}

CBookmarkCollection *CBookmarkManager::PopCollection(unsigned Track)
{
	if (Track >= m_pCollection.size()) return nullptr;
	CBookmarkCollection *pCol = m_pCollection[Track].release();
	m_pCollection[Track].reset(new CBookmarkCollection());
	return pCol;
}

void CBookmarkManager::SetCollection(unsigned Track, CBookmarkCollection *const pCol)
{
	m_pCollection[Track].reset(pCol);
}

unsigned int CBookmarkManager::GetBookmarkCount() const
{
	int Total = 0;
	for (size_t i = 0; i < m_pCollection.size(); ++i)
		Total += m_pCollection[i]->GetCount();
	return Total;
}

void CBookmarkManager::InsertTrack(unsigned Track)
{
	m_pCollection.resize(m_pCollection.size() - 1);
	m_pCollection.insert(m_pCollection.begin() + Track, std::unique_ptr<CBookmarkCollection>(new CBookmarkCollection()));
}

void CBookmarkManager::RemoveTrack(unsigned Track)
{
	m_pCollection.erase(m_pCollection.begin() + Track);
	m_pCollection.insert(m_pCollection.end(), std::unique_ptr<CBookmarkCollection>(new CBookmarkCollection()));
}

void CBookmarkManager::SwapTracks(unsigned A, unsigned B)
{
	m_pCollection[A].swap(m_pCollection[B]);
}
