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

#include <memory>
#include "stdafx.h"
#include "SequenceManager.h"
#include "SequenceCollection.h"

CSequenceManager::CSequenceManager(int Count) :
	m_iCount(Count)
{
	m_pCollection = new CSequenceCollection*[Count]();
}

CSequenceManager::~CSequenceManager()
{
	for (int i = 0; i < m_iCount; i++) {
		if (m_pCollection[i] != nullptr)
			m_pCollection[i]->RemoveAll();
		SAFE_RELEASE(m_pCollection[i]);
	}
	SAFE_RELEASE_ARRAY(m_pCollection);
}

CSequenceCollection *CSequenceManager::GetCollection(int Index)
{
	ASSERT(Index >= 0 && Index < m_iCount);
	if (m_pCollection[Index] == nullptr)
		m_pCollection[Index] = new CSequenceCollection();
	return m_pCollection[Index];
}

const CSequenceCollection *CSequenceManager::GetCollection(int Index) const
{
	ASSERT(Index >= 0 && Index < m_iCount);
	return m_pCollection[Index];
}
