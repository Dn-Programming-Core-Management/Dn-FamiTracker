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
#include "FamiTracker.h"
#include "FamiTrackerTypes.h"		// // //
#include "APU/Types.h"		// // //
#include "Instrument.h"		// // //
#include "SeqInstrument.h"		// // //
#include "InstrumentN163.h"		// // // constants
#include "ChannelHandler.h"
#include "ChannelsN163.h"
#include "InstHandler.h"		// // //
#include "SeqInstHandler.h"		// // //
#include "SeqInstHandlerN163.h"		// // //

const int N163_PITCH_SLIDE_SHIFT = 2;	// Increase amplitude of pitch slides

CChannelHandlerN163::CChannelHandlerN163() : 
	FrequencyChannelHandler(0xFFFF, 0x0F),
	m_bDisableLoad(false),		// // //
	m_bResetPhase(false),
	m_iWaveLen(4),		// // //
	m_iWaveCount(0)
{
	m_iDutyPeriod = 0;
}

void CChannelHandlerN163::ResetChannel()
{
	CChannelHandler::ResetChannel();

	m_iWavePos = m_iWavePosOld = 0;		// // //
	m_iWaveLen = 4;
	m_bLoadWave = false;
}

bool CChannelHandlerN163::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_PORTA_DOWN:
		m_iPortaSpeed = EffParam;
		if (!m_bLinearPitch) m_iPortaSpeed <<= N163_PITCH_SLIDE_SHIFT;		// // //
		m_iEffectParam = EffParam;
		m_iEffect = m_bLinearPitch ? EF_PORTA_DOWN : EF_PORTA_UP;
		break;
	case EF_PORTA_UP:
		m_iPortaSpeed = EffParam;
		if (!m_bLinearPitch) m_iPortaSpeed <<= N163_PITCH_SLIDE_SHIFT;		// // //
		m_iEffectParam = EffParam;
		m_iEffect = m_bLinearPitch ? EF_PORTA_UP : EF_PORTA_DOWN;
		break;
	case EF_DUTY_CYCLE:
		// Duty effect controls wave
		m_iDefaultDuty = m_iDutyPeriod = EffParam;
		m_bLoadWave = true;
		if (auto pHandler = dynamic_cast<CSeqInstHandlerN163*>(m_pInstHandler.get()))
			pHandler->RequestWaveUpdate();
		break;
	case EF_N163_WAVE_BUFFER:		// // //
		if (EffParam == 0x7F) {
			m_iWavePos = m_iWavePosOld;
			m_bDisableLoad = false;
		}
		else {
			m_iWavePos = EffParam << 1;
			m_bDisableLoad = true;
		}
		if (auto pHandler = dynamic_cast<CSeqInstHandlerN163*>(m_pInstHandler.get()))
			pHandler->RequestWaveUpdate();
		break;
	default: return FrequencyChannelHandler::HandleEffect(EffNum, EffParam);
	}

	return true;
}

bool CChannelHandlerN163::HandleInstrument(bool Trigger, bool NewInstrument)
{
	if (!CChannelHandler::HandleInstrument(Trigger, NewInstrument))		// // //
		return false;

	if (!m_bLoadWave && NewInstrument)
		m_iDefaultDuty = 0;

	if (!m_bDisableLoad) {
		m_iWavePos = /*pInstrument->GetAutoWavePos() ? GetIndex() * 16 :*/ m_iWavePosOld;
	}

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
	if (!m_bRelease)
		ReleaseNote();
}

void CChannelHandlerN163::HandleNote(int Note, int Octave)
{
	// New note
	CChannelHandler::HandleNote(Note, Octave);		// // //
	m_bLoadWave = false;
//	m_bResetPhase = true;
}

bool CChannelHandlerN163::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_S5B: case INST_FDS:
		switch (m_iInstTypeCurrent) {
		case INST_2A03: case INST_VRC6: case INST_S5B: case INST_FDS: break;
		default:
			m_pInstHandler.reset(new CSeqInstHandler(this, 0x0F, Type == INST_S5B ? 0x40 : 0));
			return true;
		}
		break;
	case INST_N163:
		switch (m_iInstTypeCurrent) {
		case INST_N163: break;
		default:
			m_pInstHandler.reset(new CSeqInstHandlerN163(this, 0x0F, 0));
			return true;
		}
	}
	return false;
}

void CChannelHandlerN163::SetupSlide()		// // //
{
	CChannelHandler::SetupSlide();
	if (!m_bLinearPitch) m_iPortaSpeed <<= N163_PITCH_SLIDE_SHIFT;		// // //
}

