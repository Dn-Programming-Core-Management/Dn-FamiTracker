/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/

#include "stdafx.h"
#include "ModuleException.h"		// // //
#include "InstrumentManagerInterface.h"		// // //
#include "Instrument.h"
#include "SeqInstrument.h"
#include "Instrument2A03.h"
#include "DSample.h"		// // //
#include "DocumentFile.h"

// 2A03 instruments

LPCTSTR CInstrument2A03::SEQUENCE_NAME[] = {_T("Volume"), _T("Arpeggio"), _T("Pitch"), _T("Hi-pitch"), _T("Duty / Noise")};

CInstrument2A03::CInstrument2A03() : CSeqInstrument(INST_2A03),		// // //
	m_cSamples(),
	m_cSamplePitch(),
	m_cSampleLoopOffset()
{
	for (int i = 0; i < OCTAVE_RANGE; ++i)
		for (int j = 0; j < NOTE_RANGE; ++j)
			m_cSampleDelta[i][j] = -1;
}

CInstrument *CInstrument2A03::Clone() const
{
	CInstrument2A03 *inst = new CInstrument2A03();		// // //
	inst->CloneFrom(this);
	return inst;
}

void CInstrument2A03::CloneFrom(const CInstrument *pInst)
{
	CSeqInstrument::CloneFrom(pInst);

	if (auto pNew = dynamic_cast<const CInstrument2A03*>(pInst)) {
		for (int i = 0; i < OCTAVE_RANGE; ++i) {
			for (int j = 0; j < NOTE_RANGE; ++j) {
				SetSampleIndex(i, j, pNew->GetSampleIndex(i, j));
				SetSamplePitch(i, j, pNew->GetSamplePitch(i, j));
				SetSampleDeltaValue(i, j, pNew->GetSampleDeltaValue(i, j));
			}
		}
	}
}

void CInstrument2A03::Store(CDocumentFile *pDocFile)
{
	CSeqInstrument::Store(pDocFile);		// // //

	int Version = pDocFile->GetBlockVersion();
	int Octaves = Version >= 2 ? OCTAVE_RANGE : 6;

	if (Version >= 7)		// // // 050B
		pDocFile->WriteBlockInt(GetSampleCount());
	for (int i = 0; i < Octaves; ++i) {
		for (int j = 0; j < NOTE_RANGE; ++j) {
			if (Version >= 7) {		// // // 050B
				if (!GetSampleIndex(i, j)) continue;
				pDocFile->WriteBlockChar(MIDI_NOTE(i, j)+1);
			}
			pDocFile->WriteBlockChar(GetSampleIndex(i, j));
			pDocFile->WriteBlockChar(GetSamplePitch(i, j));
			if (Version >= 6)
				pDocFile->WriteBlockChar(GetSampleDeltaValue(i, j));
		}
	}
}

bool CInstrument2A03::Load(CDocumentFile *pDocFile)
{
	if (!CSeqInstrument::Load(pDocFile)) return false;		// // //
	
	const int Version = pDocFile->GetBlockVersion();
	const int Octaves = (Version == 1) ? 6 : OCTAVE_RANGE;

	const auto ReadAssignment = [&] (int Octave, int Note) {
		try {
			char Sample = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
				pDocFile->GetBlockChar(), 0, MAX_DSAMPLES, "DPCM sample assignment index", "%i");
			if (Sample > MAX_DSAMPLES)
				Sample = 0;

			SetSampleIndex(Octave, Note, Sample);
			char Pitch = pDocFile->GetBlockChar();
			CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(Pitch & 0x7F, 0, 0xF, "DPCM sample pitch", "%i");
			SetSamplePitch(Octave, Note, Pitch & 0x8F);
			if (Version > 5) {
				char Value = pDocFile->GetBlockChar();
				if (Value < -1) // not validated
					Value = -1;
				SetSampleDeltaValue(Octave, Note, Value);
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At note %i, octave %i,", Note + 1, Octave);
			throw;
		}
	};

	if (Version >= 7) {		// // // 050B
		const int Count = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
			pDocFile->GetBlockInt(), 0, NOTE_COUNT, "DPCM sample assignment count", "%i");
		for (int i = 0; i < Count; ++i) {
			int Note = CModuleException::AssertRangeFmt<MODULE_ERROR_STRICT>(
				pDocFile->GetBlockChar(), 0, NOTE_COUNT - 1, "DPCM sample assignment note index", "%i");
			ReadAssignment(GET_OCTAVE(Note - 1), GET_NOTE(Note - 1));
		}
	}
	else
		for (int i = 0; i < Octaves; ++i)
			for (int j = 0; j < NOTE_RANGE; ++j)
				ReadAssignment(i, j);

	return true;
}

