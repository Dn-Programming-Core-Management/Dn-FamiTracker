/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
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
#include "FamiTracker.h"		// // //
#include "Settings.h"		// // //

CChannelHandlerFDS::CChannelHandlerFDS() : 
	FrequencyChannelHandler(0xFFF, 32)
{ 
	memset(m_iModTable, 0, 32);
	memset(m_iWaveTable, 0, 64);
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
	case EF_PHASE_RESET:
		if (EffParam == 0) {
			resetPhase();
		}
		break;
	default: return FrequencyChannelHandler::HandleEffect(EffNum, EffParam);
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

int CChannelHandlerFDS::CalculateVolume() const		// // //
{
	if (!theApp.GetSettings()->General.bFDSOldVolume)		// // // match NSF setting
		return LimitVolume(((m_iInstVolume + 1) * ((m_iVolume >> VOL_COLUMN_SHIFT) + 1) - 1) / 16 - GetTremolo());
	return CChannelHandler::CalculateVolume();
}

bool CChannelHandlerFDS::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B:
		switch (m_iInstTypeCurrent) {
		case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: break;
		default:
			m_pInstHandler.reset(new CSeqInstHandler(this, 0x0F, Type == INST_S5B ? 0x40 : 0));
			return true;
		}
		break;
	case INST_FDS:
		switch (m_iInstTypeCurrent) {
		case INST_FDS: break;
		default:
			m_pInstHandler.reset(new CSeqInstHandlerFDS(this, 0x1F, 0));
			return true;
		}
	}
	return false;
}

void CChannelHandlerFDS::RefreshChannel()
{
	unsigned char Volume = CalculateVolume();

	if (!m_bGate) {		// // //
		WriteRegister(0x4080, 0x80 | Volume);
		return;
	}

	int CarrierFrequency = CalculatePeriod();
	unsigned char LoFreq = CarrierFrequency & 0xFF;
	unsigned char HiFreq = (CarrierFrequency >> 8) & 0x0F;

	unsigned char ModFreqLo = m_iModulationSpeed & 0xFF;
	unsigned char ModFreqHi = (m_iModulationSpeed >> 8) & 0x0F;
	if (m_bAutoModulation) {		// // //
		int FundFrequency = CalculatePeriod(false);
		int ModFrequency = FundFrequency * m_iEffModSpeedHi / m_iEffModSpeedLo + m_iModulationOffset;
		if (ModFrequency < 0) ModFrequency = 0;
		if (ModFrequency > 0xFFF) ModFrequency = 0xFFF;
		ModFreqLo = ModFrequency & 0xFF;
		ModFreqHi = (ModFrequency >> 8) & 0x0F;
	}

	// Write volume
	if (m_iVolModMode) {		// // //
		if (m_bVolModTrigger) {
			m_bVolModTrigger = false;
			WriteRegister(0x4080, 0x80 | Volume);
		}
		WriteRegister(0x4080, ((2 - m_iVolModMode) << 6) | m_iVolModRate);
	}
	else
		WriteRegister(0x4080, 0x80 | Volume);

	// Write frequency
	WriteRegister(0x4082, LoFreq);
	WriteRegister(0x4083, HiFreq);

	// Reset mod phase on new note
	if (m_bTrigger)
		writeModTable();

	// Update modulation unit
	if (m_iModulationDelay == 0) {
		// Modulation frequency
		WriteRegister(0x4086, ModFreqLo);
		WriteRegister(0x4087, ModFreqHi);

		// Sweep depth, disable sweep envelope
		WriteRegister(0x4084, 0x80 | m_iModulationDepth); 
	}
	else {
		// Delayed modulation
		WriteRegister(0x4087, 0x80);
		m_iModulationDelay--;
	}
}

void CChannelHandlerFDS::ClearRegisters()
{	
	// Clear volume
	WriteRegister(0x4080, 0x80);

	// Silence channel
	WriteRegister(0x4082, 0x00);		// // //
	WriteRegister(0x4083, 0x80);

	// Default speed
	WriteRegister(0x408A, 0xFF);

	// Disable modulation
	WriteRegister(0x4086, 0x00);		// // //
	WriteRegister(0x4087, 0x00);
	WriteRegister(0x4084, 0x00);		// // //

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

void CChannelHandlerFDS::resetPhase()
{
	WriteRegister(0x4083, 0x80);
}

void CChannelHandlerFDS::writeModTable()
{
	// Disable modulation
	WriteRegister(0x4087, 0x80);

	// This is the time the loop takes in NSF code
	AddCycles(543);
	// Fill the table
	for (int i = 0; i < 32; ++i)
		WriteRegister(0x4088, m_iModTable[i]);
	// Reset modulation table pointer, set bias to zero
	WriteRegister(0x4085, 0x00);
}

void CChannelHandlerFDS::SetFMSpeed(int Speed)		// // //
{
	ASSERT(Speed >= 0 && Speed <= 0xFFF);
	m_iModulationSpeed = Speed;
}

void CChannelHandlerFDS::SetFMDepth(int Depth)		// // //
{
	ASSERT(Depth >= 0 && Depth <= 0x3F);
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
		WriteRegister(0x4089, 0x80);

		// This is the time the loop takes in NSF code
		AddCycles(960);

		// Wave ram
		for (int i = 0; i < 0x40; ++i)
			WriteRegister(0x4040 + i, m_iWaveTable[i]);

		// Disable write for waveform RAM, master volume = full
		WriteRegister(0x4089, 0x00);
	}
}

void CChannelHandlerFDS::FillModulationTable(const char *pBuffer)		// // //
{
	if (memcmp(m_iModTable, pBuffer, sizeof(m_iModTable))) {
		memcpy(m_iModTable, pBuffer, sizeof(m_iModTable));
		// This is the time the loop takes in NSF code
		AddCycles(319);
		writeModTable();
	}
}