void CChannelHandlerN163::RefreshChannel()
{
	int Channel = 7 - GetIndex();		// Channel #
	int WaveSize = 256 - (m_iWaveLen >> 2);
	int Frequency = CalculatePeriod();		// // //

	// Compensate for shorter waves
//	Frequency >>= 5 - int(log(double(m_iWaveLen)) / log(2.0));

	int Volume = CalculateVolume();
	int ChannelAddrBase = 0x40 + Channel * 8;

	if (!m_bGate)
		Volume = 0;

	// Update channel
	if (Channel + m_iChannels >= 8) {		// // //
		WriteData(ChannelAddrBase + 7, ((m_iChannels - 1) << 4) | Volume);
		if (!m_bGate)
			return;
		WriteData(ChannelAddrBase + 0, Frequency & 0xFF);
		WriteData(ChannelAddrBase + 2, (Frequency >> 8) & 0xFF);
		WriteData(ChannelAddrBase + 4, (WaveSize << 2) | ((Frequency >> 16) & 0x03));
		WriteData(ChannelAddrBase + 6, m_iWavePos);
	}

	if (m_bResetPhase) {
		m_bResetPhase = false;
		WriteData(ChannelAddrBase + 1, 0);
		WriteData(ChannelAddrBase + 3, 0);
		WriteData(ChannelAddrBase + 5, 0);
	}
}

void CChannelHandlerN163::SetWaveLength(int Length)		// // //
{
	ASSERT(Length >= 4 && Length <= CInstrumentN163::MAX_WAVE_SIZE && !(Length % 4));
	m_iWaveLen = Length;
}

void CChannelHandlerN163::SetWavePosition(int Pos)		// // //
{
	ASSERT(Pos >= 0 && Pos <= 0xFF);
	m_iWavePosOld = Pos;
}

void CChannelHandlerN163::SetWaveCount(int Count)		// // //
{
	ASSERT(Count > 0 && Count <= CInstrumentN163::MAX_WAVE_COUNT);
	m_iWaveCount = Count;
}

void CChannelHandlerN163::FillWaveRAM(const char *Buffer, int Count)		// // //
{
	SetAddress(m_iWavePos >> 1, true);
	for (int i = 0; i < Count; ++i)
		WriteData(Buffer[i]);
}

void CChannelHandlerN163::SetChannelCount(int Count)		// // //
{
	m_iChannels = Count;
}

int CChannelHandlerN163::getDutyMax() const {
	return CInstrumentN163::MAX_WAVE_COUNT - 1;
}

int CChannelHandlerN163::ConvertDuty(int Duty) const		// // //
{
	switch (m_iInstTypeCurrent) {
	case INST_2A03: case INST_VRC6: case INST_S5B:
		return -1;
	default:
		return Duty;
	}
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
		WriteReg(ChannelAddrBase + 7, (m_iChannels - 1) << 4);

	m_bDisableLoad = false;		// // //
	m_iDutyPeriod = 0;
}

int CChannelHandlerN163::CalculatePeriod(bool MultiplyByHarmonic) const		// // //
{
	int Detune = GetVibrato() - GetFinePitch() - GetPitch();
	int Frequency;		// // //
	if (m_bLinearPitch && m_pNoteLookupTable != nullptr) {
		Frequency = LimitPeriod(GetPeriod() + Detune);		// // //
		int Note = Frequency >> LINEAR_PITCH_AMOUNT;
		int Sub = Frequency % (1 << LINEAR_PITCH_AMOUNT);
		int Offset = Note < NOTE_COUNT - 1 ? m_pNoteLookupTable[Note + 1] - m_pNoteLookupTable[Note] : 0;
		Offset = Offset * Sub >> LINEAR_PITCH_AMOUNT;
		if (Sub && !Offset) Offset = 1;
		Frequency = m_pNoteLookupTable[Note] + Offset;
	}
	else {
		Frequency = GetPeriod() + (Detune << 4);
	}
	if (MultiplyByHarmonic) {
		Frequency *= m_iHarmonic;
	}
	return LimitRawPeriod(Frequency) << N163_PITCH_SLIDE_SHIFT;
}

CString CChannelHandlerN163::GetSlideEffectString() const		// // //
{
	CString str = _T("");
	
	switch (m_iEffect) {
	case EF_ARPEGGIO:
		if (m_iEffectParam) str.AppendFormat(_T(" %c%02X"), EFF_CHAR[m_iEffect], m_iEffectParam); break;
	case EF_PORTA_UP:
		if (m_iPortaSpeed) str.AppendFormat(_T(" %c%02X"), EFF_CHAR[EF_PORTA_DOWN], m_iPortaSpeed >> N163_PITCH_SLIDE_SHIFT); break;
	case EF_PORTA_DOWN:
		if (m_iPortaSpeed) str.AppendFormat(_T(" %c%02X"), EFF_CHAR[EF_PORTA_UP], m_iPortaSpeed >> N163_PITCH_SLIDE_SHIFT); break;
	case EF_PORTAMENTO:
		if (m_iPortaSpeed) str.AppendFormat(_T(" %c%02X"), EFF_CHAR[m_iEffect], m_iPortaSpeed >> N163_PITCH_SLIDE_SHIFT); break;
	}

	return str;
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
	WriteRegister(0xF800, Reg);
	WriteRegister(0x4800, Value);
}

void CChannelHandlerN163::SetAddress(char Addr, bool AutoInc)
{
	WriteRegister(0xF800, (AutoInc ? 0x80 : 0) | Addr);
}

void CChannelHandlerN163::WriteData(char Data)
{
	WriteRegister(0x4800, Data);
}

void CChannelHandlerN163::WriteData(int Addr, char Data)
{
	SetAddress(Addr, false);
	WriteData(Data);
}
