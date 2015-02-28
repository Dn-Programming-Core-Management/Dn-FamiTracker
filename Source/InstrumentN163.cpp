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
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "DocumentFile.h"
#include "Instrument.h"
#include "Compiler.h"
#include "Chunk.h"

const int CInstrumentN163::SEQUENCE_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};

CInstrumentN163::CInstrumentN163()
{
	// Default wave
	const char TRIANGLE_WAVE[] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 
		15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
	};

	const int DEFAULT_WAVE_SIZE = 32;

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}

	memset(m_iSamples, 0, sizeof(int) * MAX_WAVE_SIZE);

	for (int i = 0; i < MAX_WAVE_COUNT; ++i) {
		for (int j = 0; j < DEFAULT_WAVE_SIZE; ++j) {
			m_iSamples[i][j] = (i == 0 && j < 32) ? TRIANGLE_WAVE[j] : 0;	// // //
		}
	}

	m_iWaveSize = DEFAULT_WAVE_SIZE;
	m_iWavePos = 0;
	m_iWaveCount = 1;
}

CInstrument *CInstrumentN163::Clone() const
{
	CInstrumentN163 *pNew = new CInstrumentN163();

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	pNew->SetWaveSize(GetWaveSize());
	pNew->SetWavePos(GetWavePos());
//	pNew->SetAutoWavePos(GetAutoWavePos());
	pNew->SetWaveCount(GetWaveCount());

	for (int i = 0; i < MAX_WAVE_COUNT; ++i) {
		for (int j = 0; j < MAX_WAVE_SIZE; ++j) {
			pNew->SetSample(i, j, GetSample(i, j));
		}
	}

	pNew->SetName(GetName());

	return pNew;
}

void CInstrumentN163::Setup()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	for (int i = 0; i < SEQ_COUNT; ++i) {
		SetSeqEnable(i, 0);
		int Index = pDoc->GetFreeSequenceN163(i);
		if (Index != -1)
			SetSeqIndex(i, Index);
	}
}

void CInstrumentN163::Store(CDocumentFile *pDocFile)
{
	// Store sequences
	pDocFile->WriteBlockInt(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}

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
	int SeqCnt = pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(SeqCnt < (SEQUENCE_COUNT + 1));

	SeqCnt = SEQUENCE_COUNT;

	for (int i = 0; i < SeqCnt; ++i) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		int Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	m_iWaveSize = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(m_iWaveSize >= 0 && m_iWaveSize <= MAX_WAVE_SIZE);
	m_iWavePos = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(m_iWavePos >= 0 && m_iWavePos < MAX_WAVE_SIZE);		// // //
//	m_bAutoWavePos = (pDocFile->GetBlockInt() == 0) ? false : true;
	//pDocFile->GetBlockInt();
	m_iWaveCount = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(m_iWaveCount >= 1 && m_iWaveCount <= MAX_WAVE_COUNT);
	
	for (int i = 0; i < m_iWaveCount; ++i) {
		for (int j = 0; j < m_iWaveSize; ++j) {
			unsigned char WaveSample = pDocFile->GetBlockChar();
			ASSERT_FILE_DATA(WaveSample < 16);
			m_iSamples[i][j] = WaveSample;
		}
	}
	
	return true;
}

void CInstrumentN163::SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc)
{
	// Sequences
	pFile->WriteChar(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		int Sequence = GetSeqIndex(i);

		if (GetSeqEnable(i)) {
			const CSequence *pSeq = pDoc->GetSequenceN163(Sequence, i);
			pFile->WriteChar(1);
			pFile->WriteInt(pSeq->GetItemCount());
			pFile->WriteInt(pSeq->GetLoopPoint());
			pFile->WriteInt(pSeq->GetReleasePoint());
			pFile->WriteInt(pSeq->GetSetting());
			for (unsigned j = 0; j < pSeq->GetItemCount(); ++j) {
				pFile->WriteChar(pSeq->GetItem(j));
			}
		}
		else {
			pFile->WriteChar(0);
		}
	}

	// Write wave config
	int WaveCount = GetWaveCount();
	int WaveSize = GetWaveSize();

	pFile->WriteInt(WaveSize);
	pFile->WriteInt(GetWavePos());
	pFile->WriteInt(WaveCount);

	for (int i = 0; i < WaveCount; ++i) {
		for (int j = 0; j < WaveSize; ++j) {
			pFile->WriteChar(GetSample(i, j));
		}
	}
}

