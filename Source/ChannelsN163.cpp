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

#include <cmath>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "ChannelsN163.h"
#include "SoundGen.h"

const int N163_PITCH_SLIDE_SHIFT = 2;	// Increase amplitude of pitch slides

CChannelHandlerN163::CChannelHandlerN163() : 
	CChannelHandlerInverted(0xFFFF, 0x0F), 
	m_bLoadWave(false),
	m_bDisableLoad(false),		// // //
	m_bResetPhase(false),
	m_iWaveLen(0),
	m_iWaveIndex(0),
	m_iWaveCount(0)
{
	m_iDutyPeriod = 0;
}

void CChannelHandlerN163::ResetChannel()
{
	CChannelHandler::ResetChannel();

	m_iWaveIndex = 0;
	m_iWavePos = m_iWavePosOld = 0;		// // //
}

void CChannelHandlerN163::HandleNoteData(stChanNote *pNoteData, int EffColumns)
{
	m_bLoadWave = false;
	
	CChannelHandler::HandleNoteData(pNoteData, EffColumns);
	// // //
}

void CChannelHandlerN163::HandleCustomEffects(int EffNum, int EffParam)
{
	if (EffNum == EF_PORTA_DOWN) {
		m_iPortaSpeed = EffParam << N163_PITCH_SLIDE_SHIFT;
		m_iEffectParam = EffParam;		// // //
		m_iEffect = EF_PORTA_UP;
	}
	else if (EffNum == EF_PORTA_UP) {
		m_iPortaSpeed = EffParam << N163_PITCH_SLIDE_SHIFT;
		m_iEffectParam = EffParam;		// // //
		m_iEffect = EF_PORTA_DOWN;
	}
	else if (EffNum == EF_PORTAMENTO) {
		m_iPortaSpeed = EffParam << N163_PITCH_SLIDE_SHIFT;
		m_iEffectParam = EffParam;		// // //
		m_iEffect = EF_PORTAMENTO;
	}
	else if (!CheckCommonEffects(EffNum, EffParam)) {
		// Custom effects
		switch (EffNum) {
			case EF_DUTY_CYCLE:
				// Duty effect controls wave
				m_iWaveIndex = EffParam;
				m_bLoadWave = true;
				break;
			case EF_N163_WAVE_BUFFER:		// // //
				if (EffParam == 0x7F) {
					m_iWavePos = m_iWavePosOld;
					LoadWave();
					// m_bLoadWave = true;
					m_bDisableLoad = false;
				}
				else {
					CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();
					if (EffParam + (m_iWaveLen >> 1) > 0x80 - 8 * pDocument->GetNamcoChannels()) break;
					m_iWavePos = EffParam << 1;
					LoadWave();
					// m_bLoadWave = false;
					m_bDisableLoad = true;
				}
				break;
		}
	}

	/*
	switch (m_iEffect) {
	case EF_PORTA_DOWN: case EF_PORTA_UP: case EF_PORTAMENTO:
	case EF_SLIDE_UP: case EF_SLIDE_DOWN:
		m_iPortaSpeed <<= N163_PITCH_SLIDE_SHIFT;
		break;
	}
	*/
}

bool CChannelHandlerN163::HandleInstrument(int Instrument, bool Trigger, bool NewInstrument)
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();
	CInstrumentContainer<CInstrumentN163> instContainer(pDocument, Instrument);
	CInstrumentN163 *pInstrument = instContainer();

	if (pInstrument == NULL)
		return false;

	for (int i = 0; i < SEQ_COUNT; ++i) {
		const CSequence *pSequence = pDocument->GetSequence(SNDCHIP_N163, pInstrument->GetSeqIndex(i), i);
		if (Trigger || !IsSequenceEqual(i, pSequence) || pInstrument->GetSeqEnable(i) > GetSequenceState(i)) {
			if (pInstrument->GetSeqEnable(i) == 1)
				SetupSequence(i, pSequence);
			else
				ClearSequence(i);
		}
	}

	m_iWaveLen = pInstrument->GetWaveSize();
	if (!m_bDisableLoad) {		// // //
		m_iWavePos = /*pInstrument->GetAutoWavePos() ? GetIndex() * 16 :*/ pInstrument->GetWavePos();
		m_iWavePosOld = m_iWavePos;
	}
	m_iWaveCount = pInstrument->GetWaveCount();

	if (!m_bLoadWave && NewInstrument)
		m_iWaveIndex = 0;
	
	if (NewInstrument)		// // //
		m_bLoadWave = true;

	return true;
}

void CChannelHandlerN163::HandleEmptyNote()
{
}

void CChannelHandlerN163::HandleCut()
{
	CutNote();
	m_iNote = 0;
	m_bRelease = false;
}

void CChannelHandlerN163::HandleRelease()
{
	if (!m_bRelease) {
		ReleaseNote();
		ReleaseSequences();
	}
}

void CChannelHandlerN163::HandleNote(int Note, int Octave)
{
	// New note
	m_iNote	= RunNote(Octave, Note);
	m_iSeqVolume = 0x0F;
	m_bRelease = false;

//	m_bResetPhase = true;
}

