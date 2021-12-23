/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include <vector>		// // //
#include <memory>		// // //
#include "stdafx.h"
#include "Sequence.h"		// // //
#include "ModuleException.h"		// // //
#include "Instrument.h"
#include "SeqInstrument.h"		// // //
#include "InstrumentVRC7.h"		// // //
#include "Chunk.h"
#include "ChunkRenderText.h"		// // //
#include "DocumentFile.h"

/*
 * class CInstrumentVRC7
 *
 */

static const unsigned char VRC7_SINE_PATCH[] = {0x01, 0x21, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x0F};		// // //
LPCTSTR CInstrumentVRC7::SEQUENCE_NAME[] = { _T("Volume"), _T("Arpeggio"), _T("Pitch"), _T("Hi-pitch"), _T("Patch") };		// // //

CInstrumentVRC7::CInstrumentVRC7() : CSeqInstrument(INST_VRC7), m_iPatch(0)		// // //
{
	memcpy(m_iRegs, VRC7_SINE_PATCH, sizeof(VRC7_SINE_PATCH));
	m_pSequence.resize(SEQ_COUNT);		// // //
	for (int i = 0; i < SEQ_COUNT; ++i)		// // //
		m_pSequence[i].reset(new CSequence());
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
		for (int i = 0; i < SEQUENCE_COUNT; ++i)		// // //
			SetSequence(i, new CSequence(*pNew->GetSequence(i)));
	}
}

void CInstrumentVRC7::Setup()
{
}


void CInstrumentVRC7::StoreInstSequence(CInstrumentFile* pFile, const CSequence* pSeq)		// // //
{
	// Store number of items in this sequence
	pFile->WriteInt(pSeq->GetItemCount());
	// Store loop point
	pFile->WriteInt(pSeq->GetLoopPoint());
	// Store release point (v4)
	pFile->WriteInt(pSeq->GetReleasePoint());
	// Store setting (v4)
	pFile->WriteInt(pSeq->GetSetting());
	// Store items
	for (unsigned i = 0; i < pSeq->GetItemCount(); ++i)
		pFile->WriteChar(pSeq->GetItem(i));
}

CSequence* CInstrumentVRC7::LoadInstSequence(CInstrumentFile* pFile) const		// // //
{
	int SeqCount = CModuleException::AssertRangeFmt(pFile->ReadInt(), 0U, 0xFFU, "Sequence item count", "%u");
	int Loop = CModuleException::AssertRangeFmt(static_cast<int>(pFile->ReadInt()), -1, SeqCount - 1, "Sequence loop point", "%u");
	int Release = CModuleException::AssertRangeFmt(static_cast<int>(pFile->ReadInt()), -1, SeqCount - 1, "Sequence release point", "%u");

	CSequence* pSeq = new CSequence();
	pSeq->SetItemCount(SeqCount > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : SeqCount);
	pSeq->SetLoopPoint(Loop);
	pSeq->SetReleasePoint(Release);
	pSeq->SetSetting(static_cast<seq_setting_t>(pFile->ReadInt()));

	for (int i = 0; i < SeqCount; ++i)
		pSeq->SetItem(i, pFile->ReadChar());

	return pSeq;
}

void CInstrumentVRC7::StoreSequence(CDocumentFile* pDocFile, const CSequence* pSeq)		// // //
{
	// Store number of items in this sequence
	pDocFile->WriteBlockChar(pSeq->GetItemCount());
	// Store loop point
	pDocFile->WriteBlockInt(pSeq->GetLoopPoint());
	// Store release point (v4)
	pDocFile->WriteBlockInt(pSeq->GetReleasePoint());
	// Store setting (v4)
	pDocFile->WriteBlockInt(pSeq->GetSetting());
	// Store items
	for (unsigned int j = 0; j < pSeq->GetItemCount(); j++) {
		pDocFile->WriteBlockChar(pSeq->GetItem(j));
	}
}

