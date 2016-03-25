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
#include "InstrumentManagerInterface.h"		// // //
#include "Instrument.h"
#include "DocumentFile.h"

// 2A03 instruments

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
	CInstrument2A03 *pNew = static_cast<CInstrument2A03*>(CSeqInstrument::Clone());		// // //

	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		for (int j = 0; j < NOTE_RANGE; ++j) {
			pNew->SetSample(i, j, GetSample(i, j));
			pNew->SetSamplePitch(i, j, GetSamplePitch(i, j));
		}
	}

	return pNew;
}

void CInstrument2A03::Store(CDocumentFile *pDocFile)
{
	CSeqInstrument::Store(pDocFile);		// // //

	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		for (int j = 0; j < NOTE_RANGE; ++j) {
			pDocFile->WriteBlockChar(GetSample(i, j));
			pDocFile->WriteBlockChar(GetSamplePitch(i, j));
			pDocFile->WriteBlockChar(GetSampleDeltaValue(i, j));
		}
	}
}

bool CInstrument2A03::Load(CDocumentFile *pDocFile)
{
	if (!CSeqInstrument::Load(pDocFile)) return false;		// // //
	
	const int Version = pDocFile->GetBlockVersion();
	const int Octaves = (Version == 1) ? 6 : OCTAVE_RANGE;

	for (int i = 0; i < Octaves; ++i) {
		for (int j = 0; j < NOTE_RANGE; ++j) try {
			int Index = CModuleException::AssertRangeFmt(pDocFile->GetBlockChar(), 0, 0x7F, "DPCM sample assignment index", "%i");
			if (Index > MAX_DSAMPLES)
				Index = 0;
			SetSample(i, j, Index);
			SetSamplePitch(i, j, CModuleException::AssertRangeFmt(pDocFile->GetBlockChar(), 0, 0xF, "DPCM sample pitch", "%i"));
			if (Version > 5) {
				char Value = pDocFile->GetBlockChar();
				if (Value < -1) // not validated
					Value = -1;
				SetSampleDeltaValue(i, j, Value);
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At note %i, octave %i,", j + 1, i);
			throw;
		}
	}

	return true;
}

void CInstrument2A03::SaveFile(CInstrumentFile *pFile)
{
	// Saves an 2A03 instrument
	// Current version 2.4

	// Sequences
	CSeqInstrument::SaveFile(pFile);		// // //

	unsigned int Count = 0;

	// Count assigned keys
	for (int i = 0; i < OCTAVE_RANGE; ++i) {	// octaves
		for (int j = 0; j < NOTE_RANGE; ++j) {	// notes
			Count += (GetSample(i, j) > 0) ? 1 : 0;
		}
	}
	pFile->WriteInt(Count);

	bool UsedSamples[MAX_DSAMPLES];
	memset(UsedSamples, 0, sizeof(bool) * MAX_DSAMPLES);

	// DPCM
	if (!m_pInstManager) {		// // //
		pFile->WriteInt(0);
		return;
	}

	int SampleCount = 0;
	for (int i = 0; i < OCTAVE_RANGE; ++i) {	// octaves
		for (int j = 0; j < NOTE_RANGE; ++j) {	// notes
			if (unsigned char Sample = GetSample(i, j)) {
				unsigned char Index = i * NOTE_RANGE + j;
				pFile->WriteChar(Index);
				pFile->WriteChar(Sample);
				pFile->WriteChar(GetSamplePitch(i, j));
				pFile->WriteChar(GetSampleDeltaValue(i, j));
				if (!UsedSamples[Sample - 1])
					++SampleCount;
				UsedSamples[Sample - 1] = true;
			}
		}
	}

	// Write the number
	pFile->WriteInt(SampleCount);

	// List of sample names
	for (int i = 0; i < MAX_DSAMPLES; ++i) if (UsedSamples[i]) {
		if (const CDSample *pSample = m_pInstManager->GetDSample(i)) {
			pFile->WriteInt(i);
			const char *pName = pSample->GetName();
			int NameLen = strlen(pName);
			pFile->WriteInt(NameLen);
			pFile->Write(pName, NameLen);
			pFile->WriteInt(pSample->GetSize());
			pFile->Write(pSample->GetData(), pSample->GetSize());
		}
	}
}

bool CInstrument2A03::LoadFile(CInstrumentFile *pFile, int iVersion)
{
	char SampleNames[MAX_DSAMPLES][256];
	
	if (!CSeqInstrument::LoadFile(pFile, iVersion))		// // //
		return false;

	unsigned int Count;
	pFile->Read(&Count, sizeof(int));
	CModuleException::AssertRangeFmt(Count, 0U, static_cast<unsigned>(NOTE_COUNT), "DPCM assignment count", "%u");

	// DPCM instruments
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned char InstNote = pFile->ReadChar();
		int Octave = InstNote / NOTE_RANGE;
		int Note = InstNote % NOTE_RANGE;
		try {
			unsigned char Sample = CModuleException::AssertRangeFmt(pFile->ReadChar(), 0U, 0x7FU, "DPCM sample assignment index", "%u");
			if (Sample > MAX_DSAMPLES)
				Sample = 0;
			SetSamplePitch(Octave, Note, CModuleException::AssertRangeFmt(pFile->ReadChar(), 0U, 0xFU, "DPCM sample pitch", "%u"));
			SetSample(Octave, Note, Sample);
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
		pFile->Read(SampleNames[Index], Len);
		SampleNames[Index][Len] = 0;
		int Size = pFile->ReadInt();
		char *SampleData = new char[Size];
		pFile->Read(SampleData, Size);
		bool Found = false;
		for (int j = 0; j < MAX_DSAMPLES; ++j) if (const CDSample *pSample = m_pInstManager->GetDSample(j)) {		// // //
			// Compare size and name to see if identical sample exists
			if (pSample->GetSize() == Size && !memcmp(pSample->GetData(), SampleData, Size) &&		// // //
				!strcmp(pSample->GetName(), SampleNames[Index])) {
				Found = true;
				// Assign sample
				for (int o = 0; o < OCTAVE_RANGE; ++o) {
					for (int n = 0; n < NOTE_RANGE; ++n) {
						if (GetSample(o, n) == (Index + 1) && !bAssigned[o][n]) {
							SetSample(o, n, j + 1);
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
		pSample->SetName(SampleNames[Index]);
		pSample->SetData(Size, SampleData);
		int FreeSample = m_pInstManager->AddDSample(pSample);
		if (FreeSample == -1) {
			delete pSample;
			CModuleException *e = new CModuleException();
			e->AppendError("Document has no free DPCM sample slot");
			e->Raise();
		}
		TotalSize += Size;
		// Assign it
		for (int o = 0; o < OCTAVE_RANGE; ++o) {
			for (int n = 0; n < NOTE_RANGE; ++n) {
				if (GetSample(o, n) == (Index + 1) && !bAssigned[o][n]) {
					SetSample(o, n, FreeSample + 1);
					bAssigned[o][n] = true;
				}
			}
		}
	}

	return true;
}

char CInstrument2A03::GetSample(int Octave, int Note) const
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

void CInstrument2A03::SetSample(int Octave, int Note, char Sample)
{
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
			if (GetSample(i, j) != 0)
				return true;
		}
	}

	return false;
}
