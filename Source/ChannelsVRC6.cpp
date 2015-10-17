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

// This file handles playing of VRC6 channels

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "ChannelsVRC6.h"
#include "SoundGen.h"

CChannelHandlerVRC6::CChannelHandlerVRC6() : CChannelHandler(0xFFF, 0x0F)
{
}

void CChannelHandlerVRC6::HandleCustomEffects(int EffNum, int EffParam)
{
	if (!CheckCommonEffects(EffNum, EffParam)) {
		switch (EffNum) {
			case EF_DUTY_CYCLE:
				m_iDefaultDuty = m_iDutyPeriod = EffParam;
				break;
			// // //
		}
	}
}

bool CChannelHandlerVRC6::HandleInstrument(int Instrument, bool Trigger, bool NewInstrument)
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();
	CInstrumentContainer<CSeqInstrument> instContainer(pDocument, Instrument);		// // //
	CSeqInstrument *pInstrument = instContainer();

	if (pInstrument == NULL)
		return false;

	for (int i = 0; i < SEQ_COUNT; ++i) {
		const CSequence *pSequence = pDocument->GetSequence(pInstrument->GetType(), pInstrument->GetSeqIndex(i), i); // // //
		if (Trigger || !IsSequenceEqual(i, pSequence) || pInstrument->GetSeqEnable(i) > GetSequenceState(i)) {
			if (pInstrument->GetSeqEnable(i) == 1)
				SetupSequence(i, pSequence);
			else
				ClearSequence(i);
		}
	}

	return true;
}

void CChannelHandlerVRC6::HandleEmptyNote()
{
}

void CChannelHandlerVRC6::HandleCut()
{
	CutNote();
}

void CChannelHandlerVRC6::HandleRelease()
{
	if (!m_bRelease) {
		ReleaseNote();
		ReleaseSequences();
	}
}

void CChannelHandlerVRC6::HandleNote(int Note, int Octave)
{
	m_iNote		  = RunNote(Octave, Note);
	m_iSeqVolume  = 0x0F;
	m_iDutyPeriod = m_iDefaultDuty;
}

void CChannelHandlerVRC6::ProcessChannel()
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();

	// Default effects
	CChannelHandler::ProcessChannel();

	// Sequences
	for (int i = 0; i < SEQ_COUNT; ++i)
		RunSequence(i);
}

void CChannelHandlerVRC6::ResetChannel()
{
	CChannelHandler::ResetChannel();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC6 Square 1
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Square1::RefreshChannel()
{
	unsigned int Period = CalculatePeriod();
	unsigned int Volume = CalculateVolume();
	unsigned char DutyCycle = m_iDutyPeriod << 4;

	unsigned char HiFreq = (Period & 0xFF);
	unsigned char LoFreq = (Period >> 8);
	
	if (!m_bGate) {		// // //
		WriteExternalRegister(0x9000, DutyCycle);
		return;
	}

	WriteExternalRegister(0x9000, DutyCycle | Volume);
	WriteExternalRegister(0x9001, HiFreq);
	WriteExternalRegister(0x9002, 0x80 | LoFreq);
}

void CVRC6Square1::ClearRegisters()
{
	WriteExternalRegister(0x9000, 0);
	WriteExternalRegister(0x9001, 0);
	WriteExternalRegister(0x9002, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC6 Square 2
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Square2::RefreshChannel()
{
	unsigned int Period = CalculatePeriod();
	unsigned int Volume = CalculateVolume();
	unsigned char DutyCycle = m_iDutyPeriod << 4;

	unsigned char HiFreq = (Period & 0xFF);
	unsigned char LoFreq = (Period >> 8);
	
	if (!m_bGate) {		// // //
		WriteExternalRegister(0xA000, DutyCycle);
		return;
	}

	WriteExternalRegister(0xA000, DutyCycle | Volume);
	WriteExternalRegister(0xA001, HiFreq);
	WriteExternalRegister(0xA002, 0x80 | LoFreq);
}

void CVRC6Square2::ClearRegisters()
{
	WriteExternalRegister(0xA000, 0);
	WriteExternalRegister(0xA001, 0);
	WriteExternalRegister(0xA002, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC6 Sawtooth
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Sawtooth::RefreshChannel()
{
	unsigned int Period = CalculatePeriod();

	unsigned char HiFreq = (Period & 0xFF);
	unsigned char LoFreq = (Period >> 8);

	unsigned int TremVol = GetTremolo();
	int Volume = (m_iSeqVolume * (m_iVolume >> VOL_COLUMN_SHIFT)) / 15 - TremVol;

	Volume = (Volume << 1) | ((m_iDutyPeriod & 1) << 5);

	if (Volume < 0)
		Volume = 0;
	if (Volume > 63)
		Volume = 63;

	if (m_iSeqVolume > 0 && m_iVolume > 0 && Volume == 0)
		Volume = 2;

	if (!m_bGate)
		Volume = 0;
	
	if (!m_bGate) {		// // //
		WriteExternalRegister(0xB000, 0);
		return;
	}

	WriteExternalRegister(0xB000, Volume);
	WriteExternalRegister(0xB001, HiFreq);
	WriteExternalRegister(0xB002, 0x80 | LoFreq);
}

void CVRC6Sawtooth::ClearRegisters()
{
	WriteExternalRegister(0xB000, 0);
	WriteExternalRegister(0xB001, 0);
	WriteExternalRegister(0xB002, 0);
}