CSequence* CInstrumentVRC7::LoadSequence(CDocumentFile* pDocFile) const		// // //
{
	int SeqCount = static_cast<unsigned char>(pDocFile->GetBlockChar());
	unsigned int LoopPoint = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), -1, SeqCount - 1, "Sequence loop point", "%i");;
	unsigned int ReleasePoint = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), -1, SeqCount - 1, "Sequence release point", "%i");;

	// CModuleException::AssertRangeFmt(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count", "%i");

	CSequence* pSeq = new CSequence();
	pSeq->SetItemCount(SeqCount > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : SeqCount);
	pSeq->SetLoopPoint(LoopPoint);
	pSeq->SetReleasePoint(ReleasePoint);
	pSeq->SetSetting(static_cast<seq_setting_t>(pDocFile->GetBlockInt()));

	for (int x = 0; x < SeqCount; ++x) {
		char Value = pDocFile->GetBlockChar();
		pSeq->SetItem(x, Value);
	}

	return pSeq;
}

void CInstrumentVRC7::Store(CDocumentFile *pDocFile)
{
	pDocFile->WriteBlockInt(m_iPatch);

	for (int i = 0; i < 8; ++i)
		pDocFile->WriteBlockChar(GetCustomReg(i));

	for (int i = 0; i < SEQUENCE_COUNT; ++i)		// // //
		StoreSequence(pDocFile, GetSequence(i));
}

bool CInstrumentVRC7::Load(CDocumentFile *pDocFile)
{
	m_iPatch = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), 0, 0xF, "VRC7 patch number", "%i");

	for (int i = 0; i < 8; ++i)
		SetCustomReg(i, pDocFile->GetBlockChar());


	if (pDocFile->GetFileVersion() >= pDocFile->FILE_VER) {		// // //
		SetSequence(SEQ_VOLUME, LoadSequence(pDocFile));
		SetSequence(SEQ_ARPEGGIO, LoadSequence(pDocFile));
		SetSequence(SEQ_PITCH, LoadSequence(pDocFile));
		SetSequence(SEQ_HIPITCH, LoadSequence(pDocFile));
		SetSequence(SEQ_DUTYCYCLE, LoadSequence(pDocFile));
	}

	return true;
}

void CInstrumentVRC7::SaveFile(CInstrumentFile *pFile)
{
	pFile->WriteInt(m_iPatch);

	for (int i = 0; i < 8; ++i)
		pFile->WriteChar(GetCustomReg(i));

	// Sequences
	for (int i = 0; i < SEQUENCE_COUNT; ++i)
		StoreInstSequence(pFile, GetSequence(i));
}

bool CInstrumentVRC7::LoadFile(CInstrumentFile *pFile, int iVersion)
{
	m_iPatch = pFile->ReadInt();

	for (int i = 0; i < 8; ++i)
		SetCustomReg(i, pFile->ReadChar());

	// Sequences
	if (iVersion >= 25)
		for (int i = 0; i < SEQUENCE_COUNT; ++i)		// // //
			SetSequence(i, LoadInstSequence(pFile));

	return true;
}

int CInstrumentVRC7::Compile(CChunk *pChunk, int Index)
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


int	CInstrumentVRC7::GetSeqEnable(int Index) const		// // //
{
	return Index < SEQUENCE_COUNT;
}

int	CInstrumentVRC7::GetSeqIndex(int Index) const		// // //
{
	ASSERT(false);
	return 0;
}

void CInstrumentVRC7::SetSeqIndex(int Index, int Value)		// // //
{
	ASSERT(false);
}

CSequence* CInstrumentVRC7::GetSequence(int SeqType) const		// // //
{
	return m_pSequence[SeqType].get();
}

void CInstrumentVRC7::SetSequence(int SeqType, CSequence* pSeq)		// // //
{
	m_pSequence[SeqType].reset(pSeq);
}
