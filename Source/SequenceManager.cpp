/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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

#include "stdafx.h"
#include <vector>
#include <memory>
#include "Sequence.h"
#include "SequenceCollection.h"
#include "SequenceManager.h"

CSequenceManager::CSequenceManager(int Count)
{
	m_pCollection.resize(Count);
	for (int i = 0; i < Count; ++i)
		m_pCollection[i].reset(new CSequenceCollection());
}

int CSequenceManager::GetCount() const
{
	return static_cast<int>(m_pCollection.size());
}

CSequenceCollection *CSequenceManager::GetCollection(unsigned int Index)
{
	if (Index >= m_pCollection.size()) return nullptr;
	if (!m_pCollection[Index])
		m_pCollection[Index].reset(new CSequenceCollection());
	return m_pCollection[Index].get();
}

const CSequenceCollection *CSequenceManager::GetCollection(unsigned int Index) const
{
	if (Index >= m_pCollection.size()) return nullptr;
	return m_pCollection[Index].get();
}
