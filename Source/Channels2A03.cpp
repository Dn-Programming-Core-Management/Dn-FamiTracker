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

// This file handles playing of 2A03 channels

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerTypes.h"		// // //
#include "APU/Types.h"		// // //
#include "APU/APU.h"		// // // for DPCM
#include "DSample.h"		// // //
#include "Instrument.h"
#include "ChannelHandler.h"
#include "Channels2A03.h"
#include "Settings.h"
#include "InstHandler.h"		// // //
#include "SeqInstHandler.h"		// // //
#include "InstHandlerDPCM.h"		// // //

CChannelHandler2A03::CChannelHandler2A03() :
	CChannelHandler(0x7FF, 0x0F),
	m_bHardwareEnvelope(false),
	m_bEnvelopeLoop(true),
	m_bResetEnvelope(false),
	m_iLengthCounter(1)
{
}

void CChannelHandler2A03::HandleNoteData(stChanNote *pNoteData, int EffColumns)
{
	// // //
	CChannelHandler::HandleNoteData(pNoteData, EffColumns);

	if (pNoteData->Note != NONE && pNoteData->Note != HALT && pNoteData->Note != RELEASE) {
		if (!m_bEnvelopeLoop || m_bHardwareEnvelope)		// // //
			m_bResetEnvelope = true;
	}
}

bool CChannelHandler2A03::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
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
	default: return CChannelHandler::HandleEffect(EffNum, EffParam);
	}

	return true;
}

void CChannelHandler2A03::HandleEmptyNote()
{
	// // //
}

void CChannelHandler2A03::HandleCut()
{
	CutNote();
}

void CChannelHandler2A03::HandleRelease()
{
	if (!m_bRelease)
		ReleaseNote();
/*
	if (!m_bSweeping && (m_cSweep != 0 || m_iSweep != 0)) {
		m_iSweep = 0;
		m_cSweep = 0;
		m_iLastPeriod = 0xFFFF;
	}
	else if (m_bSweeping) {
		m_cSweep = m_iSweep;
		m_iLastPeriod = 0xFFFF;
	}
	*/
}

bool CChannelHandler2A03::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS:
		switch (m_iInstTypeCurrent) {
		case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS: break;
		default:
			m_pInstHandler.reset(new CSeqInstHandler(this, 0x0F, Type == INST_S5B ? 0x40 : 0));
			return true;
		}
	}
	return false;
}

void CChannelHandler2A03::ResetChannel()
{
	CChannelHandler::ResetChannel();
	m_bEnvelopeLoop = true;		// // //
	m_bHardwareEnvelope = false;
	m_iLengthCounter = 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// // // 2A03 Square
///////////////////////////////////////////////////////////////////////////////////////////////////////////

C2A03Square::C2A03Square() :
	CChannelHandler2A03(),
	m_cSweep(0),
	m_bSweeping(0),
	m_iSweep(0)
{
}

const char C2A03Square::MAX_DUTY = 0x03;

int C2A03Square::getDutyMax() const {
	return static_cast<int>(MAX_DUTY);
}

void C2A03Square::RefreshChannel()
{
	int Period = CalculatePeriod();
	int Volume = CalculateVolume();
	char DutyCycle = (m_iDutyPeriod & MAX_DUTY);

	unsigned char HiFreq = (Period & 0xFF);
	unsigned char LoFreq = (Period >> 8);
	
	int Address = 0x4000 + m_iChannel * 4;		// // //
	if (m_bGate)		// // //
		WriteRegister(Address, (DutyCycle << 6) | (m_bEnvelopeLoop << 5) | (!m_bHardwareEnvelope << 4) | Volume);		// // //
	else {
		WriteRegister(Address, 0x30);
		m_iLastPeriod = 0xFFFF;
		return;
	}

	if (m_cSweep) {
		if (m_cSweep & 0x80) {
			WriteRegister(Address + 1, m_cSweep);
			m_cSweep &= 0x7F;
			WriteRegister(0x4017, 0x80);	// Clear sweep unit
			WriteRegister(0x4017, 0x00);
			WriteRegister(Address + 2, HiFreq);
			WriteRegister(Address + 3, LoFreq + (m_iLengthCounter << 3));		// // //
			m_iLastPeriod = 0xFFFF;
		}
	}
	else {
		WriteRegister(Address + 1, 0x08);
		//WriteRegister(0x4017, 0x80);	// Manually execute one APU frame sequence to kill the sweep unit
		//WriteRegister(0x4017, 0x00);
		WriteRegister(Address + 2, HiFreq);
		
		if (LoFreq != (m_iLastPeriod >> 8) || m_bResetEnvelope)		// // //
			WriteRegister(Address + 3, LoFreq + (m_iLengthCounter << 3));
	}

	m_iLastPeriod = Period;
	m_bResetEnvelope = false;		// // //
}

void C2A03Square::SetChannelID(int ID)		// // //
{
	CChannelHandler::SetChannelID(ID);
	m_iChannel = ID - CHANID_SQUARE1;
}

int C2A03Square::ConvertDuty(int Duty) const		// // //
{
	switch (m_iInstTypeCurrent) {
	case INST_VRC6:	return DUTY_2A03_FROM_VRC6[Duty & 0x07];
	case INST_S5B:	return 0x02;
	default:		return Duty;
	}
}

void C2A03Square::ClearRegisters()
{
	int Address = 0x4000 + m_iChannel * 4;		// // //
	WriteRegister(Address + 0, 0x30);
	WriteRegister(Address + 1, 0x08);
	WriteRegister(Address + 2, 0x00);
	WriteRegister(Address + 3, 0x00);
	m_iLastPeriod = 0xFFFF;
}

void C2A03Square::HandleNoteData(stChanNote *pNoteData, int EffColumns)
{
	m_iSweep = 0;
	m_bSweeping = false;
	CChannelHandler2A03::HandleNoteData(pNoteData, EffColumns);
}

bool C2A03Square::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_SWEEPUP:
		m_iSweep = 0x88 | (EffParam & 0x77);
		m_iLastPeriod = 0xFFFF;
		m_bSweeping = true;
		break;
	case EF_SWEEPDOWN:
		m_iSweep = 0x80 | (EffParam & 0x77);
		m_iLastPeriod = 0xFFFF;
		m_bSweeping = true;
		break;
	case EF_PHASE_RESET:
		if (EffParam == 0) {
			resetPhase();
		}
		break;
	default: return CChannelHandler2A03::HandleEffect(EffNum, EffParam);
	}

	return true;
}

