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
#include "DocumentFile.h"
#include "Compiler.h"
#include "Chunk.h"

/*
 * Class CInstrument, base class for instruments
 *
 */

CInstrument::CInstrument(inst_type_t type) : CRefCounter(), m_iType(type)		// // //
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

inst_type_t CInstrument::GetType() const		// // //
{
	return m_iType;
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

// // //
/*
 * Base class for instruments using sequences
 */

CSeqInstrument::CSeqInstrument(inst_type_t type) : CInstrument(type)
{
	for (int i = 0; i < SEQ_COUNT; ++i) {
		m_iSeqEnable[i] = 0;
		m_iSeqIndex[i] = 0;
	}
}

CInstrument *CSeqInstrument::Clone() const
{
	CSeqInstrument *inst = static_cast<CSeqInstrument*>(CreateNew());		// // //

	for (int i = 0; i < SEQ_COUNT; i++) {
		inst->SetSeqEnable(i, GetSeqEnable(i));
		inst->SetSeqIndex(i, GetSeqIndex(i));
	}

	inst->SetName(GetName());

	return inst;
}

void CSeqInstrument::Setup()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	for (int i = 0; i < SEQ_COUNT; ++i) {
		SetSeqEnable(i, 0);
		int Index = pDoc->GetFreeSequence(m_iType, i);
		if (Index != -1)
			SetSeqIndex(i, Index);
	}
}

void CSeqInstrument::Store(CDocumentFile *pDocFile)
{
	pDocFile->WriteBlockInt(SEQ_COUNT);

	for (int i = 0; i < SEQ_COUNT; i++) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}
}

bool CSeqInstrument::Load(CDocumentFile *pDocFile)
{
	int SeqCnt = pDocFile->GetBlockInt();
	ASSERT_FILE_DATA(SeqCnt < (SEQ_COUNT + 1));
	SeqCnt = SEQ_COUNT;

	for (int i = 0; i < SeqCnt; i++) {
		SetSeqEnable(i, pDocFile->GetBlockChar());
		int Index = pDocFile->GetBlockChar();
		ASSERT_FILE_DATA(Index < MAX_SEQUENCES);
		SetSeqIndex(i, Index);
	}

	return true;
}

void CSeqInstrument::SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc)
{
	pFile->WriteChar(SEQ_COUNT);

	for (int i = 0; i < SEQ_COUNT; ++i) {
		unsigned Sequence = GetSeqIndex(i);
		if (GetSeqEnable(i)) {
			const CSequence *pSeq = pDoc->GetSequence(m_iType, Sequence, i);
			pFile->WriteChar(1);
			pFile->WriteInt(pSeq->GetItemCount());
			pFile->WriteInt(pSeq->GetLoopPoint());
			pFile->WriteInt(pSeq->GetReleasePoint());
			pFile->WriteInt(pSeq->GetSetting());
			for (unsigned j = 0; j < pSeq->GetItemCount(); j++) {
				pFile->WriteChar(pSeq->GetItem(j));
			}
		}
		else {
			pFile->WriteChar(0);
		}
	}
}

bool CSeqInstrument::LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
{
	// Sequences
	stSequence OldSequence;

	unsigned char SeqCount = pFile->ReadChar();

	// Loop through all instrument effects
	for (unsigned i = 0; i < SeqCount; ++i) {
		unsigned char Enabled = pFile->ReadChar();
		if (Enabled == 1) {
			// Read the sequence
			int Count = pFile->ReadInt();
			if (Count < 0 || Count > MAX_SEQUENCE_ITEMS)
				return false;
			
			// Find a free sequence
			int Index = pDoc->GetFreeSequence(m_iType, i);

			if (Index != -1) {
				CSequence *pSeq = pDoc->GetSequence(m_iType, static_cast<unsigned>(Index), i);

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
					if (iVersion > 20)
						pSeq->SetReleasePoint(pFile->ReadInt());
					if (iVersion >= 22)
						pSeq->SetSetting(static_cast<seq_setting_t>(pFile->ReadInt()));
					for (int j = 0; j < Count; ++j)
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

	return true;
}

int CSeqInstrument::Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index)
{
	int StoredBytes = 0;

	const char *label = NULL;		// // //
	switch (GetType()) {
	case INST_2A03: pChunk->StoreByte(0);  label = CCompiler::LABEL_SEQ_2A03; break;
	case INST_VRC6: pChunk->StoreByte(4);  label = CCompiler::LABEL_SEQ_VRC6; break;
	case INST_N163: pChunk->StoreByte(9);  label = CCompiler::LABEL_SEQ_N163; break;
	case INST_S5B:  pChunk->StoreByte(10); label = CCompiler::LABEL_SEQ_S5B;  break;
	}
	ASSERT(label != NULL);

	int ModSwitch = 0;
	for (unsigned i = 0; i < SEQ_COUNT; ++i) {
		const CSequence *pSequence = pDoc->GetSequence(m_iType, unsigned(GetSeqIndex(i)), i);
		ModSwitch = ModSwitch | (GetSeqEnable(i) && pSequence != NULL && pSequence->GetItemCount() > 0 ? (1 << i) : 0);
	}
	pChunk->StoreByte(ModSwitch);
	StoredBytes += 2;
	
	for (unsigned i = 0; i < SEQ_COUNT; ++i) {
		if (ModSwitch & (1 << i)) {
			CStringA str;
			str.Format(label, GetSeqIndex(i) * SEQ_COUNT + i);
			pChunk->StoreReference(str);
			StoredBytes += 2;
		}
	}
	
	return StoredBytes;
}

int	CSeqInstrument::GetSeqEnable(int Index) const
{
	ASSERT(Index < SEQ_COUNT);
	return m_iSeqEnable[Index];
}

int	CSeqInstrument::GetSeqIndex(int Index) const
{
	ASSERT(Index < SEQ_COUNT);
	return m_iSeqIndex[Index];
}

void CSeqInstrument::SetSeqEnable(int Index, int Value)
{
	ASSERT(Index < SEQ_COUNT);
	if (m_iSeqEnable[Index] != Value)
		InstrumentChanged();
	m_iSeqEnable[Index] = Value;
}

bool CSeqInstrument::CanRelease() const
{
	return GetSeqEnable(SEQ_VOLUME) != 0
		&& CFamiTrackerDoc::GetDoc()->GetSequence(m_iType, GetSeqIndex(SEQ_VOLUME), SEQ_VOLUME)->GetReleasePoint() != -1;
}

void CSeqInstrument::SetSeqIndex(int Index, int Value)
{
	ASSERT(Index < SEQ_COUNT);
	if (m_iSeqIndex[Index] != Value)
		InstrumentChanged();
	m_iSeqIndex[Index] = Value;
}
