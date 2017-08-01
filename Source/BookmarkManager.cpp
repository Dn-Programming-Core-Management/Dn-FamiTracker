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

#include "BookmarkManager.h"
#include "Bookmark.h"
#include "BookmarkCollection.h"

CBookmarkManager::CBookmarkManager(unsigned Count) :
	m_pCollection(Count)
{
	ClearAll();
}

void CBookmarkManager::ClearAll()
{
	for (auto &ptr : m_pCollection)
		ptr = std::make_unique<CBookmarkCollection>();
}

CBookmarkCollection *CBookmarkManager::GetCollection(unsigned Track) const
{
	return Track >= m_pCollection.size() ? nullptr : m_pCollection[Track].get();
}

CBookmarkCollection *CBookmarkManager::PopCollection(unsigned Track)
{
	if (Track >= m_pCollection.size())
		return nullptr;
	CBookmarkCollection *pCol = m_pCollection[Track].release();
	m_pCollection[Track] = std::make_unique<CBookmarkCollection>();
	return pCol;
}

void CBookmarkManager::SetCollection(unsigned Track, CBookmarkCollection *const pCol)
{
	m_pCollection[Track].reset(pCol);
}

unsigned int CBookmarkManager::GetBookmarkCount() const
{
	int Total = 0;
	for (const auto &ptr : m_pCollection)
		Total += ptr->GetCount();
	return Total;
}

void CBookmarkManager::InsertTrack(unsigned Track)
{
	m_pCollection.pop_back();
	m_pCollection.insert(m_pCollection.begin() + Track, std::make_unique<CBookmarkCollection>());
}

void CBookmarkManager::RemoveTrack(unsigned Track)
{
	m_pCollection.erase(m_pCollection.begin() + Track);
	m_pCollection.insert(m_pCollection.end(), std::make_unique<CBookmarkCollection>());
}

void CBookmarkManager::SwapTracks(unsigned A, unsigned B)
{
	m_pCollection[A].swap(m_pCollection[B]);
}
