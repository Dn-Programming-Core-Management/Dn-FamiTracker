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

const int CSequenceCollection::MAX_SEQUENCES = 128;

CSequenceCollection::CSequenceCollection()
{
	m_pSequence.resize(MAX_SEQUENCES);
}

CSequence *CSequenceCollection::GetSequence(unsigned int Index)
{
	if (!m_pSequence[Index])
		m_pSequence[Index].reset(new CSequence());
	return m_pSequence[Index].get();
}

void CSequenceCollection::SetSequence(unsigned int Index, CSequence *Seq)
{
	m_pSequence[Index].reset(Seq);
}

const CSequence *CSequenceCollection::GetSequence(unsigned int Index) const
{
	return m_pSequence[Index].get();
}

unsigned int CSequenceCollection::GetFirstFree() const
{
	for (int i = 0; i < MAX_SEQUENCES; i++)
		if (m_pSequence[i] == nullptr || !m_pSequence[i]->GetItemCount())
			return i;
	return -1;
}

void CSequenceCollection::RemoveAll()
{
	for (auto it = m_pSequence.begin(); it < m_pSequence.end(); ++it)
		it->reset();
}
