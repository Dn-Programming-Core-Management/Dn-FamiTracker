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

#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"

/*
 * Class CInstrument, base class for instruments
 *
 */

CInstrument::CInstrument() : CRefCounter(), m_iType(0)
{
	memset(m_cName, 0, INST_NAME_MAX);
}

CInstrument::~CInstrument()
{
}

void CInstrument::SetName(const char *Name)
{
	strncpy(m_cName, Name, INST_NAME_MAX);
}

void CInstrument::GetName(char *Name) const
{
	strncpy(Name, m_cName, INST_NAME_MAX);
}

const char *CInstrument::GetName() const
{
	return m_cName;
}

void CInstrument::InstrumentChanged() const
{
	// Set modified flag
	CFrameWnd *pFrameWnd = dynamic_cast<CFrameWnd*>(AfxGetMainWnd());
	if (pFrameWnd != NULL) {
		CFamiTrackerDoc *pDoc = (CFamiTrackerDoc*)pFrameWnd->GetActiveDocument();		// // //
		if (pDoc != NULL)
			pDoc->SetModifiedFlag();
			pDoc->SetExceededFlag();		// // //
	}
}

// Reference counting

CRefCounter::CRefCounter() : m_iRefCounter(1)
{
}

CRefCounter::~CRefCounter()
{
	ASSERT(m_iRefCounter == 0);
}

void CRefCounter::Retain()
{
	ASSERT(m_iRefCounter > 0);

	InterlockedIncrement((volatile LONG*)&m_iRefCounter);
}

void CRefCounter::Release()
{
	ASSERT(m_iRefCounter > 0);

	InterlockedDecrement((volatile LONG*)&m_iRefCounter);

	if (!m_iRefCounter)
		delete this;
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
