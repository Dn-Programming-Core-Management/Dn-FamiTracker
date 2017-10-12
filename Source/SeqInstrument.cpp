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

#include "SeqInstrument.h"
#include "ModuleException.h"
#include "DocumentFile.h"
#include "InstrumentManagerInterface.h"
#include "Sequence.h"
#include "OldSequence.h"		// // //
#include "Chunk.h"
#include "ChunkRenderText.h"
#include "SimpleFile.h"

/*
 * Base class for instruments using sequences
 */

CSeqInstrument::CSeqInstrument(inst_type_t type) : CInstrument(type),
	m_iSeqEnable(),
	m_iSeqIndex()
{
}

CInstrument *CSeqInstrument::Clone() const
{
	CSeqInstrument *inst = new CSeqInstrument(m_iType);		// // //
	inst->CloneFrom(this);
	return inst;
}

void CSeqInstrument::CloneFrom(const CInstrument *pInst)
{
	CInstrument::CloneFrom(pInst);
	
	if (auto pNew = dynamic_cast<const CSeqInstrument*>(pInst))
		for (int i = 0; i < SEQ_COUNT; i++) {
			SetSeqEnable(i, pNew->GetSeqEnable(i));
			SetSeqIndex(i, pNew->GetSeqIndex(i));
		}
}

void CSeqInstrument::OnRegisterManager()		// // //
{
	for (int i = 0; i < SEQ_COUNT; ++i) {
		SetSeqEnable(i, 0);
		int Index = m_pInstManager->AddSequence(m_iType, i, nullptr, this);
		if (Index != -1)
			SetSeqIndex(i, Index);
	}
}

void CSeqInstrument::Store(CDocumentFile *pDocFile) const
{
	pDocFile->WriteBlockInt(SEQ_COUNT);

	for (int i = 0; i < SEQ_COUNT; i++) {
		pDocFile->WriteBlockChar(GetSeqEnable(i));
		pDocFile->WriteBlockChar(GetSeqIndex(i));
	}
}

bool CSeqInstrument::Load(CDocumentFile *pDocFile)
{
	int SeqCnt = CModuleException::AssertRangeFmt(pDocFile->GetBlockInt(), 0, SEQ_COUNT, "Instrument sequence count");
	SeqCnt = SEQ_COUNT;

	for (int i = 0; i < SeqCnt; ++i) {
		int Enable = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
			pDocFile->GetBlockChar(), 0, 1, "Instrument sequence enable flag");
		SetSeqEnable(i, Enable != 0);
		int Index = static_cast<unsigned char>(pDocFile->GetBlockChar());		// // //
		SetSeqIndex(i, CModuleException::AssertRangeFmt(Index, 0, MAX_SEQUENCES - 1, "Instrument sequence index"));
	}

	return true;
}

void CSeqInstrument::SaveFile(CSimpleFile *pFile) const
{
	pFile->WriteChar(SEQ_COUNT);

	for (int i = 0; i < SEQ_COUNT; ++i) {
		unsigned Sequence = GetSeqIndex(i);
		if (GetSeqEnable(i)) {
			const CSequence *pSeq = GetSequence(i);
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

bool CSeqInstrument::LoadFile(CSimpleFile *pFile, int iVersion)
{
	// Sequences
	CSequence *pSeq;

	unsigned char SeqCount = CModuleException::AssertRangeFmt(pFile->ReadChar(), 0, SEQ_COUNT, "Sequence count");

	// Loop through all instrument effects
	for (unsigned i = 0; i < SeqCount; ++i) try {
		if (pFile->ReadChar() != 1) {
			SetSeqEnable(i, false);
			SetSeqIndex(i, 0);
			continue;
		}
		SetSeqEnable(i, true);

		// Read the sequence
		int Count = CModuleException::AssertRangeFmt(pFile->ReadInt(), 0, 0xFF, "Sequence item count");

		if (iVersion < 20) {
			COldSequence OldSeq;
			for (int j = 0; j < Count; ++j) {
				char Length = pFile->ReadChar();
				OldSeq.AddItem(Length, pFile->ReadChar());
			}
			pSeq = OldSeq.Convert(i).release();
		}
		else {
			pSeq = new CSequence();
			int Count2 = Count > MAX_SEQUENCE_ITEMS ? MAX_SEQUENCE_ITEMS : Count;
			pSeq->SetItemCount(Count2);
			pSeq->SetLoopPoint(CModuleException::AssertRangeFmt(
				static_cast<int>(pFile->ReadInt()), -1, Count2 - 1, "Sequence loop point"));
			if (iVersion > 20) {
				pSeq->SetReleasePoint(CModuleException::AssertRangeFmt(
					static_cast<int>(pFile->ReadInt()), -1, Count2 - 1, "Sequence release point"));
				if (iVersion >= 22)
					pSeq->SetSetting(static_cast<seq_setting_t>(pFile->ReadInt()));
			}
			for (int j = 0; j < Count; ++j)
				if (j < Count2) pSeq->SetItem(j, pFile->ReadChar());
		}
		int Index = m_pInstManager->AddSequence(m_iType, i, pSeq, this);
		if (Index == -1) {
			for (unsigned int j = 0; j < i; ++j)
				SetSequence(j, nullptr);
			CModuleException *e = new CModuleException();
			e->AppendError("Document has no free sequence slot");
			e->Raise();
		}
		SetSeqIndex(i, Index);
	}
	catch (CModuleException *e) {
		e->AppendError("At %s sequence,", GetSequenceName(i));
		if (pSeq) delete pSeq;
		throw;
	}

	return true;
}

int CSeqInstrument::Compile(CChunk *pChunk, int Index) const
{
	int StoredBytes = 0;

	switch (GetType()) {		// // //
	case INST_2A03: pChunk->StoreByte(0);  break;
	case INST_VRC6: pChunk->StoreByte(4);  break;
	case INST_N163: pChunk->StoreByte(9);  break;
	case INST_S5B:  pChunk->StoreByte(10); break;
	}

	int ModSwitch = 0;
	for (unsigned i = 0; i < SEQ_COUNT; ++i) {
		const CSequence *pSequence = GetSequence(i);
		if (GetSeqEnable(i) && pSequence != nullptr && pSequence->GetItemCount() > 0)
			ModSwitch |= 1 << i;
	}
	pChunk->StoreByte(ModSwitch);
	StoredBytes += 2;
	
	for (unsigned i = 0; i < SEQ_COUNT; ++i) {
		if (ModSwitch & (1 << i)) {
			pChunk->StorePointer({CHUNK_SEQUENCE, GetSeqIndex(i) * SEQ_COUNT + i, (unsigned)GetType()});		// // //
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

CSequence *CSeqInstrument::GetSequence(int SeqType) const		// // //
{
	return m_pInstManager->GetSequence(m_iType, SeqType, m_iSeqIndex[SeqType]);
}

void CSeqInstrument::SetSequence(int SeqType, CSequence *pSeq)		// // //
{
	m_pInstManager->SetSequence(m_iType, SeqType, m_iSeqIndex[SeqType], pSeq);
}

bool CSeqInstrument::CanRelease() const
{
	return GetSeqEnable(SEQ_VOLUME) != 0 && GetSequence(SEQ_VOLUME)->GetReleasePoint() != -1;
}

void CSeqInstrument::SetSeqIndex(int Index, int Value)
{
	ASSERT(Index < SEQ_COUNT);
	if (m_iSeqIndex[Index] != Value)
		InstrumentChanged();
	m_iSeqIndex[Index] = Value;
}
