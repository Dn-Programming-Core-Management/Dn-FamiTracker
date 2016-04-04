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
#include "FamiTrackerTypes.h"		// // //
#include "APU/Types.h"		// // //
#include "Instrument.h"
#include "ChannelHandler.h"
#include "ChannelsFDS.h"
#include "InstHandler.h"		// // //
#include "SeqInstHandler.h"		// // //
#include "SeqInstHandlerFDS.h"		// // //

CChannelHandlerFDS::CChannelHandlerFDS() : 
	CChannelHandlerInverted(0xFFF, 32)
{ 
	memset(m_iModTable, 0, 32);
	memset(m_iWaveTable, 0, 64);

	m_bResetMod = false;
}

void CChannelHandlerFDS::HandleNoteData(stChanNote *pNoteData, int EffColumns)
{
	m_iEffModDepth = -1;
	if (!m_bAutoModulation) {		// // //
		m_iEffModSpeedHi = -1;
		m_iEffModSpeedLo = -1;
	}
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

bool CChannelHandlerFDS::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_FDS_MOD_DEPTH:
		if (EffParam < 0x40)		// // //
			m_iEffModDepth = EffParam;
		else if (EffParam >= 0x80 && m_bAutoModulation) {
			m_iEffModSpeedHi = EffParam - 0x80;
		}
		break;
	case EF_FDS_MOD_SPEED_HI:
		if (EffParam >= 0x10) {		// // //
			m_iEffModSpeedHi = EffParam >> 4;
			m_iEffModSpeedLo = (EffParam & 0x0F) + 1;
			m_bAutoModulation = true;
		}
		else {
			m_iEffModSpeedHi = EffParam;
			if (m_bAutoModulation)
				m_iEffModSpeedLo = 0;
			m_bAutoModulation = false;
		}
		break;
	case EF_FDS_MOD_SPEED_LO:
		m_iEffModSpeedLo = EffParam;
		if (m_bAutoModulation)		// // //
			m_iEffModSpeedHi = 0;
		m_bAutoModulation = false;
		break;
	case EF_FDS_VOLUME:
		if (EffParam < 0x80) {
			m_iVolModRate = EffParam & 0x3F;
			m_iVolModMode = (EffParam >> 6) + 1;
		}
		else if (EffParam == 0xE0)
			m_iVolModMode = 0;
		break;
	case EF_FDS_MOD_BIAS:		// // //
		m_iModulationOffset = EffParam - 0x80;
		break;
	default: return CChannelHandlerInverted::HandleEffect(EffNum, EffParam);
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
	}
}

void CChannelHandlerFDS::HandleNote(int Note, int Octave)
{
	// Trigger a new note
	m_iNote	= RunNote(Octave, Note);
	m_bResetMod = true;

	m_iInstVolume = 0x1F;
}

bool CChannelHandlerFDS::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_FDS:
		SAFE_RELEASE(m_pInstHandler);
		m_pInstHandler = new CSeqInstHandlerFDS(this, 0x1F, 0);
		return true;
	}
	return false;
}

void CChannelHandlerFDS::RefreshChannel()
{
	int Frequency = CalculatePeriod();
	unsigned char LoFreq = Frequency & 0xFF;
	unsigned char HiFreq = (Frequency >> 8) & 0x0F;

	unsigned char ModFreqLo = m_iModulationSpeed & 0xFF;
	unsigned char ModFreqHi = (m_iModulationSpeed >> 8) & 0x0F;
	if (m_bAutoModulation) {		// // //
		int newFreq = Frequency * m_iEffModSpeedHi / m_iEffModSpeedLo + m_iModulationOffset;
		newFreq = std::min(0xFFF, std::max(0, newFreq));
		ModFreqLo = newFreq & 0xFF;
		ModFreqHi = (newFreq >> 8) & 0x0F;
	}

	unsigned char Volume = CalculateVolume();

	if (!m_bGate) {		// // //
		WriteExternalRegister(0x4080, 0x80 | Volume);
		return;
	}

	// Write frequency
	WriteExternalRegister(0x4082, LoFreq);
	WriteExternalRegister(0x4083, HiFreq);

	// Write volume
	if (m_iVolModMode) {		// // //
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

	m_iInstVolume = 0x20;

	m_bAutoModulation = false;		// // //
	m_iModulationOffset = 0;		// // //
	m_iVolModMode = 0;
	m_iVolModRate = 0;
	m_bVolModTrigger = false;

	memset(m_iModTable, 0, 32);
	memset(m_iWaveTable, 0, 64);
}

CString CChannelHandlerFDS::GetCustomEffectString() const		// // //
{
	CString str = _T("");

	if (m_iVolModMode)
		str.AppendFormat(_T(" E%02X"), ((m_iVolModMode - 1) << 6) | m_iVolModRate);
	if (m_iEffModDepth != -1)
		str.AppendFormat(_T(" H%02X"), m_iEffModDepth);
	if (m_bAutoModulation) {
		str.AppendFormat(_T(" I%X%X"), m_iEffModSpeedHi > 0xF ? 1 : m_iEffModSpeedHi, m_iEffModSpeedLo - 1);
		if (m_iEffModSpeedHi > 0xF)
			str.AppendFormat(_T(" H%02X"), 0x80 + m_iEffModSpeedHi);
		if (m_iModulationOffset != 0)
			str.AppendFormat(_T(" Z%02X"), m_iModulationOffset + 0x80);
	}
	else {
		if (m_iModulationSpeed >> 8)
			str.AppendFormat(_T(" I%02X"), m_iModulationSpeed >> 8);
		if (m_iModulationSpeed & 0xFF)
			str.AppendFormat(_T(" J%02X"), m_iModulationSpeed & 0xFF);
	}

	return str;
}

void CChannelHandlerFDS::SetFMSpeed(int Speed)		// // //
{
	ASSERT(Speed >= 0 && Speed <= 0x3F);
	m_iModulationSpeed = Speed;
}

void CChannelHandlerFDS::SetFMDepth(int Depth)		// // //
{
	ASSERT(Depth >= 0 && Depth <= 0xFFF);
	m_iModulationDepth = Depth;
}

void CChannelHandlerFDS::SetFMDelay(int Delay)		// // //
{
	ASSERT(Delay >= 0 && Delay <= 0xFF);
	m_iModulationDelay = Delay;
}

void CChannelHandlerFDS::FillWaveRAM(const char *pBuffer)		// // //
{
	if (memcmp(m_iWaveTable, pBuffer, sizeof(m_iWaveTable))) {
		memcpy(m_iWaveTable, pBuffer, sizeof(m_iWaveTable));

		// Fills the 64 byte waveform table
		// Enable write for waveform RAM
		WriteExternalRegister(0x4089, 0x80);

		// This is the time the loop takes in NSF code
		AddCycles(1088);

		// Wave ram
		for (int i = 0; i < 0x40; ++i)
			WriteExternalRegister(0x4040 + i, m_iWaveTable[i]);

		// Disable write for waveform RAM, master volume = full
		WriteExternalRegister(0x4089, 0x00);
	}
}

void CChannelHandlerFDS::FillModulationTable(const char *pBuffer)		// // //
{
	if (memcmp(m_iModTable, pBuffer, sizeof(m_iModTable))) {
		memcpy(m_iModTable, pBuffer, sizeof(m_iModTable));

		// Disable modulation
		WriteExternalRegister(0x4087, 0x80);
		// Reset modulation table pointer, set bias to zero
		WriteExternalRegister(0x4085, 0x00);
		// Fill the table
		for (int i = 0; i < 32; ++i)
			WriteExternalRegister(0x4088, m_iModTable[i]);
	}
}