void CInstrument2A03::SaveFile(CInstrumentFile *pFile)
{
	// Saves an 2A03 instrument
	// Current version 2.4

	// Sequences
	CSeqInstrument::SaveFile(pFile);		// // //

	// DPCM
	if (!m_pInstManager) {		// // //
		pFile->WriteInt(0);
		pFile->WriteInt(0);
		return;
	}

	unsigned int Count = GetSampleCount();		// // // 050B
	pFile->WriteInt(Count);

	bool UsedSamples[MAX_DSAMPLES];
	memset(UsedSamples, 0, sizeof(bool) * MAX_DSAMPLES);

	int UsedCount = 0;
	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		for (int j = 0; j < NOTE_RANGE; ++j) {
			if (unsigned char Sample = GetSampleIndex(i, j)) {
				unsigned char Index = MIDI_NOTE(i, j) + 1;
				pFile->WriteChar(Index);
				pFile->WriteChar(Sample);
				pFile->WriteChar(GetSamplePitch(i, j));
				pFile->WriteChar(GetSampleDeltaValue(i, j));
				if (!UsedSamples[Sample - 1])
					++UsedCount;
				UsedSamples[Sample - 1] = true;
			}
		}
	}

	// Write the number
	pFile->WriteInt(UsedCount);

	// List of sample names
	for (int i = 0; i < MAX_DSAMPLES; ++i) if (UsedSamples[i]) {
		if (const CDSample *pSample = m_pInstManager->GetDSample(i)) {
			pFile->WriteInt(i);
			const char *pName = pSample->GetName();
			std::size_t NameLen = strlen(pName);
			pFile->WriteInt((unsigned int)NameLen);
			pFile->Write(pName, static_cast<UINT>(NameLen));
			pFile->WriteInt(pSample->GetSize());
			pFile->Write(pSample->GetData(), pSample->GetSize());
		}
	}
}

bool CInstrument2A03::LoadFile(CInstrumentFile *pFile, int iVersion)
{
	if (!CSeqInstrument::LoadFile(pFile, iVersion))		// // //
		return false;

	unsigned int Count = CModuleException::AssertRangeFmt(pFile->ReadInt(), 0U, static_cast<unsigned>(NOTE_COUNT), "DPCM assignment count", "%u");

	// DPCM instruments
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned char InstNote = CModuleException::AssertRangeFmt(
			pFile->ReadChar(), 0, NOTE_COUNT - 1, "DPCM sample assignment note index", "%i");
		int Octave = GET_OCTAVE(InstNote - 1);
		int Note = GET_NOTE(InstNote - 1);
		try {
			char Sample = CModuleException::AssertRangeFmt(pFile->ReadChar(), 0, MAX_DSAMPLES, "DPCM sample assignment index", "%u");
			if (Sample > MAX_DSAMPLES)
				Sample = 0;
			SetSampleIndex(Octave, Note, Sample);

			char Pitch = pFile->ReadChar();
			CModuleException::AssertRangeFmt(Pitch & 0x7F, 0, 0xF, "DPCM sample pitch", "%i");

			SetSamplePitch(Octave, Note, Pitch);

			SetSampleDeltaValue(Octave, Note, CModuleException::AssertRangeFmt(
				static_cast<char>(iVersion >= 24 ? pFile->ReadChar() : -1), -1, 0x7F, "DPCM sample delta value", "%i"));
		}
		catch (CModuleException *e) {
			e->AppendError("At note %i, octave %i,", Note + 1, Octave);
			throw;
		}
	}

	// DPCM samples list
	bool bAssigned[OCTAVE_RANGE][NOTE_RANGE] = {};
	int TotalSize = 0;		// // // ???
	for (int i = 0; i < MAX_DSAMPLES; ++i)
		if (const CDSample *pSamp = m_pInstManager->GetDSample(i))
			TotalSize += pSamp->GetSize();

	unsigned int SampleCount = pFile->ReadInt();
	for (unsigned int i = 0; i < SampleCount; ++i) {

		int Index = CModuleException::AssertRangeFmt(
			pFile->ReadInt(), 0U, static_cast<unsigned>(MAX_DSAMPLES - 1), "DPCM sample index", "%u");

		int Len = CModuleException::AssertRangeFmt(
			pFile->ReadInt(), 0U, static_cast<unsigned>(CDSample::MAX_NAME_SIZE - 1), "DPCM sample name length", "%u");

		char SampleName[256]{};
		pFile->Read(SampleName, Len);

		int Size = pFile->ReadInt();
		char *SampleData = new char[Size];
		pFile->Read(SampleData, Size);
		bool Found = false;
		for (int j = 0; j < MAX_DSAMPLES; ++j) if (const CDSample *pSample = m_pInstManager->GetDSample(j)) {		// // //
			// Compare size and name to see if identical sample exists
			if (pSample->GetSize() == Size && !memcmp(pSample->GetData(), SampleData, Size) &&		// // //
				!strcmp(pSample->GetName(), SampleName)) {
				Found = true;
				// Assign sample
				for (int o = 0; o < OCTAVE_RANGE; ++o) {
					for (int n = 0; n < NOTE_RANGE; ++n) {
						if (GetSampleIndex(o, n) == (Index + 1) && !bAssigned[o][n]) {
							SetSampleIndex(o, n, j + 1);
							bAssigned[o][n] = true;
						}
					}
				}
				break;
			}
		}
		if (Found) {
			SAFE_RELEASE_ARRAY(SampleData);
			continue;
		}

		// Load sample
		
		if (TotalSize + Size > MAX_SAMPLE_SPACE) {
			SAFE_RELEASE_ARRAY(SampleData);
			CModuleException *e = new CModuleException();
			e->AppendError("Insufficient DPCM sample space (maximum %d KB)", MAX_SAMPLE_SPACE / 1024);
			e->Raise();
		}
		CDSample *pSample = new CDSample();		// // //
		pSample->SetName(SampleName);
		pSample->SetData(Size, SampleData);
		int FreeSample = m_pInstManager->AddDSample(pSample);		// not off-by-one
		if (FreeSample == -1) {
			SAFE_RELEASE(pSample);
			CModuleException *e = new CModuleException();
			e->AppendError("Document has no free DPCM sample slot");
			e->Raise();
		}
		TotalSize += Size;
		// Assign it
		for (int o = 0; o < OCTAVE_RANGE; ++o) {
			for (int n = 0; n < NOTE_RANGE; ++n) {
				if (GetSampleIndex(o, n) == (Index + 1) && !bAssigned[o][n]) {
					SetSampleIndex(o, n, FreeSample + 1);
					bAssigned[o][n] = true;
				}
			}
		}
	}

	return true;
}