void C2A03Square::HandleEmptyNote()
{
	if (m_bSweeping)
		m_cSweep = m_iSweep;
}

void C2A03Square::HandleNote(int Note, int Octave)		// // //
{
	CChannelHandler2A03::HandleNote(Note, Octave);

	if (!m_bSweeping && (m_cSweep != 0 || m_iSweep != 0)) {
		m_iSweep = 0;
		m_cSweep = 0;
		m_iLastPeriod = 0xFFFF;
	}
	else if (m_bSweeping) {
		m_cSweep = m_iSweep;
		m_iLastPeriod = 0xFFFF;
	}
}

CString C2A03Square::GetCustomEffectString() const		// // //
{
	CString str = _T("");
	
	if (!m_bEnvelopeLoop)
		str.AppendFormat(_T(" E%02X"), m_iLengthCounter);
	if (!m_bEnvelopeLoop || m_bHardwareEnvelope)
		str.AppendFormat(_T(" EE%X"), !m_bEnvelopeLoop * 2 + m_bHardwareEnvelope);

	return str;
}

void C2A03Square::resetPhase()
{
	int Address = 0x4000 + m_iChannel * 4;
	int LoPeriod = CalculatePeriod() >> 8;
	WriteRegister(Address + 3, LoPeriod + (m_iLengthCounter << 3));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Triangle 
///////////////////////////////////////////////////////////////////////////////////////////////////////////

CTriangleChan::CTriangleChan() :		// // //
	CChannelHandler2A03(),
	m_iLinearCounter(-1)
{
}

void CTriangleChan::RefreshChannel()
{
	int Freq = CalculatePeriod();

	unsigned char HiFreq = (Freq & 0xFF);
	unsigned char LoFreq = (Freq >> 8);
	
	if (m_iInstVolume > 0 && m_iVolume > 0 && m_bGate) {
		WriteRegister(0x4008, (m_bEnvelopeLoop << 7) | (m_iLinearCounter & 0x7F));		// // //
		WriteRegister(0x400A, HiFreq);
		if (m_bEnvelopeLoop || m_bResetEnvelope)		// // //
			WriteRegister(0x400B, LoFreq + (m_iLengthCounter << 3));
	}
	else
		WriteRegister(0x4008, 0);

	m_bResetEnvelope = false;		// // //
}

void CTriangleChan::ResetChannel()
{
	CChannelHandler2A03::ResetChannel();
	m_iLinearCounter = -1;
}

int CTriangleChan::GetChannelVolume() const
{
	return m_iVolume ? VOL_COLUMN_MAX : 0;
}

bool CTriangleChan::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_VOLUME:
		if (EffParam < 0x20) {		// // //
			m_iLengthCounter = EffParam;
			m_bEnvelopeLoop = false;
			m_bResetEnvelope = true;
			if (m_iLinearCounter == -1)	m_iLinearCounter = 0x7F;
		}
		else if (EffParam >= 0xE0 && EffParam < 0xE4) {
			if (!m_bEnvelopeLoop)
				m_bResetEnvelope = true;
			m_bEnvelopeLoop = ((EffParam & 0x01) != 0x01);
		}
		break;
	case EF_NOTE_CUT:
		if (EffParam >= 0x80) {
			m_iLinearCounter = EffParam - 0x80;
			m_bEnvelopeLoop = false;
			m_bResetEnvelope = true;
		}
		else {
			m_bEnvelopeLoop = true;
			return CChannelHandler2A03::HandleEffect(EffNum, EffParam); // true
		}
		break;
	default: return CChannelHandler2A03::HandleEffect(EffNum, EffParam);
	}

	return true;
}

