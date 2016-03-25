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

#include <vector>
#include <memory>
#include "stdafx.h"
#include "Sequence.h"		// // //
#include "ModuleException.h"		// // //
#include "Instrument.h"
#include "SeqInstrument.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "Chunk.h"
#include "ChunkRenderText.h"		// // //
#include "DocumentFile.h"

const char TEST_WAVE[] = {
	00, 01, 12, 22, 32, 36, 39, 39, 42, 47, 47, 50, 48, 51, 54, 58,
	54, 55, 49, 50, 52, 61, 63, 63, 59, 56, 53, 51, 48, 47, 41, 35,
	35, 35, 41, 47, 48, 51, 53, 56, 59, 63, 63, 61, 52, 50, 49, 55,
	54, 58, 54, 51, 48, 50, 47, 47, 42, 39, 39, 36, 32, 22, 12, 01
};

const int FIXED_FDS_INST_SIZE = 2 + 16 + 4 + 1;		// // //

CInstrumentFDS::CInstrumentFDS() : CSeqInstrument(INST_FDS),		// // //
	m_iModulationSpeed(0),
	m_iModulationDepth(0),
	m_iModulationDelay(0),
	m_bModulationEnable(true),
	m_iModulation()
{
	memcpy(m_iSamples, TEST_WAVE, WAVE_SIZE);	
	m_pSequence.resize(SEQ_COUNT);
	for (int i = 0; i < SEQ_COUNT; ++i)
		m_pSequence[i].reset(new CSequence());
}

CInstrument *CInstrumentFDS::Clone() const
{
	CInstrumentFDS *pNewInst = new CInstrumentFDS();

	// Copy parameters
	memcpy(pNewInst->m_iSamples, m_iSamples, WAVE_SIZE);
	memcpy(pNewInst->m_iModulation, m_iModulation, MOD_SIZE);
	pNewInst->m_iModulationDelay = m_iModulationDelay;
	pNewInst->m_iModulationDepth = m_iModulationDepth;
	pNewInst->m_iModulationSpeed = m_iModulationSpeed;

	// Copy sequences
	for (int i = 0; i < SEQUENCE_COUNT; ++i)		// // //
		m_pSequence[i]->Copy(pNewInst->GetSequence(i));

	// Copy name
	pNewInst->SetName(GetName());

	return pNewInst;
}

void CInstrumentFDS::Setup()
{
}

void CInstrumentFDS::StoreInstSequence(CInstrumentFile *pFile, const CSequence *pSeq)
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

CSequence *CInstrumentFDS::LoadInstSequence(CInstrumentFile *pFile) const
{
	int SeqCount = CModuleException::AssertRangeFmt(pFile->ReadInt(), 0U, 0xFFU, "Sequence item count", "%u");
	int Loop = CModuleException::AssertRangeFmt(static_cast<int>(pFile->ReadInt()), -1, SeqCount - 1, "Sequence loop point", "%u");
	int Release = CModuleException::AssertRangeFmt(static_cast<int>(pFile->ReadInt()), -1, SeqCount - 1, "Sequence release point", "%u");

	CSequence *pSeq = new CSequence();
	pSeq->SetItemCount(SeqCount > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : SeqCount);
	pSeq->SetLoopPoint(Loop);
	pSeq->SetReleasePoint(Release);
	pSeq->SetSetting(static_cast<seq_setting_t>(pFile->ReadInt()));		// // //

	for (int i = 0; i < SeqCount; ++i)
		pSeq->SetItem(i, pFile->ReadChar());

	return pSeq;
}

void CInstrumentFDS::StoreSequence(CDocumentFile *pDocFile, const CSequence *pSeq)
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

CSequence *CInstrumentFDS::LoadSequence(CDocumentFile *pDocFile) const
{
	int SeqCount = static_cast<unsigned char>(pDocFile->GetBlockChar());
	unsigned int LoopPoint = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), -1, SeqCount - 1, "Sequence loop point", "%i");;
	unsigned int ReleasePoint = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), -1, SeqCount - 1, "Sequence release point", "%i");;

	// CModuleException::AssertRangeFmt(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count", "%i");

	CSequence *pSeq = new CSequence();
	pSeq->SetItemCount(SeqCount > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : SeqCount);
	pSeq->SetLoopPoint(LoopPoint);
	pSeq->SetReleasePoint(ReleasePoint);
	pSeq->SetSetting(static_cast<seq_setting_t>(pDocFile->GetBlockInt()));		// // //

	for (int x = 0; x < SeqCount; ++x) {
		char Value = pDocFile->GetBlockChar();
		pSeq->SetItem(x, Value);
	}

	return pSeq;
}

