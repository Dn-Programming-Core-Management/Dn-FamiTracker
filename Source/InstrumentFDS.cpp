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

#include <map>
#include <vector>
#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"
#include "Compiler.h"
#include "Chunk.h"
#include "DocumentFile.h"

const char TEST_WAVE[] = {
	00, 01, 12, 22, 32, 36, 39, 39, 42, 47, 47, 50, 48, 51, 54, 58,
	54, 55, 49, 50, 52, 61, 63, 63, 59, 56, 53, 51, 48, 47, 41, 35,
	35, 35, 41, 47, 48, 51, 53, 56, 59, 63, 63, 61, 52, 50, 49, 55,
	54, 58, 54, 51, 48, 50, 47, 47, 42, 39, 39, 36, 32, 22, 12, 01
};

const int FIXED_FDS_INST_SIZE = 1 + 16 + 4 + 1;

CInstrumentFDS::CInstrumentFDS() : CInstrument(INST_FDS)		// // //
{
	memcpy(m_iSamples, TEST_WAVE, WAVE_SIZE);	
	memset(m_iModulation, 0, MOD_SIZE);

	m_iModulationSpeed = 0;
	m_iModulationDepth = 0;
	m_iModulationDelay = 0;
	m_bModulationEnable = true;

	m_pVolume = new CSequence();
	m_pArpeggio = new CSequence();
	m_pPitch = new CSequence();
}

CInstrumentFDS::~CInstrumentFDS()
{
	SAFE_RELEASE(m_pVolume);
	SAFE_RELEASE(m_pArpeggio);
	SAFE_RELEASE(m_pPitch);
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
	pNewInst->m_pVolume->Copy(m_pVolume);
	pNewInst->m_pArpeggio->Copy(m_pArpeggio);
	pNewInst->m_pPitch->Copy(m_pPitch);

	// Copy name
	pNewInst->SetName(GetName());

	return pNewInst;
}

void CInstrumentFDS::Setup()
{
}

void CInstrumentFDS::StoreInstSequence(CInstrumentFile *pFile, CSequence *pSeq)
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

bool CInstrumentFDS::LoadInstSequence(CInstrumentFile *pFile, CSequence *pSeq)
{
	unsigned SeqCount = pFile->ReadInt();
	if (SeqCount > MAX_SEQUENCE_ITEMS)
		SeqCount = MAX_SEQUENCE_ITEMS;

	pSeq->Clear();
	pSeq->SetItemCount(SeqCount);
	pSeq->SetLoopPoint(pFile->ReadInt());
	pSeq->SetReleasePoint(pFile->ReadInt());
	pSeq->SetSetting(static_cast<seq_setting_t>(pFile->ReadInt()));		// // //

	for (unsigned i = 0; i < SeqCount; ++i)
		pSeq->SetItem(i, pFile->ReadChar());

	return true;
}

void CInstrumentFDS::StoreSequence(CDocumentFile *pDocFile, CSequence *pSeq)
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

bool CInstrumentFDS::LoadSequence(CDocumentFile *pDocFile, CSequence *pSeq)
{
	int SeqCount;
	unsigned int LoopPoint;
	unsigned int ReleasePoint;
	unsigned int Settings;

	SeqCount = (unsigned char)pDocFile->GetBlockChar();
	LoopPoint = pDocFile->GetBlockInt();
	ReleasePoint = pDocFile->GetBlockInt();
	Settings = pDocFile->GetBlockInt();

	ASSERT_FILE_DATA(SeqCount <= MAX_SEQUENCE_ITEMS);

	pSeq->Clear();
	pSeq->SetItemCount(SeqCount);
	pSeq->SetLoopPoint(LoopPoint);
	pSeq->SetReleasePoint(ReleasePoint);
	pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //

	for (int x = 0; x < SeqCount; ++x) {
		char Value = pDocFile->GetBlockChar();
		pSeq->SetItem(x, Value);
	}

	return true;
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
	StoreSequence(pDocFile, m_pVolume);
	StoreSequence(pDocFile, m_pArpeggio);
	StoreSequence(pDocFile, m_pPitch);
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
		LoadSequence(pDocFile, m_pVolume);
		LoadSequence(pDocFile, m_pArpeggio);
		if (pDocFile->GetBlockVersion() > 2)
			LoadSequence(pDocFile, m_pPitch);
	}
	else {
*/
	unsigned int a = pDocFile->GetBlockInt();
	unsigned int b = pDocFile->GetBlockInt();
	pDocFile->RollbackPointer(8);

	if (a < 256 && (b & 0xFF) != 0x00) {
	}
	else {
		LoadSequence(pDocFile, m_pVolume);
		LoadSequence(pDocFile, m_pArpeggio);
		//
		// Note: Remove this line when files are unable to load 
		// (if a file contains FDS instruments but FDS is disabled)
		// this was a problem in an earlier version.
		//
		if (pDocFile->GetBlockVersion() > 2)
			LoadSequence(pDocFile, m_pPitch);
	}

//	}

	// Older files was 0-15, new is 0-31
	if (pDocFile->GetBlockVersion() <= 3) {
		for (unsigned int i = 0; i < m_pVolume->GetItemCount(); ++i)
			m_pVolume->SetItem(i, m_pVolume->GetItem(i) * 2);
	}

	return true;
}