void CTriangleChan::ClearRegisters()
{
	WriteRegister(0x4008, 0);
	WriteRegister(0x400A, 0);
	WriteRegister(0x400B, 0);
}

CString CTriangleChan::GetCustomEffectString() const		// // //
{
	CString str = _T("");
	
	if (m_iLinearCounter > -1)
		str.AppendFormat(_T(" S%02X"), m_iLinearCounter | 0x80);
	if (!m_bEnvelopeLoop)
		str.AppendFormat(_T(" E%02X"), m_iLengthCounter);
	if (!m_bEnvelopeLoop)
		str.AppendFormat(_T(" EE%X"), !m_bEnvelopeLoop);

	return str;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Noise
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CNoiseChan::HandleNote(int Note, int Octave)
{
	CChannelHandler2A03::HandleNote(Note, Octave);		// // //

	int NewNote = (MIDI_NOTE(Octave, Note) & 0x0F) | 0x100;
	int NesFreq = TriggerNote(NewNote);

	// // // NesFreq = (NesFreq & 0x0F) | 0x10;

	// // // NewNote &= 0x0F;

	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO) {
		if (m_iPeriod == 0)
			m_iPeriod = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iPeriod = NesFreq;

	m_bGate = true;

	m_iNote			= NewNote;
}

void CNoiseChan::SetupSlide()		// // //
{
	#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1)

	switch (m_iEffect) {
	case EF_PORTAMENTO:
		m_iPortaSpeed = m_iEffectParam;
		break;
	case EF_SLIDE_UP:
		m_iNote += (m_iEffectParam & 0xF);
		m_iPortaSpeed = GET_SLIDE_SPEED(m_iEffectParam);
		break;
	case EF_SLIDE_DOWN:
		m_iNote -= (m_iEffectParam & 0xF);
		m_iPortaSpeed = GET_SLIDE_SPEED(m_iEffectParam);
		break;
	}

	#undef GET_SLIDE_SPEED

	RegisterKeyState(m_iNote);
	m_iPortaTo = m_iNote;
}

int CNoiseChan::LimitPeriod(int Period) const		// // //
{
	return Period; // no limit
}

int CNoiseChan::LimitRawPeriod(int Period) const		// // //
{
	return Period; // no limit
}

const char CNoiseChan::MAX_DUTY = 0x01;

int CNoiseChan::getDutyMax() const {
	return MAX_DUTY;
}

void CNoiseChan::RefreshChannel()
{
	int Period = CalculatePeriod();
	int Volume = CalculateVolume();
	char NoiseMode = (m_iDutyPeriod & MAX_DUTY) << 7;

	Period = Period & 0x0F;
	Period ^= 0x0F;
	
	if (m_bGate)		// // //
		WriteRegister(0x400C, (m_bEnvelopeLoop << 5) | (!m_bHardwareEnvelope << 4) | Volume);		// // //
	else {
		WriteRegister(0x400C, 0x30);
		return;
	}
	WriteRegister(0x400E, NoiseMode | Period);
	if (m_bEnvelopeLoop || m_bResetEnvelope)		// // //
		WriteRegister(0x400F, m_iLengthCounter << 3);

	m_bResetEnvelope = false;		// // //
}

void CNoiseChan::ClearRegisters()
{
	WriteRegister(0x400C, 0x30);
	WriteRegister(0x400E, 0);
	WriteRegister(0x400F, 0);
}

CString CNoiseChan::GetCustomEffectString() const		// // //
{
	CString str = _T("");

	if (!m_bEnvelopeLoop)
		str.AppendFormat(_T(" E%02X"), m_iLengthCounter);
	if (!m_bEnvelopeLoop || m_bHardwareEnvelope)
		str.AppendFormat(_T(" EE%X"), !m_bEnvelopeLoop * 2 + m_bHardwareEnvelope);

	return str;
}

int CNoiseChan::TriggerNote(int Note)
{
	RegisterKeyState(Note);
	return Note | 0x100;		// // //
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// DPCM
///////////////////////////////////////////////////////////////////////////////////////////////////////////

CDPCMChan::CDPCMChan() :		// // //
	CChannelHandler(0xF, 0x3F),		// // // does not use these anyway
	mEnabled(false),
	mTriggerSample(false),
	m_cDAC(255),
	mRetriggerPeriod(0),
	mRetriggerCtr(0)
{ 
}


void CDPCMChan::HandleNoteData(stChanNote *pNoteData, int EffColumns)
{
	m_iCustomPitch = -1;
	mRetriggerPeriod = 0;

	if (pNoteData->Note != NONE) {
		m_iNoteCut = 0;
		m_iNoteRelease = 0;			// // //
	}

	CChannelHandler::HandleNoteData(pNoteData, EffColumns);
}



// Called once per row.
bool CDPCMChan::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_DAC:
		m_cDAC = EffParam & 0x7F;
		break;
	case EF_SAMPLE_OFFSET:
		m_iOffset = EffParam & 0x3F;		// // //
		break;
	case EF_DPCM_PITCH:
		m_iCustomPitch = EffParam & 0x0F;		// // //
		break;
	case EF_RETRIGGER:
		/*
		int mRetriggerPeriod;	// If zero, DPCM will not retrigger.
		int mRetriggerCtr;		// Time until next DPCM retrigger (frames)

		queueSample() sets mRetriggerCtr to mRetriggerPeriod + 1, which gets decremented to mRetriggerPeriod
			during the same frame by RefreshChannel().
		In the absence of queueSample(), mRetriggerCtr is cycled within [1..mRetriggerPeriod] by RefreshChannel().
		If a row-note plays on any non-Xxx row, mRetriggerCtr is reset to mRetriggerPeriod=0.

		If new row:
			mRetriggerPeriod = 0.
			
			If xx = (Xxx, HandleEffect.EF_RETRIGGER):
				mRetriggerPeriod = xx
				if mRetriggerCtr == 0:	// Most recent row contains note without Xxx
					queueSample()
			
			if note: PlaySample():
				triggerSample()

		queueSample() {
			// If mRetriggerPeriod != 0, this initializes retriggering.
			// Otherwise reset mRetriggerCtr.
			if (mRetriggerPeriod == 0)
				mRetriggerCtr = 0;
			else
				mRetriggerCtr = mRetriggerPeriod + 1;	// Decremented the same frame in RefreshChannel()
		}
		triggerSample() {
			mTriggerSample = true	// play sample this frame.
			queueSample()
		}

		Each frame: RefreshChannel():	// renders channel (pushes registers to 2a03)
			// Optionally retrigger sample
			If mRetriggerPeriod:
				mRetriggerCtr--
				If zero:
					mRetriggerCtr = mRetriggerPeriod.
					mEnabled = true
					mTriggerSample = true
			
			if mTriggerSample:
				play DPCM.
		*/

		mRetriggerPeriod = std::max((int)EffParam, 1);
		if (mRetriggerCtr == 0) {	// Most recent row contains note without Xxx
			queueSample();
		}
		break;
	case EF_PHASE_RESET:
		if (EffParam == 0) {
			resetPhase();
		}
		break;
	case EF_NOTE_CUT:
	case EF_NOTE_RELEASE:
		return CChannelHandler::HandleEffect(EffNum, EffParam);
	default: return false; // unless WAVE_CHAN analog for CChannelHandler exists
	}

	return true;
}

