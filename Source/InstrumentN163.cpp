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
#include "ModuleException.h"		// // //
#include "DocumentFile.h"
#include "Instrument.h"
#include "SeqInstrument.h"		// // //
#include "InstrumentN163.h"		// // //
#include "Chunk.h"
#include "ChunkRenderText.h"		// // //

// // // Default wave
static const char TRIANGLE_WAVE[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};
static const int DEFAULT_WAVE_SIZE = sizeof(TRIANGLE_WAVE) / sizeof(char);

LPCTSTR CInstrumentN163::SEQUENCE_NAME[] = {_T("Volume"), _T("Arpeggio"), _T("Pitch"), _T("Hi-pitch"), _T("Wave Index")};

CInstrumentN163::CInstrumentN163() : CSeqInstrument(INST_N163),		// // //
	m_iSamples(),
	m_iWaveSize(DEFAULT_WAVE_SIZE),
	m_iWavePos(0),
	m_bAutoWavePos(false),		// // // 050B
	m_iWaveCount(1)
{
	for (int j = 0; j < DEFAULT_WAVE_SIZE; ++j)
		m_iSamples[0][j] = TRIANGLE_WAVE[j];
}

CInstrument *CInstrumentN163::Clone() const
{
	CInstrumentN163 *inst = new CInstrumentN163();		// // //
	inst->CloneFrom(this);
	return inst;
}

void CInstrumentN163::CloneFrom(const CInstrument *pInst)
{
	CSeqInstrument::CloneFrom(pInst);
	
	if (auto pNew = dynamic_cast<const CInstrumentN163*>(pInst)) {
		SetWaveSize(pNew->GetWaveSize());
		SetWavePos(pNew->GetWavePos());
	//	SetAutoWavePos(pInst->GetAutoWavePos());
		SetWaveCount(pNew->GetWaveCount());

		for (int i = 0; i < MAX_WAVE_COUNT; ++i)
			for (int j = 0; j < MAX_WAVE_SIZE; ++j)
				SetSample(i, j, pNew->GetSample(i, j));
	}
}

void CInstrumentN163::Store(CDocumentFile *pDocFile)
{
	// Store sequences
	CSeqInstrument::Store(pDocFile);		// // //

	// Store wave
	pDocFile->WriteBlockInt(m_iWaveSize);
	pDocFile->WriteBlockInt(m_iWavePos);
	//pDocFile->WriteBlockInt(m_bAutoWavePos ? 1 : 0);
	pDocFile->WriteBlockInt(m_iWaveCount);

	for (int i = 0; i < m_iWaveCount; ++i) {
		for (int j = 0; j < m_iWaveSize; ++j) {
			pDocFile->WriteBlockChar(m_iSamples[i][j]);
		}
	}
}

bool CInstrumentN163::Load(CDocumentFile *pDocFile)
{
	if (!CSeqInstrument::Load(pDocFile)) return false;		// // //

	m_iWaveSize = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), 4, MAX_WAVE_SIZE, "N163 wave size", "%i");
	m_iWavePos = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), 0, MAX_WAVE_SIZE - 1, "N163 wave position", "%i");
	CModuleException::AssertRangeFmt<MODULE_ERROR_OFFICIAL>(m_iWavePos, 0, 0x7F, "N163 wave position", "%i");
	if (pDocFile->GetBlockVersion() >= 8) {		// // // 050B
		bool AutoPosition = pDocFile->GetBlockInt() != 0;
	}
	m_iWaveCount = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), 1, MAX_WAVE_COUNT, "N163 wave count", "%i");
	CModuleException::AssertRangeFmt<MODULE_ERROR_OFFICIAL>(m_iWaveCount, 1, 0x10, "N163 wave count", "%i");
	
	for (int i = 0; i < m_iWaveCount; ++i) {
		for (int j = 0; j < m_iWaveSize; ++j) try {
			m_iSamples[i][j] = CModuleException::AssertRangeFmt(pDocFile->GetBlockChar(), 0, 15, "N163 wave sample", "%i");
		}
		catch (CModuleException *e) {
			e->AppendError("At wave %i, sample %i,", i, j);
			throw;
		}
	}
	
	return true;
}

void CInstrumentN163::SaveFile(CInstrumentFile *pFile)
{
	// Sequences
	CSeqInstrument::SaveFile(pFile);		// // //

	// Write wave config
	int WaveCount = GetWaveCount();
	int WaveSize = GetWaveSize();

	pFile->WriteInt(WaveSize);
	pFile->WriteInt(GetWavePos());
	pFile->WriteInt(m_bAutoWavePos);		// // // 050B
	pFile->WriteInt(WaveCount);

	for (int i = 0; i < WaveCount; ++i) {
		for (int j = 0; j < WaveSize; ++j) {
			pFile->WriteChar(GetSample(i, j));
		}
	}
}

