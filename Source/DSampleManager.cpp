/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include <vector>
#include <memory>
#include "DSampleManager.h"

const unsigned int CDSampleManager::MAX_DSAMPLES = 64;

CDSampleManager::CDSampleManager() : m_pDSample(), m_iTotalSize(0U)
{
	m_pDSample.resize(MAX_DSAMPLES);
}

const CDSample *CDSampleManager::GetDSample(unsigned Index) const
{
	return m_pDSample[Index].get();
}

bool CDSampleManager::SetDSample(unsigned Index, CDSample *pSamp)
{
	bool Changed = m_pDSample[Index].get() != pSamp;
	if (m_pDSample[Index])
		m_iTotalSize -= m_pDSample[Index]->GetSize();
	if (pSamp)
		m_iTotalSize += pSamp->GetSize();
	m_pDSample[Index].reset(pSamp);
	return Changed;
}

bool CDSampleManager::IsSampleUsed(unsigned Index) const
{
	return m_pDSample[Index] != nullptr;
}

unsigned int CDSampleManager::GetSampleCount() const
{
	unsigned int Count = 0;
	for (size_t i = 0; i < MAX_DSAMPLES; ++i)
		if (m_pDSample[i])
			++Count;
	return Count;
}

unsigned int CDSampleManager::GetFirstFree() const
{
	for (size_t i = 0; i < MAX_DSAMPLES; ++i)
		if (!m_pDSample[i])
			return i;
	return -1;
}

unsigned int CDSampleManager::GetTotalSize() const
{
	return m_iTotalSize;
}