void CDPCMChan::HandleEmptyNote()
{
}

void CDPCMChan::HandleCut()
{
//	KillChannel();
	CutNote();
}

void CDPCMChan::HandleRelease()
{
	m_bRelease = true;
}

void CDPCMChan::HandleNote(int Note, int Octave)
{
	CChannelHandler::HandleNote(Note, Octave);		// // //
	m_iNote = MIDI_NOTE(Octave, Note);		// // //
	TriggerNote(m_iNote);
	m_bGate = true;
}

bool CDPCMChan::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03:
		switch (m_iInstTypeCurrent) {
		case INST_2A03: break;
		default:
			m_pInstHandler.reset(new CInstHandlerDPCM(this));
			return true;
		}
	}
	return false;
}

void CDPCMChan::resetPhase()
{
	// Trigger the sample again
	triggerSample();
}


// Called once per row.
void CDPCMChan::PlaySample(const CDSample *pSamp, int Pitch)		// // //
{
	int SampleSize = pSamp->GetSize();
	m_pAPU->WriteSample(pSamp->GetData(), SampleSize);		// // //
	m_iPeriod = m_iCustomPitch != -1 ? m_iCustomPitch : Pitch;
	m_iSampleLength = (SampleSize >> 4) - (m_iOffset << 2);
	m_iLoopLength = SampleSize - m_iLoopOffset;
	m_iLoop = (Pitch & 0x80) >> 1;
	triggerSample();
}