void CChannelHandlerN163::SetupSlide()		// // //
{
	CChannelHandler::SetupSlide();
	m_iPortaSpeed <<= N163_PITCH_SLIDE_SHIFT;
}

void CChannelHandlerN163::ProcessChannel()
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();

	// Default effects
	CChannelHandler::ProcessChannel();

	bool bUpdateWave = GetSequenceState(SEQ_DUTYCYCLE) != SEQ_STATE_DISABLED;

	m_iChannels = pDocument->GetNamcoChannels() - 1;

	// Sequences
	for (int i = 0; i < CInstrumentN163::SEQUENCE_COUNT; ++i)
		RunSequence(i);

	if (bUpdateWave) {
		m_iWaveIndex = m_iDutyPeriod;
		m_bLoadWave = true;
	}
}

void CChannelHandlerN163::RefreshChannel()
{
	CheckWaveUpdate();

	int Channel = 7 - GetIndex();		// Channel #
	int WaveSize = 256 - (m_iWaveLen >> 2);
	int Frequency = LimitPeriod(GetPeriod() - ((-GetVibrato() + GetFinePitch() + GetPitch()) << 4)) << 2;		// // //

	// Compensate for shorter waves
//	Frequency >>= 5 - int(log(double(m_iWaveLen)) / log(2.0));

	int Volume = CalculateVolume();
	int ChannelAddrBase = 0x40 + Channel * 8;

	if (!m_bGate)
		Volume = 0;

	if (m_bLoadWave && m_bGate) {
		m_bLoadWave = false;
		LoadWave();
	}

	if (m_iLastInstrument != m_iInstrument) {		// // //
		m_iLastInstrument = m_iInstrument;
		LoadWave();
	}

	// Update channel
	if (Channel + m_pSoundGen->GetDocument()->GetNamcoChannels() >= 8) {		// // //
		WriteData(ChannelAddrBase + 0, Frequency & 0xFF);
		WriteData(ChannelAddrBase + 2, (Frequency >> 8) & 0xFF);
		WriteData(ChannelAddrBase + 4, (WaveSize << 2) | ((Frequency >> 16) & 0x03));
		WriteData(ChannelAddrBase + 6, m_iWavePos);
		WriteData(ChannelAddrBase + 7, (m_iChannels << 4) | Volume);
	}

	if (m_bResetPhase) {
		m_bResetPhase = false;
		WriteData(ChannelAddrBase + 1, 0);
		WriteData(ChannelAddrBase + 3, 0);
		WriteData(ChannelAddrBase + 5, 0);
	}

	// if (!m_bDisableLoad)		// // //
		// LoadWave(); // 0CC: check if there are side effects
}

void CChannelHandlerN163::ClearRegisters()
{
	int Channel = GetIndex();
	int ChannelAddrBase = 0x40 + Channel * 8;
	
	for (int i = 0; i < 8; i++) {		// // //
		WriteReg(ChannelAddrBase + i, 0);
		WriteReg(ChannelAddrBase + i - 0x40, 0);
	}
	
	if (Channel == 7)		// // //
		WriteReg(ChannelAddrBase + 7, (m_iChannels << 4) | 0);

	m_bLoadWave = false;
	m_bDisableLoad = false;		// // //
	m_iDutyPeriod = 0;
}

CString CChannelHandlerN163::GetCustomEffectString() const		// // //
{
	CString str = _T("");

	if (m_bDisableLoad)
		str.AppendFormat(_T(" Z%02X"), m_iWavePos >> 1);

	return str;
}

void CChannelHandlerN163::WriteReg(int Reg, int Value)
{
	WriteExternalRegister(0xF800, Reg);
	WriteExternalRegister(0x4800, Value);
}

void CChannelHandlerN163::SetAddress(char Addr, bool AutoInc)
{
	WriteExternalRegister(0xF800, (AutoInc ? 0x80 : 0) | Addr);
}

void CChannelHandlerN163::WriteData(char Data)
{
	WriteExternalRegister(0x4800, Data);
}

void CChannelHandlerN163::WriteData(int Addr, char Data)
{
	SetAddress(Addr, false);
	WriteData(Data);
}

void CChannelHandlerN163::LoadWave()
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();

	if (m_iInstrument == MAX_INSTRUMENTS)
		return;

	// Fill the wave RAM
	CInstrumentContainer<CInstrumentN163> instContainer(pDocument, m_iInstrument);
	CInstrumentN163 *pInstrument = instContainer();

	if (pInstrument == NULL)
		return;

	// Start of wave in memory
	int Channel = GetIndex();
	int StartAddr = m_iWavePos >> 1;

	SetAddress(StartAddr, true);

	if (m_iWaveIndex >= m_iWaveCount)
		m_iWaveIndex = m_iWaveCount - 1;

	for (int i = 0; i < m_iWaveLen; i += 2) {
		WriteData((pInstrument->GetSample(m_iWaveIndex, i + 1) << 4) | pInstrument->GetSample(m_iWaveIndex, i));
	}
}

void CChannelHandlerN163::CheckWaveUpdate()
{
	// Check wave changes
	if (theApp.GetSoundGenerator()->HasWaveChanged())
		m_bLoadWave = true;
}