void CInstrumentFDS::SaveFile(CInstrumentFile *pFile, const CFamiTrackerDoc *pDoc)
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
	StoreInstSequence(pFile, m_pVolume);
	StoreInstSequence(pFile, m_pArpeggio);
	StoreInstSequence(pFile, m_pPitch);
}

bool CInstrumentFDS::LoadFile(CInstrumentFile *pFile, int iVersion, CFamiTrackerDoc *pDoc)
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
	LoadInstSequence(pFile, m_pVolume);
	LoadInstSequence(pFile, m_pArpeggio);
	LoadInstSequence(pFile, m_pPitch);

	if (iVersion <= 22) {
		for (unsigned int i = 0; i < m_pVolume->GetItemCount(); ++i)
			m_pVolume->SetItem(i, m_pVolume->GetItem(i) * 2);
	}

	return true;
}

int CInstrumentFDS::Compile(CFamiTrackerDoc *pDoc, CChunk *pChunk, int Index)
{
	CStringA str;

	// Store wave
//	int Table = pCompiler->AddWavetable(m_iSamples);
//	int Table = 0;
//	pChunk->StoreByte(Table);

	// Store modulation table, two entries/byte
	for (int i = 0; i < 16; ++i) {
		char Data = GetModulation(i << 1) | (GetModulation((i << 1) + 1) << 3);
		pChunk->StoreByte(Data);
	}
	
	pChunk->StoreByte(GetModulationDelay());
	pChunk->StoreByte(GetModulationDepth());
	pChunk->StoreWord(GetModulationSpeed());

	// Store sequences
	char Switch = (m_pVolume->GetItemCount() > 0 ? 1 : 0) | (m_pArpeggio->GetItemCount() > 0 ? 2 : 0) | (m_pPitch->GetItemCount() > 0 ? 4 : 0);

	pChunk->StoreByte(Switch);

	// Volume
	if (Switch & 1) {
		str.Format(CCompiler::LABEL_SEQ_FDS, Index * 5 + 0);
		pChunk->StoreReference(str);
	}

	// Arpeggio
	if (Switch & 2) {
		str.Format(CCompiler::LABEL_SEQ_FDS, Index * 5 + 1);
		pChunk->StoreReference(str);
	}
	
	// Pitch
	if (Switch & 4) {
		str.Format(CCompiler::LABEL_SEQ_FDS, Index * 5 + 2);
		pChunk->StoreReference(str);
	}

	int size = FIXED_FDS_INST_SIZE;
	size += (m_pVolume->GetItemCount() > 0 ? 2 : 0);
	size += (m_pArpeggio->GetItemCount() > 0 ? 2 : 0);
	size += (m_pPitch->GetItemCount() > 0 ? 2 : 0);
	return size;
}

bool CInstrumentFDS::CanRelease() const
{
	if (m_pVolume->GetItemCount() > 0) {
		if (m_pVolume->GetReleasePoint() != -1)
			return true;
		
	}
	return false;
}

unsigned char CInstrumentFDS::GetSample(int Index) const
{
	ASSERT(Index < WAVE_SIZE);
	return m_iSamples[Index];
}

void CInstrumentFDS::SetSample(int Index, int Sample)
{
	ASSERT(Index < WAVE_SIZE);
	m_iSamples[Index] = Sample;
	InstrumentChanged();
}

int CInstrumentFDS::GetModulation(int Index) const
{
	return m_iModulation[Index];
}

void CInstrumentFDS::SetModulation(int Index, int Value)
{
	m_iModulation[Index] = Value;
	InstrumentChanged();
}

int CInstrumentFDS::GetModulationSpeed() const
{
	return m_iModulationSpeed;
}

void CInstrumentFDS::SetModulationSpeed(int Speed)
{
	m_iModulationSpeed = Speed;
	InstrumentChanged();
}

int CInstrumentFDS::GetModulationDepth() const
{
	return m_iModulationDepth;
}

void CInstrumentFDS::SetModulationDepth(int Depth)
{
	m_iModulationDepth = Depth;
	InstrumentChanged();
}

int CInstrumentFDS::GetModulationDelay() const
{
	return m_iModulationDelay;
}

void CInstrumentFDS::SetModulationDelay(int Delay)
{
	m_iModulationDelay = Delay;
	InstrumentChanged();
}

CSequence* CInstrumentFDS::GetVolumeSeq() const
{
	return m_pVolume;
}

CSequence* CInstrumentFDS::GetArpSeq() const
{
	return m_pArpeggio;
}

CSequence* CInstrumentFDS::GetPitchSeq() const
{
	return m_pPitch;
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