void CDPCMChan::triggerSample() {
	// Trigger sample.
	mEnabled = true;
	mTriggerSample = true;

	// If mRetriggerPeriod != 0, this initializes retriggering. Otherwise reset mRetriggerCtr.
	queueSample();
}

void CDPCMChan::queueSample() {
	if (mRetriggerPeriod == 0) {
		// Not retriggering, reset mRetriggerCtr.
		mRetriggerCtr = 0;
	} else {
		// mRetriggerCtr gets decremented this frame, and reaches 0 in mRetriggerPeriod frames.
		mRetriggerCtr = mRetriggerPeriod + 1;
	}
}


// Called once per frame. Renders note to registers. Initializes playback.
void CDPCMChan::RefreshChannel()
{
	if (m_cDAC != 255) {
		WriteRegister(0x4011, m_cDAC);
		m_cDAC = 255;
	}

	if (mRetriggerPeriod != 0) {
		mRetriggerCtr--;
		if (mRetriggerCtr == 0) {
			mRetriggerCtr = mRetriggerPeriod;
			mEnabled = true;
			mTriggerSample = true;
		}
	}


	if (m_bRelease) {
		// Release command
		WriteRegister(0x4015, 0x0F);
		mEnabled = false;
		m_bRelease = false;
	}

/*	
	if (m_bRelease) {
		// Release loop flag
		m_bRelease = false;
		WriteRegister(0x4010, 0x00 | (m_iPeriod & 0x0F));
		return;
	}
*/	

	if (!mEnabled)
		return;

	if (!m_bGate) {
		// Cut sample
		WriteRegister(0x4015, 0x0F);

		if (!theApp.GetSettings()->General.bNoDPCMReset || theApp.IsPlaying()) {
			WriteRegister(0x4011, 0);	// regain full volume for TN
		}

		mEnabled = false;		// don't write to this channel anymore
	}
	else if (mTriggerSample) {
		// Start playing the sample
		WriteRegister(0x4010, (m_iPeriod & 0x0F) | m_iLoop);
		WriteRegister(0x4012, m_iOffset);							// load address, start at $C000
		WriteRegister(0x4013, m_iSampleLength);						// length
		WriteRegister(0x4015, 0x0F);
		WriteRegister(0x4015, 0x1F);								// fire sample

		// Loop offset
		if (m_iLoopOffset > 0) {
			WriteRegister(0x4012, m_iLoopOffset);
			WriteRegister(0x4013, m_iLoopLength);
		}

		mTriggerSample = false;
	}
}


int CDPCMChan::GetChannelVolume() const
{
	return VOL_COLUMN_MAX;
}

void CDPCMChan::WriteDCOffset(unsigned char Delta)		// // //
{
	// Initial delta counter value
	if (Delta != 255 && m_cDAC == 255)
		m_cDAC = Delta;
}

void CDPCMChan::SetLoopOffset(unsigned char Loop)		// // //
{
	m_iLoopOffset = Loop;
}

void CDPCMChan::ClearRegisters()
{
	WriteRegister(0x4015, 0x0F);

	WriteRegister(0x4010, 0);
	WriteRegister(0x4011, 0);
	WriteRegister(0x4012, 0);
	WriteRegister(0x4013, 0);

	m_iOffset = 0;
	m_cDAC = 255;
}

CString CDPCMChan::GetCustomEffectString() const		// // //
{
	CString str = _T("");

	if (m_iOffset)
		str.AppendFormat(_T(" Y%02X"), m_iOffset);

	return str;
}
