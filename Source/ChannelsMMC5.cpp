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

// MMC5 file

#include <cmath>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "ChannelsMMC5.h"
#include "SoundGen.h"

const int CChannelHandlerMMC5::SEQ_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};

CChannelHandlerMMC5::CChannelHandlerMMC5() : CChannelHandler(0x7FF, 0x0F)
{
	m_bHardwareEnvelope = false;		// // //
	m_bEnvelopeLoop = true;
	m_bResetEnvelope = false;
	m_iLengthCounter = 1;
}

void CChannelHandlerMMC5::HandleNoteData(stChanNote *pNoteData, int EffColumns)
{
	// // //
	CChannelHandler::HandleNoteData(pNoteData, EffColumns);

	if (pNoteData->Note != NONE && pNoteData->Note != HALT && pNoteData->Note != RELEASE) {
		if (!m_bEnvelopeLoop || m_bHardwareEnvelope)		// // //
			m_bResetEnvelope = true;
	}
}

void CChannelHandlerMMC5::HandleCustomEffects(int EffNum, int EffParam)
{
	if (!CheckCommonEffects(EffNum, EffParam)) {
		switch (EffNum) {
			case EF_VOLUME:
				if (EffParam < 0x20) {		// // //
					m_iLengthCounter = EffParam;
					m_bEnvelopeLoop = false;
					m_bResetEnvelope = true;
				}
				else if (EffParam >= 0xE0 && EffParam < 0xE4) {
					if (!m_bEnvelopeLoop || !m_bHardwareEnvelope)
						m_bResetEnvelope = true;
					m_bHardwareEnvelope = ((EffParam & 0x01) == 0x01);
					m_bEnvelopeLoop = ((EffParam & 0x02) != 0x02);
				}
				break;
			case EF_DUTY_CYCLE:
				m_iDefaultDuty = m_iDutyPeriod = EffParam;
				break;
			// // //
		}
	}
}

bool CChannelHandlerMMC5::HandleInstrument(int Instrument, bool Trigger, bool NewInstrument)
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();
	CInstrumentContainer<CInstrument2A03> instContainer(pDocument, Instrument);
	CInstrument2A03 *pInstrument = instContainer();

	if (pInstrument == NULL)
		return false;

	for (int i = 0; i < CInstrument2A03::SEQUENCE_COUNT; ++i) {
		const CSequence *pSequence = pDocument->GetSequence(SNDCHIP_NONE, pInstrument->GetSeqIndex(i), i);
		if (Trigger || !IsSequenceEqual(i, pSequence) || pInstrument->GetSeqEnable(i) > GetSequenceState(i)) {
			if (pInstrument->GetSeqEnable(i) == 1)
				SetupSequence(i, pSequence);
			else 
				ClearSequence(i);
		}
	}

	return true;
}

void CChannelHandlerMMC5::HandleEmptyNote()
{
}

void CChannelHandlerMMC5::HandleCut()
{
	CutNote();
}

void CChannelHandlerMMC5::HandleRelease()
{
	if (!m_bRelease) {
		ReleaseNote();
		ReleaseSequences();
	}
}

void CChannelHandlerMMC5::HandleNote(int Note, int Octave)
{
	m_iNote		  = RunNote(Octave, Note);
	m_iDutyPeriod = m_iDefaultDuty;
	m_iSeqVolume  = 0x0F;		// // //
}

void CChannelHandlerMMC5::ProcessChannel()
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();

	// Default effects
	CChannelHandler::ProcessChannel();

	// Sequences
	for (int i = 0; i < SEQUENCES; ++i)
		RunSequence(i);
}

void CChannelHandlerMMC5::ResetChannel()
{
	CChannelHandler::ResetChannel();
	m_bEnvelopeLoop = true;		// // //
	m_bHardwareEnvelope = false;
	m_iLengthCounter = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// // // MMC5 Channels
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CChannelHandlerMMC5::RefreshChannel()		// // //
{
	int Period = CalculatePeriod();
	int Volume = CalculateVolume();
	char DutyCycle = (m_iDutyPeriod & 0x03);

	unsigned char HiFreq		= (Period & 0xFF);
	unsigned char LoFreq		= (Period >> 8);
	unsigned char LastLoFreq	= (m_iLastPeriod >> 8);
	unsigned int  Offs			= 0x5000 + 4 * (m_iChannelID - CHANID_MMC5_SQUARE1);

	m_iLastPeriod = Period;

	WriteExternalRegister(0x5015, 0x03);
	
	if (m_bGate)		// // //
		WriteExternalRegister(Offs, (DutyCycle << 6) | (m_bEnvelopeLoop << 5) | (!m_bHardwareEnvelope << 4) | Volume);
	else {
		WriteExternalRegister(Offs, 0x30);
		m_iLastPeriod = 0xFFFF;
		return;
	}
	WriteExternalRegister(Offs + 2, HiFreq);
	if (LoFreq != LastLoFreq || m_bResetEnvelope)		// // //
		WriteExternalRegister(Offs + 3, LoFreq + (m_iLengthCounter << 3));

	m_iLastPeriod = Period;		// // //
	m_bResetEnvelope = false;
}

void CChannelHandlerMMC5::ClearRegisters()
{
	unsigned char Offs = 0x5000 + 4 * (m_iChannelID - CHANID_MMC5_SQUARE1);		// // //
	WriteExternalRegister(Offs, 0x30);
	WriteExternalRegister(Offs + 2, 0);
	WriteExternalRegister(Offs + 3, 0);
}

CString CChannelHandlerMMC5::GetCustomEffectString() const		// // //
{
	CString str = _T("");

	if (!m_bEnvelopeLoop)
		str.AppendFormat(_T(" E%02X"), m_iLengthCounter);
	if (!m_bEnvelopeLoop || m_bHardwareEnvelope)
		str.AppendFormat(_T(" EE%X"), !m_bEnvelopeLoop * 2 + m_bHardwareEnvelope);

	return str;
}
