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
