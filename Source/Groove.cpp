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
#include "Groove.h"

CGroove::CGroove(int Speed)
{
	Clear(Speed);
}

void CGroove::Copy(const CGroove *Source)
{
	SetSize(Source->GetSize());
	memcpy(m_iEntry, Source->m_iEntry, MAX_GROOVE_SIZE);
}

void CGroove::Clear(unsigned char Speed)
{
	m_iLength = (Speed > 0);
	memset(m_iEntry, 0, MAX_GROOVE_SIZE);
	SetEntry(0, Speed);
}

unsigned char CGroove::GetEntry(int Index) const
{
	if (m_iLength > 0)
		return m_iEntry[Index % m_iLength];
	else
		return 6; // return DEFAULT_SPEED;
}

void CGroove::SetEntry(unsigned char Index, unsigned char Value)
{
	if (Index >= m_iLength) return;
	m_iEntry[Index] = Value;
}

unsigned char CGroove::GetSize() const
{
	return m_iLength;
}

void CGroove::SetSize(unsigned char Size)
{
	if (Size > MAX_GROOVE_SIZE) return;
	if (m_iLength < Size) for (unsigned char i = m_iLength; i < Size; i++)
		m_iEntry[i] = 0;
	m_iLength = Size;
}

float CGroove::GetAverage() const
{
	float Total = 0;
	if (!m_iLength) return 6.0; // return DEFAULT_SPEED;
	else {
		for (unsigned char i = 0; i < m_iLength; i++) Total += m_iEntry[i];
		return Total / m_iLength;
	}
}