bool CInstrumentN163::LoadFile(CInstrumentFile *pFile, int iVersion)
{
	// Sequences
	CSeqInstrument::LoadFile(pFile, iVersion);		// // //

	// Read wave config
	int WaveSize = CModuleException::AssertRangeFmt(static_cast<int>(pFile->ReadInt()), 4, MAX_WAVE_SIZE, "N163 wave size", "%i");
	int WavePos = CModuleException::AssertRangeFmt(static_cast<int>(pFile->ReadInt()), 0, MAX_WAVE_SIZE - 1, "N163 wave position", "%i");
	if (iVersion >= 0x250) {		// // // 050B
		m_bAutoWavePos = pFile->ReadInt() != 0;
	}
	int WaveCount = CModuleException::AssertRangeFmt(static_cast<int>(pFile->ReadInt()), 1, MAX_WAVE_COUNT, "N163 wave count", "%i");
	
	SetWaveSize(WaveSize);
	SetWavePos(WavePos);
	SetWaveCount(WaveCount);

	for (int i = 0; i < WaveCount; ++i)
		for (int j = 0; j < WaveSize; ++j) try {
			SetSample(i, j, CModuleException::AssertRangeFmt(pFile->ReadChar(), 0U, 15U, "N163 wave sample", "%u"));
		}
	catch (CModuleException *e) {
		e->AppendError("At wave %i, sample %i,", i, j);
		throw;
	}

	return true;
}

int CInstrumentN163::Compile(CChunk *pChunk, int Index)
{
	int StoredBytes = CSeqInstrument::Compile(pChunk, Index);		// // //;

	// Store wave info
	pChunk->StoreByte(m_iWaveSize >> 1);
	pChunk->StoreByte(/*m_bAutoWavePos ? 0xFF :*/ m_iWavePos);
	StoredBytes += 2;

	// Store reference to wave
	CStringA waveLabel;
	waveLabel.Format(CChunkRenderText::LABEL_WAVES, Index);
	pChunk->StoreReference(waveLabel);
	StoredBytes += 2;
	
	return StoredBytes;
}

int CInstrumentN163::StoreWave(CChunk *pChunk) const
{
	// Number of waves
//	pChunk->StoreByte(m_iWaveCount);

	// Pack samples
	for (int i = 0; i < m_iWaveCount; ++i) {
		for (int j = 0; j < m_iWaveSize; j += 2) {
			pChunk->StoreByte((m_iSamples[i][j + 1] << 4) | m_iSamples[i][j]);
		}
	}

	return m_iWaveCount * (m_iWaveSize >> 1);
}

bool CInstrumentN163::IsWaveEqual(CInstrumentN163 *pInstrument)
{
	int Count = GetWaveCount();
	int Size = GetWaveSize();

	if (pInstrument->GetWaveCount() != Count)
		return false;

	if (pInstrument->GetWaveSize() != Size)
		return false;

	for (int i = 0; i < Count; ++i) {
		for (int j = 0; j < Size; ++j) {
			if (GetSample(i, j) != pInstrument->GetSample(i, j))
				return false;
		}
	}

	return true;
}

bool CInstrumentN163::InsertNewWave(int Index)		// // //
{
	if (m_iWaveCount >= CInstrumentN163::MAX_WAVE_COUNT)
		return false;
	if (Index < 0 || Index > m_iWaveCount || Index >= CInstrumentN163::MAX_WAVE_COUNT)
		return false;

	memmove(m_iSamples[Index + 1], m_iSamples[Index], CInstrumentN163::MAX_WAVE_SIZE * (m_iWaveCount - Index) * sizeof(int));
	memset(m_iSamples[Index], 0, CInstrumentN163::MAX_WAVE_SIZE * sizeof(int));
	m_iWaveCount++;
	InstrumentChanged();
	return true;
}

bool CInstrumentN163::RemoveWave(int Index)		// // //
{
	if (m_iWaveCount <= 1)
		return false;
	if (Index < 0 || Index >= m_iWaveCount)
		return false;
	
	memmove(m_iSamples[Index], m_iSamples[Index + 1], CInstrumentN163::MAX_WAVE_SIZE * (m_iWaveCount - Index - 1) * sizeof(int));
	m_iWaveCount--;
	InstrumentChanged();
	return true;
}

int CInstrumentN163::GetWaveSize() const
{
	return m_iWaveSize;
}

void CInstrumentN163::SetWaveSize(int size)
{
	m_iWaveSize = size;
	InstrumentChanged();
}

int CInstrumentN163::GetSample(int wave, int pos) const
{
	return m_iSamples[wave][pos];
}

void CInstrumentN163::SetSample(int wave, int pos, int sample)
{
	m_iSamples[wave][pos] = sample;
	InstrumentChanged();
}

int CInstrumentN163::GetWavePos() const
{
	return m_iWavePos;
}

void CInstrumentN163::SetWavePos(int pos)
{
	m_iWavePos = MAX_WAVE_SIZE - m_iWaveSize;		// // // prevent reading non-wave n163 registers
	if (pos < m_iWavePos) m_iWavePos = pos;
	InstrumentChanged();
}
/*
void CInstrumentN163::SetAutoWavePos(bool Enable)
{
	m_bAutoWavePos = Enable;
}

bool CInstrumentN106::GetAutoWavePos() const
{
	return m_bAutoWavePos;
}
*/
void CInstrumentN163::SetWaveCount(int count)
{
	ASSERT(count <= MAX_WAVE_COUNT);
	if (m_iWaveCount != count)			// // //
		InstrumentChanged();
	m_iWaveCount = count;
}

int CInstrumentN163::GetWaveCount() const
{
	return m_iWaveCount;
}
