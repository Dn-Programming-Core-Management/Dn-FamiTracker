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
