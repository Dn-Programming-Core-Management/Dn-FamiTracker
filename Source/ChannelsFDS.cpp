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

// Famicom disk sound

#include "stdafx.h"
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "ChannelsFDS.h"
#include "SoundGen.h"

CChannelHandlerFDS::CChannelHandlerFDS() : 
	CChannelHandlerInverted(0xFFF, 32)
{ 
	ClearSequences();

	memset(m_iModTable, 0, 32);
	memset(m_iWaveTable, 0, 64);

	m_bResetMod = false;
}

void CChannelHandlerFDS::HandleNoteData(stChanNote *pNoteData, int EffColumns)
{
	m_iEffModDepth = -1;
	m_iEffModSpeedHi = -1;
	m_iEffModSpeedLo = -1;
	m_bVolModTrigger = false;		// // //

	CChannelHandler::HandleNoteData(pNoteData, EffColumns);
	// // //
	if (pNoteData->Note != NONE && pNoteData->Note != HALT && pNoteData->Note != RELEASE)
		m_bVolModTrigger = true;

	if (m_iEffModDepth != -1)
		m_iModulationDepth = m_iEffModDepth;

	if (m_iEffModSpeedHi != -1)
		m_iModulationSpeed = (m_iModulationSpeed & 0xFF) | (m_iEffModSpeedHi << 8);

	if (m_iEffModSpeedLo != -1)
		m_iModulationSpeed = (m_iModulationSpeed & 0xF00) | m_iEffModSpeedLo;
}

void CChannelHandlerFDS::HandleCustomEffects(int EffNum, int EffParam)
{
	if (EffNum == EF_PORTA_DOWN) {
		m_iPortaSpeed = EffParam;
		m_iEffect = EF_PORTA_UP;
	}
	else if (EffNum == EF_PORTA_UP) {
		m_iPortaSpeed = EffParam;
		m_iEffect = EF_PORTA_DOWN;
	}
	else if (!CheckCommonEffects(EffNum, EffParam)) {
		// Custom effects
		switch (EffNum) {
			// // //
			case EF_FDS_MOD_DEPTH:
				m_iEffModDepth = EffParam & 0x3F;
				break;
			case EF_FDS_MOD_SPEED_HI:
				m_iEffModSpeedHi = EffParam & 0x0F;
				break;
			case EF_FDS_MOD_SPEED_LO:
				m_iEffModSpeedLo = EffParam;
				break;
			case EF_FDS_VOLUME:
				if (EffParam <= 0x7F) {
					m_iVolModRate = EffParam & 0x3F;
					m_iVolModMode = (EffParam >> 6) + 1;
				}
				else if (EffParam == 0xE0)
					m_iVolModMode = 0;
				break;
		}
	}
}

bool CChannelHandlerFDS::HandleInstrument(int Instrument, bool Trigger, bool NewInstrument)
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();
	CInstrumentContainer<CInstrumentFDS> instContainer(pDocument, Instrument);	// TODO check this
	CInstrumentFDS *pInstrument = instContainer();

	if (pInstrument == NULL)
		return false;

	if (Trigger || NewInstrument) {
		FillWaveRAM(pInstrument);
		FillModulationTable(pInstrument);
	}

	if (Trigger) {
		CSequence *pVolSeq = pInstrument->GetVolumeSeq();
		CSequence *pArpSeq = pInstrument->GetArpSeq();
		CSequence *pPitchSeq = pInstrument->GetPitchSeq();

		(pVolSeq->GetItemCount() > 0) ? SetupSequence(SEQ_VOLUME, pVolSeq) : ClearSequence(SEQ_VOLUME);
		(pArpSeq->GetItemCount() > 0) ? SetupSequence(SEQ_ARPEGGIO, pArpSeq) : ClearSequence(SEQ_ARPEGGIO);
		(pPitchSeq->GetItemCount() > 0) ? SetupSequence(SEQ_PITCH, pPitchSeq) : ClearSequence(SEQ_PITCH);

//			if (pInstrument->GetModulationEnable()) {
			m_iModulationSpeed = pInstrument->GetModulationSpeed();
			m_iModulationDepth = pInstrument->GetModulationDepth();
			m_iModulationDelay = pInstrument->GetModulationDelay();
//			}
	}

	return true;
}

void CChannelHandlerFDS::HandleEmptyNote()
{
}

void CChannelHandlerFDS::HandleCut()
{
	CutNote();
}

void CChannelHandlerFDS::HandleRelease()
{
	if (!m_bRelease) {
		ReleaseNote();
		ReleaseSequences();
	}
}

void CChannelHandlerFDS::HandleNote(int Note, int Octave)
{
	// Trigger a new note
	m_iNote	= RunNote(Octave, Note);
	m_bResetMod = true;
	m_iLastInstrument = m_iInstrument;

	m_iSeqVolume = 0x1F;
}

void CChannelHandlerFDS::ProcessChannel()
{
	// Default effects
	CChannelHandler::ProcessChannel();	

	// Sequences
	if (GetSequenceState(SEQ_VOLUME) != SEQ_STATE_DISABLED)
		RunSequence(SEQ_VOLUME);

	if (GetSequenceState(SEQ_ARPEGGIO) != SEQ_STATE_DISABLED)
		RunSequence(SEQ_ARPEGGIO);

	if (GetSequenceState(SEQ_PITCH) != SEQ_STATE_DISABLED)
		RunSequence(SEQ_PITCH);
}