void CInstrumentFDS::DoubleVolume() const
{
	CSequence *pVol = m_pSequence[SEQ_VOLUME].get();
	for (unsigned int i = 0; i < pVol->GetItemCount(); ++i)
		pVol->SetItem(i, pVol->GetItem(i) * 2);
}

void CInstrumentFDS::Store(CDocumentFile *pDocFile)
{
	// Write wave
	for (int i = 0; i < WAVE_SIZE; ++i) {
		pDocFile->WriteBlockChar(GetSample(i));
	}

	// Write modulation table
	for (int i = 0; i < MOD_SIZE; ++i) {
		pDocFile->WriteBlockChar(GetModulation(i));
	}

	// Modulation parameters
	pDocFile->WriteBlockInt(GetModulationSpeed());
	pDocFile->WriteBlockInt(GetModulationDepth());
	pDocFile->WriteBlockInt(GetModulationDelay());

	// Sequences
	for (int i = 0; i < SEQUENCE_COUNT; ++i)		// // //
		StoreSequence(pDocFile, GetSequence(i));
}

bool CInstrumentFDS::Load(CDocumentFile *pDocFile)
{
	for (int i = 0; i < WAVE_SIZE; ++i) {
		SetSample(i, pDocFile->GetBlockChar());
	}

	for (int i = 0; i < MOD_SIZE; ++i) {
		SetModulation(i, pDocFile->GetBlockChar());
	}

	SetModulationSpeed(pDocFile->GetBlockInt());
	SetModulationDepth(pDocFile->GetBlockInt());
	SetModulationDelay(pDocFile->GetBlockInt());

	// hack to fix earlier saved files (remove this eventually)
/*
	if (pDocFile->GetBlockVersion() > 2) {
		LoadSequence(pDocFile, GetSequence(SEQ_VOLUME));
		LoadSequence(pDocFile, GetSequence(SEQ_ARPEGGIO));
		if (pDocFile->GetBlockVersion() > 2)
			LoadSequence(pDocFile, GetSequence(SEQ_PITCH));
	}
	else {
*/
	unsigned int a = pDocFile->GetBlockInt();
	unsigned int b = pDocFile->GetBlockInt();
	pDocFile->RollbackPointer(8);

	if (a < 256 && (b & 0xFF) != 0x00) {
	}
	else {
		SetSequence(SEQ_VOLUME, LoadSequence(pDocFile));
		SetSequence(SEQ_ARPEGGIO, LoadSequence(pDocFile));
		//
		// Note: Remove this line when files are unable to load 
		// (if a file contains FDS instruments but FDS is disabled)
		// this was a problem in an earlier version.
		//
		if (pDocFile->GetBlockVersion() > 2)
			SetSequence(SEQ_PITCH, LoadSequence(pDocFile));
	}

//	}

	// Older files was 0-15, new is 0-31
	if (pDocFile->GetBlockVersion() <= 3) DoubleVolume();

	return true;
}

void CInstrumentFDS::SaveFile(CInstrumentFile *pFile)
{
	// Write wave
	for (int i = 0; i < WAVE_SIZE; ++i) {
		pFile->WriteChar(GetSample(i));
	}

	// Write modulation table
	for (int i = 0; i < MOD_SIZE; ++i) {
		pFile->WriteChar(GetModulation(i));
	}

	// Modulation parameters
	pFile->WriteInt(GetModulationSpeed());
	pFile->WriteInt(GetModulationDepth());
	pFile->WriteInt(GetModulationDelay());

	// Sequences
	for (int i = 0; i < SEQUENCE_COUNT; ++i)
		StoreInstSequence(pFile, GetSequence(i));
}

bool CInstrumentFDS::LoadFile(CInstrumentFile *pFile, int iVersion)
{
	// Read wave
	for (int i = 0; i < WAVE_SIZE; ++i) {
		SetSample(i, pFile->ReadChar());
	}

	// Read modulation table
	for (int i = 0; i < MOD_SIZE; ++i) {
		SetModulation(i, pFile->ReadChar());
	}

	// Modulation parameters
	SetModulationSpeed(pFile->ReadInt());
	SetModulationDepth(pFile->ReadInt());
	SetModulationDelay(pFile->ReadInt());

	// Sequences
	for (int i = 0; i < SEQUENCE_COUNT; ++i)		// // //
		SetSequence(i, LoadInstSequence(pFile));

	if (iVersion <= 22) DoubleVolume();

	return true;
}

