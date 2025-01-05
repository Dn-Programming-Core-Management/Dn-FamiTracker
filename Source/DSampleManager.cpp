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
			return static_cast<int>(i);
	return -1;
}

unsigned int CDSampleManager::GetTotalSize() const
{
	return m_iTotalSize;
}