void CChannelHandlerFDS::RefreshChannel()
{
	CheckWaveUpdate();	

	int Frequency = CalculatePeriod();
	unsigned char LoFreq = Frequency & 0xFF;
	unsigned char HiFreq = (Frequency >> 8) & 0x0F;

	unsigned char ModFreqLo = m_iModulationSpeed & 0xFF;
	unsigned char ModFreqHi = (m_iModulationSpeed >> 8) & 0x0F;

	unsigned char Volume = CalculateVolume();

	if (!m_bGate)
		Volume = 0;

	// Write frequency
	WriteExternalRegister(0x4082, LoFreq);
	WriteExternalRegister(0x4083, HiFreq);

	// Write volume
	if (m_iVolModMode) {
		if (m_bVolModTrigger) {
			m_bVolModTrigger = false;
			WriteExternalRegister(0x4080, 0x80 | Volume);
		}
		WriteExternalRegister(0x4080, ((2 - m_iVolModMode) << 6) | m_iVolModRate);
	}
	else
		WriteExternalRegister(0x4080, 0x80 | Volume);

	if (m_bResetMod)
		WriteExternalRegister(0x4085, 0);

	m_bResetMod = false;

	// Update modulation unit
	if (m_iModulationDelay == 0) {
		// Modulation frequency
		WriteExternalRegister(0x4086, ModFreqLo);
		WriteExternalRegister(0x4087, ModFreqHi);

		// Sweep depth, disable sweep envelope
		WriteExternalRegister(0x4084, 0x80 | m_iModulationDepth); 
	}
	else {
		// Delayed modulation
		WriteExternalRegister(0x4087, 0x80);
		m_iModulationDelay--;
	}

}

void CChannelHandlerFDS::ClearRegisters()
{	
	// Clear gain
	WriteExternalRegister(0x4090, 0x00);

	// Clear volume
	WriteExternalRegister(0x4080, 0x80);

	// Silence channel
	WriteExternalRegister(0x4082, 0x00);		// // //
	WriteExternalRegister(0x4083, 0x80);

	// Default speed
	WriteExternalRegister(0x408A, 0xFF);

	// Disable modulation
	WriteExternalRegister(0x4086, 0x00);		// // //
	WriteExternalRegister(0x4087, 0x00);
	WriteExternalRegister(0x4084, 0x00);		// // //

	m_iSeqVolume = 0x20;

	m_iVolModMode = 0;		// // //
	m_iVolModRate = 0;
	m_bVolModTrigger = 0;

	memset(m_iModTable, 0, 32);
	memset(m_iWaveTable, 0, 64);
}

CString CChannelHandlerFDS::GetCustomEffectString() const		// // //
{
	CString str = _T("");

	if (m_iVolModMode)
		str.AppendFormat(_T(" E%02X"), ((m_iVolModMode - 1) << 6) | m_iVolModRate);

	return str;
}

void CChannelHandlerFDS::FillWaveRAM(CInstrumentFDS *pInstrument)
{
	bool bNew(false);

	for (int i = 0; i < 64; ++i) {
		if (m_iWaveTable[i] != pInstrument->GetSample(i)) {
			bNew = true;
			break;
		}
	}

	if (bNew) {
		for (int i = 0; i < 64; ++i)
			m_iWaveTable[i] = pInstrument->GetSample(i);

		// Fills the 64 byte waveform table
		// Enable write for waveform RAM
		WriteExternalRegister(0x4089, 0x80);

		// This is the time the loop takes in NSF code
		AddCycles(1088);

		// Wave ram
		for (int i = 0; i < 0x40; ++i)
			WriteExternalRegister(0x4040 + i, pInstrument->GetSample(i));

		// Disable write for waveform RAM, master volume = full
		WriteExternalRegister(0x4089, 0x00);
	}
}

void CChannelHandlerFDS::FillModulationTable(CInstrumentFDS *pInstrument)
{
	// Fills the 32 byte modulation table

	bool bNew(true);

	for (int i = 0; i < 32; ++i) {
		if (m_iModTable[i] != pInstrument->GetModulation(i)) {
			bNew = true;
			break;
		}
	}

	if (bNew) {
		// Copy table
		for (int i = 0; i < 32; ++i)
			m_iModTable[i] = pInstrument->GetModulation(i);

		// Disable modulation
		WriteExternalRegister(0x4087, 0x80);
		// Reset modulation table pointer, set bias to zero
		WriteExternalRegister(0x4085, 0x00);
		// Fill the table
		for (int i = 0; i < 32; ++i)
			WriteExternalRegister(0x4088, m_iModTable[i]);
	}
}

void CChannelHandlerFDS::CheckWaveUpdate()
{
	// Check wave changes
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();
	bool bWaveChanged = theApp.GetSoundGenerator()->HasWaveChanged();

	if (m_iInstrument != MAX_INSTRUMENTS && bWaveChanged) {
		CInstrumentContainer<CInstrumentFDS> instContainer(pDocument, m_iInstrument);
		CInstrumentFDS *pInstrument = instContainer();
		if (pInstrument != NULL) {
			// Realtime update
			m_iModulationSpeed = pInstrument->GetModulationSpeed();
			m_iModulationDepth = pInstrument->GetModulationDepth();
			FillWaveRAM(pInstrument);
			FillModulationTable(pInstrument);
		}
	}
}
