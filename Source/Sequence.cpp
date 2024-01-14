/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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
#include "Sequence.h"

CSequence::CSequence()
{
	Clear();
}

void CSequence::Clear()
{
	m_iItemCount = 0;
	m_iLoopPoint = -1;
	m_iReleasePoint = -1;
	m_iSetting = SETTING_DEFAULT;		// // //

	memset(m_cValues, 0, sizeof(char) * MAX_SEQUENCE_ITEMS);

	m_iPlaying = -1;
}

bool CSequence::operator==(const CSequence &other)		// // //
{
	return m_iItemCount == other.m_iItemCount &&
		m_iLoopPoint == other.m_iLoopPoint &&
		m_iReleasePoint == other.m_iReleasePoint &&
		m_iSetting == other.m_iSetting &&
		memcmp(m_cValues, other.m_cValues, m_iItemCount * sizeof(*m_cValues)) == 0;
}

void CSequence::SetItem(int Index, signed char Value)
{
	m_cValues[Index] = Value;
}

void CSequence::SetItemCount(unsigned int Count)
{
	ASSERT(Count <= MAX_SEQUENCE_ITEMS);
	
	m_iItemCount = Count;

	if (m_iLoopPoint > m_iItemCount)
		m_iLoopPoint = -1;
	if (m_iReleasePoint > m_iItemCount)
		m_iReleasePoint = -1;
}

void CSequence::SetLoopPoint(unsigned int Point)
{
	m_iLoopPoint = Point;
	if (m_iLoopPoint > m_iItemCount)		// // //
		m_iLoopPoint = -1;
}

void CSequence::SetReleasePoint(unsigned int Point)
{
	m_iReleasePoint = Point;
	if (m_iReleasePoint > m_iItemCount)		// // //
		m_iReleasePoint = -1;
}

void CSequence::SetSetting(seq_setting_t Setting)		// // //
{
	m_iSetting = Setting;
}

signed char CSequence::GetItem(int Index) const
{
	return m_cValues[Index];
}

unsigned int CSequence::GetItemCount() const
{
	return m_iItemCount;
}

unsigned int CSequence::GetLoopPoint() const
{
	return m_iLoopPoint;
}

unsigned int CSequence::GetReleasePoint() const
{
	return m_iReleasePoint;
}

unsigned int CSequence::GetSetting() const		// // //
{
	return m_iSetting;
}

void CSequence::Copy(const CSequence *pSeq)
{
	// Copy all values from pSeq
	m_iItemCount = pSeq->m_iItemCount;
	m_iLoopPoint = pSeq->m_iLoopPoint;
	m_iReleasePoint = pSeq->m_iReleasePoint;
	m_iSetting = pSeq->m_iSetting;

	memcpy(m_cValues, pSeq->m_cValues, MAX_SEQUENCE_ITEMS);
}
