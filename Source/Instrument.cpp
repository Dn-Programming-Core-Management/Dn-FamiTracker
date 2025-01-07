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
#include "InstrumentManagerInterface.h"		// // //
#include "Instrument.h"

/*
 * Class CInstrument, base class for instruments
 *
 */

CInstrument::CInstrument(inst_type_t type) : m_iType(type), m_pInstManager(nullptr)		// // //
{
	memset(m_cName, 0, INST_NAME_MAX);
}

CInstrument::~CInstrument()
{
}

void CInstrument::CloneFrom(const CInstrument *pSeq)
{
	SetName(pSeq->GetName());
	m_iType = pSeq->GetType();
}

void CInstrument::SetName(const char *Name)
{
	strncpy_s(m_cName, Name, INST_NAME_MAX);
	InstrumentChanged();		// // //
}

void CInstrument::GetName(char *Name) const
{
	strncpy_s(Name, INST_NAME_MAX, m_cName, INST_NAME_MAX);
}

const char *CInstrument::GetName() const
{
	return m_cName;
}

void CInstrument::RegisterManager(CInstrumentManagerInterface *pManager)		// // //
{
	m_pInstManager = pManager;
}

inst_type_t CInstrument::GetType() const		// // //
{
	return m_iType;
}

void CInstrument::InstrumentChanged() const
{
	// Set modified flag
	if (m_pInstManager)		// // //
		m_pInstManager->InstrumentChanged();
}

// File load / store

void CInstrumentFile::WriteInt(unsigned int Value)
{
	Write(&Value, sizeof(int));
}

void CInstrumentFile::WriteChar(unsigned char Value)
{
	Write(&Value, sizeof(char));
}

unsigned int CInstrumentFile::ReadInt()
{
	unsigned int Value;
	Read(&Value, sizeof(int));
	return Value;
}

unsigned char CInstrumentFile::ReadChar()
{
	unsigned char Value;
	Read(&Value, sizeof(char));
	return Value;
}