bool CInstrumentN163::LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
{
	// Sequences
	unsigned char SeqCount = pFile->ReadChar();

	// Loop through all instrument effects
	for (unsigned int i = 0; i < SeqCount; ++i) {

		unsigned char Enabled = pFile->ReadChar();
		if (Enabled == 1) {
			// Read the sequence
			int Count = pFile->ReadInt();
			if (Count < 0 || Count > MAX_SEQUENCE_ITEMS)
				return false;

			// Find a free sequence
			int Index = pDoc->GetFreeSequenceN163(i);
			if (Index != -1) {
				CSequence *pSeq = pDoc->GetSequenceN163(Index, i);
	
				pSeq->SetItemCount(Count);
				pSeq->SetLoopPoint(pFile->ReadInt());
				pSeq->SetReleasePoint(pFile->ReadInt());
				pSeq->SetSetting(static_cast<seq_setting_t>(pFile->ReadInt()));		// // //
				for (int j = 0; j < Count; ++j) {
					pSeq->SetItem(j, pFile->ReadChar());
				}
				SetSeqEnable(i, true);
				SetSeqIndex(i, Index);
			}
		}
		else {
			SetSeqEnable(i, false);
			SetSeqIndex(i, 0);
		}
	}

	// Read wave config
	int WaveSize = pFile->ReadInt();
	int WavePos = pFile->ReadInt();
	int WaveCount = pFile->ReadInt();
	
	if (WaveSize <= 0 || WaveSize > MAX_WAVE_SIZE)			// // //
		return false;
	if (WaveCount <= 0 || WaveCount > MAX_WAVE_COUNT)
		return false;

	SetWaveSize(WaveSize);
	SetWavePos(WavePos);
	SetWaveCount(WaveCount);

	for (int i = 0; i < WaveCount; ++i) {
		for (int j = 0; j < WaveSize; ++j) {
			SetSample(i, j, pFile->ReadChar());
		}
	}

	return true;
}

int CInstrumentN163::Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index)
{
	int ModSwitch = 0;
	int StoredBytes = 0;
	// // //
	ASSERT(pDoc != NULL);

	// Store wave info
	pChunk->StoreByte(m_iWaveSize >> 1);
	pChunk->StoreByte(/*m_bAutoWavePos ? 0xFF :*/ m_iWavePos);
	StoredBytes += 2;

	// Store reference to wave
	CStringA waveLabel;
	waveLabel.Format(CCompiler::LABEL_WAVES, Index);
	pChunk->StoreReference(waveLabel);
	StoredBytes += 2;

	// Store sequences
	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		ModSwitch = (ModSwitch >> 1) | (GetSeqEnable(i) && (pDoc->GetSequence(SNDCHIP_N163, GetSeqIndex(i), i)->GetItemCount() > 0) ? 0x10 : 0);
	}

	pChunk->StoreByte(ModSwitch);
	StoredBytes++;

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		if (GetSeqEnable(i) != 0 && (pDoc->GetSequence(SNDCHIP_N163, GetSeqIndex(i), i)->GetItemCount() != 0)) {
			CStringA str;
			str.Format(CCompiler::LABEL_SEQ_N163, GetSeqIndex(i) * SEQUENCE_COUNT + i);
			pChunk->StoreReference(str);
			StoredBytes += 2;
		}
	}
	
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

bool CInstrumentN163::CanRelease() const
{
	if (GetSeqEnable(0) != 0) {
		int index = GetSeqIndex(SEQ_VOLUME);
		return CFamiTrackerDoc::GetDoc()->GetSequence(SNDCHIP_N163, index, SEQ_VOLUME)->GetReleasePoint() != -1;
	}

	return false;
}

int	CInstrumentN163::GetSeqEnable(int Index) const
{
	ASSERT(Index < SEQ_COUNT);
	return m_iSeqEnable[Index];
}

int	CInstrumentN163::GetSeqIndex(int Index) const
{
	ASSERT(Index < SEQ_COUNT);
	return m_iSeqIndex[Index];
}

void CInstrumentN163::SetSeqEnable(int Index, int Value)
{
	ASSERT(Index < SEQ_COUNT);
	if (m_iSeqEnable[Index] != Value)
		InstrumentChanged();
	m_iSeqEnable[Index] = Value;
}

void CInstrumentN163::SetSeqIndex(int Index, int Value)
{
	ASSERT(Index < SEQ_COUNT);
	if (m_iSeqIndex[Index] != Value)
		InstrumentChanged();
	m_iSeqIndex[Index] = Value;
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
	ASSERT(wave < MAX_WAVE_COUNT);
	ASSERT(pos < MAX_WAVE_SIZE);

	return m_iSamples[wave][pos];
}

void CInstrumentN163::SetSample(int wave, int pos, int sample)
{
	ASSERT(wave < MAX_WAVE_COUNT);
	ASSERT(pos < MAX_WAVE_SIZE);

	m_iSamples[wave][pos] = sample;
	InstrumentChanged();
}

int CInstrumentN163::GetWavePos() const
{
	return m_iWavePos;
}

void CInstrumentN163::SetWavePos(int pos)
{
	m_iWavePos = std::min(pos, MAX_WAVE_SIZE - m_iWaveSize);		// // // prevent reading non-wave n163 registers
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