int CInstrument2A03::GetSampleCount() const		// // // 050B
{
	int Count = 0;
	for (int i = 0; i < OCTAVE_RANGE; ++i) {	// octaves
		for (int j = 0; j < NOTE_RANGE; ++j) {	// notes
			Count += (GetSampleIndex(i, j) > 0) ? 1 : 0;
		}
	}
	return Count;
}

char CInstrument2A03::GetSampleIndex(int Octave, int Note) const
{
	return m_cSamples[Octave][Note];
}

char CInstrument2A03::GetSamplePitch(int Octave, int Note) const
{
	return m_cSamplePitch[Octave][Note];
}

bool CInstrument2A03::GetSampleLoop(int Octave, int Note) const
{
	return (m_cSamplePitch[Octave][Note] & 0x80) == 0x80;
}

char CInstrument2A03::GetSampleLoopOffset(int Octave, int Note) const
{
	return m_cSampleLoopOffset[Octave][Note];
}

char CInstrument2A03::GetSampleDeltaValue(int Octave, int Note) const
{
	return m_cSampleDelta[Octave][Note];
}

void CInstrument2A03::SetSampleIndex(int Octave, int Note, char Sample)
{
	// Sample is off by one; 0 means there is no index assigned to a note
	m_cSamples[Octave][Note] = Sample;
	InstrumentChanged();
}

void CInstrument2A03::SetSamplePitch(int Octave, int Note, char Pitch)
{
	m_cSamplePitch[Octave][Note] = Pitch;
	InstrumentChanged();
}

void CInstrument2A03::SetSampleLoop(int Octave, int Note, bool Loop)
{
	m_cSamplePitch[Octave][Note] = (m_cSamplePitch[Octave][Note] & 0x7F) | (Loop ? 0x80 : 0);
	InstrumentChanged();
}

void CInstrument2A03::SetSampleLoopOffset(int Octave, int Note, char Offset)
{
	m_cSampleLoopOffset[Octave][Note] = Offset;
	InstrumentChanged();
}

void CInstrument2A03::SetSampleDeltaValue(int Octave, int Note, char Value)
{
	m_cSampleDelta[Octave][Note] = Value;
	InstrumentChanged();
}

bool CInstrument2A03::AssignedSamples() const
{
	// Returns true if there are assigned samples in this instrument	

	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		for (int j = 0; j < NOTE_RANGE; ++j) {
			if (GetSampleIndex(i, j) != 0)
				return true;
		}
	}

	return false;
}

const CDSample *CInstrument2A03::GetDSample(int Octave, int Note) const		// // //
{
	if (!m_pInstManager) return nullptr;
	char Index = GetSampleIndex(Octave, Note);
	if (!Index) return nullptr;
	return m_pInstManager->GetDSample(Index - 1);
}
