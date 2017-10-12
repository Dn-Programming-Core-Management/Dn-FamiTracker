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

#include "InstrumentVRC7.h"		// // //
#include "ModuleException.h"		// // //
#include "Chunk.h"
#include "DocumentFile.h"
#include "SimpleFile.h"

/*
 * class CInstrumentVRC7
 *
 */

static const unsigned char VRC7_SINE_PATCH[] = {0x01, 0x21, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x0F};		// // //

CInstrumentVRC7::CInstrumentVRC7() : CInstrument(INST_VRC7), m_iPatch(0)		// // //
{
	memcpy(m_iRegs, VRC7_SINE_PATCH, sizeof(VRC7_SINE_PATCH));
}

CInstrument *CInstrumentVRC7::Clone() const
{
	CInstrumentVRC7 *inst = new CInstrumentVRC7();		// // //
	inst->CloneFrom(this);
	return inst;
}

void CInstrumentVRC7::CloneFrom(const CInstrument *pInst)
{
	CInstrument::CloneFrom(pInst);
	
	if (auto pNew = dynamic_cast<const CInstrumentVRC7*>(pInst)) {
		SetPatch(pNew->GetPatch());
		for (int i = 0; i < 8; ++i)
			SetCustomReg(i, pNew->GetCustomReg(i));
	}
}

void CInstrumentVRC7::Store(CDocumentFile *pDocFile) const
{
	pDocFile->WriteBlockInt(m_iPatch);

	for (int i = 0; i < 8; ++i)
		pDocFile->WriteBlockChar(GetCustomReg(i));
}

bool CInstrumentVRC7::Load(CDocumentFile *pDocFile)
{
	m_iPatch = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), 0, 0xF, "VRC7 patch number");

	for (int i = 0; i < 8; ++i)
		SetCustomReg(i, pDocFile->GetBlockChar());

	return true;
}

void CInstrumentVRC7::SaveFile(CSimpleFile *pFile) const
{
	pFile->WriteInt(m_iPatch);

	for (int i = 0; i < 8; ++i)
		pFile->WriteChar(GetCustomReg(i));
}

bool CInstrumentVRC7::LoadFile(CSimpleFile *pFile, int iVersion)
{
	m_iPatch = pFile->ReadInt();

	for (int i = 0; i < 8; ++i)
		SetCustomReg(i, pFile->ReadChar());

	return true;
}

int CInstrumentVRC7::Compile(CChunk *pChunk, int Index) const
{
	int Patch = GetPatch();

	pChunk->StoreByte(6);		// // // CHAN_VRC7
	pChunk->StoreByte(Patch << 4);	// Shift up by 4 to make room for volume

	if (Patch == 0) {
		// Write custom patch settings
		for (int i = 0; i < 8; ++i) {
			pChunk->StoreByte(GetCustomReg(i));
		}
	}

	return (Patch == 0) ? 10 : 2;		// // //
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