int CInstrumentFDS::Compile(CChunk *pChunk, int Index)
{
	CStringA str;

	// Store wave
//	int Table = pCompiler->AddWavetable(m_iSamples);
//	int Table = 0;
//	pChunk->StoreByte(Table);

	pChunk->StoreByte(7);		// // // CHAN_FDS

	// Store sequences
	char Switch = 0;		// // //
	int size = FIXED_FDS_INST_SIZE;
	for (int i = 0; i < SEQUENCE_COUNT; ++i)
		if (GetSequence(i)->GetItemCount() > 0) {
			Switch |= 1 << i;
			size += 2;
		}
	pChunk->StoreByte(Switch);

	for (int i = 0; i < SEQUENCE_COUNT; ++i)
		if (Switch & (1 << i)) {
			str.Format(CChunkRenderText::LABEL_SEQ_FDS, Index * 5 + i);
			pChunk->StoreReference(str);
		}

	// // // Store modulation table, two entries/byte
	for (int i = 0; i < 16; ++i) {
		char Data = GetModulation(i << 1) | (GetModulation((i << 1) + 1) << 3);
		pChunk->StoreByte(Data);
	}
	
	pChunk->StoreByte(GetModulationDelay());
	pChunk->StoreByte(GetModulationDepth());
	pChunk->StoreWord(GetModulationSpeed());

	return size;
}

bool CInstrumentFDS::CanRelease() const
{
	const CSequence *pVol = GetSequence(SEQ_VOLUME);
	return pVol && pVol->GetItemCount() && pVol->GetReleasePoint() != -1;
}

unsigned char CInstrumentFDS::GetSample(int Index) const
{
	ASSERT(Index < WAVE_SIZE);
	return m_iSamples[Index];
}

void CInstrumentFDS::SetSample(int Index, int Sample)
{
	ASSERT(Index < WAVE_SIZE);
	if (m_iSamples[Index] != Sample)		// // //
		InstrumentChanged();
	m_iSamples[Index] = Sample;
}

int CInstrumentFDS::GetModulation(int Index) const
{
	return m_iModulation[Index];
}

void CInstrumentFDS::SetModulation(int Index, int Value)
{
	if (m_iModulation[Index] != Value)		// // //
		InstrumentChanged();
	m_iModulation[Index] = Value;
}

int CInstrumentFDS::GetModulationSpeed() const
{
	return m_iModulationSpeed;
}

void CInstrumentFDS::SetModulationSpeed(int Speed)
{
	if (m_iModulationSpeed != Speed)		// // //
		InstrumentChanged();
	m_iModulationSpeed = Speed;
}

int CInstrumentFDS::GetModulationDepth() const
{
	return m_iModulationDepth;
}

void CInstrumentFDS::SetModulationDepth(int Depth)
{
	if (m_iModulationDepth != Depth)		// // //
		InstrumentChanged();
	m_iModulationDepth = Depth;
}

int CInstrumentFDS::GetModulationDelay() const
{
	return m_iModulationDelay;
}

void CInstrumentFDS::SetModulationDelay(int Delay)
{
	if (m_iModulationDelay != Delay)		// // //
		InstrumentChanged();
	m_iModulationDelay = Delay;
}

bool CInstrumentFDS::GetModulationEnable() const
{
	return m_bModulationEnable;
}

void CInstrumentFDS::SetModulationEnable(bool Enable)
{
	if (m_bModulationEnable != Enable)			// // //
		InstrumentChanged();
	m_bModulationEnable = Enable;
}

int	CInstrumentFDS::GetSeqEnable(int Index) const
{
	return Index < SEQUENCE_COUNT; // && m_iSeqEnable[Index];
}

int	CInstrumentFDS::GetSeqIndex(int Index) const
{
	ASSERT(false);
	return 0;
}

void CInstrumentFDS::SetSeqIndex(int Index, int Value)
{
	ASSERT(false);
}

CSequence *CInstrumentFDS::GetSequence(int SeqType) const		// // //
{
	return m_pSequence[SeqType].get();
}

void CInstrumentFDS::SetSequence(int SeqType, CSequence *pSeq)
{
	m_pSequence[SeqType].reset(pSeq);
}
