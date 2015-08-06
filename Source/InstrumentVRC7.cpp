/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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
#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"
#include "Compiler.h"
#include "Chunk.h"
#include "DocumentFile.h"

/*
 * class CInstrumentVRC7
 *
 */

CInstrumentVRC7::CInstrumentVRC7() :
	m_iPatch(0)
{
	m_iRegs[0] = 0x01;
	m_iRegs[1] = 0x21;
	m_iRegs[2] = 0x00;
	m_iRegs[3] = 0x00;
	m_iRegs[4] = 0x00;
	m_iRegs[5] = 0xF0;
	m_iRegs[6] = 0x00;
	m_iRegs[7] = 0x0F;
}

CInstrument *CInstrumentVRC7::Clone() const
{
	CInstrumentVRC7 *pNew = new CInstrumentVRC7();

	pNew->SetPatch(GetPatch());

	for (int i = 0; i < 8; ++i)
		pNew->SetCustomReg(i, GetCustomReg(i));

	pNew->SetName(GetName());

	return pNew;
}

void CInstrumentVRC7::Setup()
{
}

void CInstrumentVRC7::Store(CDocumentFile *pDocFile)
{
	pDocFile->WriteBlockInt(m_iPatch);

	for (int i = 0; i < 8; ++i)
		pDocFile->WriteBlockChar(GetCustomReg(i));
}

bool CInstrumentVRC7::Load(CDocumentFile *pDocFile)
{
	m_iPatch = pDocFile->GetBlockInt();

	for (int i = 0; i < 8; ++i)
		SetCustomReg(i, pDocFile->GetBlockChar());

	return true;
}

void CInstrumentVRC7::SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc)
{
	pFile->WriteInt(m_iPatch);

	for (int i = 0; i < 8; ++i)
		pFile->WriteChar(GetCustomReg(i));
}

bool CInstrumentVRC7::LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
{
	m_iPatch = pFile->ReadInt();

	for (int i = 0; i < 8; ++i)
		SetCustomReg(i, pFile->ReadChar());

	return true;
}

int CInstrumentVRC7::Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index)
{
	int Patch = GetPatch();

	pChunk->StoreByte(Patch << 4);	// Shift up by 4 to make room for volume

	if (Patch == 0) {
		// Write custom patch settings
		for (int i = 0; i < 8; ++i) {
			pChunk->StoreByte(GetCustomReg(i));
		}
	}

	return (Patch == 0) ? 9 : 1;
}

bool CInstrumentVRC7::CanRelease() const
{
	return false;	// This can use release but disable it when previewing notes
}

void CInstrumentVRC7::SetPatch(unsigned int Patch)
{
	m_iPatch = Patch;
	InstrumentChanged();
}

unsigned int CInstrumentVRC7::GetPatch() const
{
	return m_iPatch;
}

void CInstrumentVRC7::SetCustomReg(int Reg, unsigned char Value)		// // //
{
	m_iRegs[Reg] = Value;
	InstrumentChanged();
}

unsigned char CInstrumentVRC7::GetCustomReg(int Reg) const		// // //
{
	return m_iRegs[Reg];
}
