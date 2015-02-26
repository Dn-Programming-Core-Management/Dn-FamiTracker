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

#include <map>
#include <vector>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"
#include "Compiler.h"
#include "Chunk.h"
#include "DocumentFile.h"

/*
 * class CInstrumentVRC6
 *
 */

const int CInstrumentVRC6::SEQUENCE_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};

CInstrumentVRC6::CInstrumentVRC6()
{
	for (int i = 0; i < SEQUENCE_COUNT; i++) {
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}	
}

CInstrument *CInstrumentVRC6::Clone() const
{
	CInstrumentVRC6 *pNew = new CInstrumentVRC6();

	for (int i = 0; i < SEQUENCE_COUNT; i++) {
		pNew->SetSeqEnable(i, GetSeqEnable(i));
		pNew->SetSeqIndex(i, GetSeqIndex(i));
	}

	pNew->SetName(GetName());

	return pNew;
}

void CInstrumentVRC6::Setup()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	for (int i = 0; i < SEQ_COUNT; ++i) {
		SetSeqEnable(i, 0);
		int Index = pDoc->GetFreeSequenceVRC6(i);
		if (Index != -1)
			SetSeqIndex(i, Index);
	}
}

void CInstrumentVRC6::Store(CDocumentFile *pDocFile)
{
	pDocFile->WriteBlockInt(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; i++) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}
}

bool CInstrumentVRC6::Load(CDocumentFile *pDocFile)
{
	int SeqCnt = pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(SeqCnt < (SEQUENCE_COUNT + 1));

	SeqCnt = SEQUENCE_COUNT;//SEQ_COUNT;

	for (int i = 0; i < SeqCnt; i++) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		int Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	return true;
}

void CInstrumentVRC6::SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc)
{
	// Sequences
	pFile->WriteChar(SEQUENCE_COUNT);

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		int Sequence = GetSeqIndex(i);
		if (GetSeqEnable(i)) {
			CSequence *pSeq = pDoc->GetSequenceVRC6(Sequence, i);

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
}

bool CInstrumentVRC6::LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
{
	// Sequences
	stSequence OldSequence;

	unsigned char SeqCount = pFile->ReadChar();

	// Loop through all instrument effects
	for (int i = 0; i < SeqCount; ++i) {
		unsigned char Enabled = pFile->ReadChar();
		if (Enabled == 1) {
			// Read the sequence
			int Count = pFile->ReadInt();
			int Index = pDoc->GetFreeSequenceVRC6(i);
			if (Index != -1) {
				CSequence *pSeq = pDoc->GetSequence(SNDCHIP_VRC6, Index, i);

				if (iVersion < 20) {
					OldSequence.Count = Count;
					for (int j = 0; j < Count; ++j) {
						OldSequence.Length[j] = pFile->ReadChar();
						OldSequence.Value[j] = pFile->ReadChar();
					}
					CFamiTrackerDoc::ConvertSequence(&OldSequence, pSeq, i);	// convert
				}
				else {
					pSeq->SetItemCount(Count);
					pSeq->SetLoopPoint(pFile->ReadInt());
					if (iVersion > 20) {
						pSeq->SetReleasePoint(pFile->ReadInt());
					}
					if (iVersion >= 22) {
						pSeq->SetSetting(pFile->ReadInt());
					}
					for (int j = 0; j < Count; ++j) {
						pSeq->SetItem(j, pFile->ReadChar());
					}
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

	return true;
}

int CInstrumentVRC6::Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index)
{
	int ModSwitch = 0;
	int StoredBytes = 0;

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		ModSwitch = (ModSwitch >> 1) | (GetSeqEnable(i) && (pDoc->GetSequence(SNDCHIP_VRC6, GetSeqIndex(i), i)->GetItemCount() > 0) ? 0x10 : 0);
	}

	pChunk->StoreByte(ModSwitch);
	StoredBytes++;

	for (int i = 0; i < SEQUENCE_COUNT; ++i) {
		if (GetSeqEnable(i) != 0 && (pDoc->GetSequence(SNDCHIP_VRC6, GetSeqIndex(i), i)->GetItemCount() != 0)) {
			CStringA str;
			str.Format(CCompiler::LABEL_SEQ_VRC6, GetSeqIndex(i) * SEQUENCE_COUNT + i);
			pChunk->StoreReference(str);
			StoredBytes += 2;
		}
	}
	
	return StoredBytes;
}

bool CInstrumentVRC6::CanRelease() const
{
	if (GetSeqEnable(0) != 0) {
		int index = GetSeqIndex(SEQ_VOLUME);
		return CFamiTrackerDoc::GetDoc()->GetSequence(SNDCHIP_VRC6, index, SEQ_VOLUME)->GetReleasePoint() != -1;
	}

	return false;
}

int	CInstrumentVRC6::GetSeqEnable(int Index) const
{
	return m_iSeqEnable[Index];
}

int	CInstrumentVRC6::GetSeqIndex(int Index) const
{
	return m_iSeqIndex[Index];
}

void CInstrumentVRC6::SetSeqEnable(int Index, int Value)
{
	if (m_iSeqEnable[Index] != Value)
		InstrumentChanged();		
	m_iSeqEnable[Index] = Value;
}

void CInstrumentVRC6::SetSeqIndex(int Index, int Value)
{
	if (m_iSeqIndex[Index] != Value)
		InstrumentChanged();
	m_iSeqIndex[Index] = Value;
}
