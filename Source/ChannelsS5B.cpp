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

// Sunsoft 5B (YM2149/AY-3-8910)

#include "stdafx.h"
#include "FamiTrackerTypes.h"		// // //
#include "APU/Types.h"		// // //
#include "Sequence.h"		// // //
#include "Instrument.h"		// // //
#include "ChannelHandler.h"
#include "ChannelsS5B.h"
#include "APU/APU.h"
#include "InstHandler.h"		// // //
#include "SeqInstHandler.h"		// // //

// Static member variables, for the shared stuff in 5B
int			  CChannelHandlerS5B::m_iModes		= 0;
int			  CChannelHandlerS5B::m_iNoiseFreq	= 0;
unsigned char CChannelHandlerS5B::m_iEnvFreqHi	= 0;
unsigned char CChannelHandlerS5B::m_iEnvFreqLo	= 0;
int			  CChannelHandlerS5B::m_iEnvType	= 0;
bool		  CChannelHandlerS5B::m_bRegsDirty	= false;

// Class functions

void CChannelHandlerS5B::SetEnvelopeHigh(int Val)
{
	m_iEnvFreqHi = Val;
	m_bRegsDirty = true;
}

void CChannelHandlerS5B::SetEnvelopeLow(int Val)
{
	m_iEnvFreqLo = Val;
	m_bRegsDirty = true;
}

void CChannelHandlerS5B::SetEnvelopeType(int Val)
{
	m_iEnvType = Val;
	m_bRegsDirty = true;
}

void CChannelHandlerS5B::SetMode(int Chan, int Square, int Noise)
{
	int initModes = m_iModes;

	Chan -= CHANID_S5B_CH1;

	switch (Chan) {
		case 0:
			m_iModes &= 0x36;
			break;
		case 1:
			m_iModes &= 0x2D;
			break;
		case 2:
			m_iModes &= 0x1B;
			break;
	}

	m_iModes |= (Noise << (3 + Chan)) | (Square << Chan);
	
	if (m_iModes != initModes) {
		m_bRegsDirty = true;
	}
}

void CChannelHandlerS5B::SetNoiseFreq(int Freq)
{
	m_iNoiseFreq = Freq;
	m_bRegsDirty = true;
}

void CChannelHandlerS5B::UpdateRegs(CAPU *pAPU)
{
	if (!m_bRegsDirty)
		return;

	// Done only once
	WriteRegister(0xC000, 0x07);
	WriteRegister(0xE000, m_iModes);

	WriteRegister(0xC000, 0x06);
	WriteRegister(0xE000, m_iNoiseFreq);

	WriteRegister(0xC000, 0x0B);
	WriteRegister(0xE000, m_iEnvFreqLo);

	WriteRegister(0xC000, 0x0C);
	WriteRegister(0xE000, m_iEnvFreqHi);

	WriteRegister(0xC000, 0x0D);
	WriteRegister(0xE000, m_iEnvType);

	m_bRegsDirty = false;
}

// Instance functions

CChannelHandlerS5B::CChannelHandlerS5B() : 
	CChannelHandler(0xFFF, 0x0F), 
	m_iNoiseOffset(0), 
	m_bUpdate(false)
{
	m_iDefaultDuty = S5B_MODE_SQUARE;		// // //
}

/*
bool NoteValid(int Note)
{
	return (Note != NONE && Note != HALT && Note != RELEASE);
}
*/

bool CChannelHandlerS5B::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_SUNSOFT_ENV_HI: // I
		SetEnvelopeHigh(EffParam);
		break;
	case EF_SUNSOFT_ENV_LO: // H
		SetEnvelopeLow(EffParam);
		break;
	case EF_SUNSOFT_ENV_TYPE: // J
		SetEnvelopeType(EffParam);
		//m_bEnvEnable = true;
		//m_bUpdate = true;
		break;
	case EF_DUTY_CYCLE:
		m_iDefaultDuty = m_iDutyPeriod = EffParam;
		break;
	default: return CChannelHandler::HandleEffect(EffNum, EffParam);
	}

	return true;
}

void CChannelHandlerS5B::HandleEmptyNote()
{
}

void CChannelHandlerS5B::HandleCut()
{
	CutNote();
	m_iDutyPeriod = S5B_MODE_SQUARE;
	m_iNote = 0;
}

void CChannelHandlerS5B::HandleRelease()
{
	if (!m_bRelease) {
		ReleaseNote();
		m_bUpdate = true;
	}
}

void CChannelHandlerS5B::HandleNote(int Note, int Octave)
{
	m_bUpdate = true;
}

bool CChannelHandlerS5B::CreateInstHandler(inst_type_t Type)
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

void CChannelHandlerS5B::WriteReg(int Reg, int Value)
{
	WriteRegister(0xC000, Reg);
	WriteRegister(0xE000, Value);
}

void CChannelHandlerS5B::ResetChannel()
{
	CChannelHandler::ResetChannel();

	m_iDefaultDuty = S5B_MODE_SQUARE;
	m_iNoiseFreq = 0;
	m_iEnvFreqHi = 0;
	m_iEnvFreqLo = 0;
	m_iEnvType = 0;
}

int CChannelHandlerS5B::ConvertDuty(int Duty) const		// // //
{
	switch (m_iInstTypeCurrent) {
	case INST_2A03: case INST_VRC6: case INST_N163:
		return S5B_MODE_SQUARE;
	default:
		return Duty;
	}
}

void CChannelHandlerS5B::ClearRegisters()
{
	SetMode(m_iChannelID, 1, 1);
	WriteReg(8 + m_iChannelID - CHANID_S5B_CH1, 0);		// Clear volume
}

CString CChannelHandlerS5B::GetCustomEffectString() const		// // //
{
	CString str = _T("");

	if (m_iEnvFreqLo)
		str.AppendFormat(_T(" H%02X"), m_iEnvFreqLo);
	if (m_iEnvFreqHi)
		str.AppendFormat(_T(" I%02X"), m_iEnvFreqHi);
	if (m_iEnvType)
		str.AppendFormat(_T(" J%02X"), m_iEnvType);

	return str;
}

void CChannelHandlerS5B::RefreshChannel()
{
	int Period = CalculatePeriod();
	unsigned char LoPeriod = Period & 0xFF;
	unsigned char HiPeriod = Period >> 8;
	int Volume = CalculateVolume(true);		// // //

	unsigned char Noise = (m_iDutyPeriod & S5B_MODE_NOISE) ? 0 : 1;
	unsigned char Square = (m_iDutyPeriod & S5B_MODE_SQUARE) ? 0 : 1;
	unsigned char Envelope = (m_iDutyPeriod & S5B_MODE_ENVELOPE) ? 0 : 1;
	unsigned char NoisePeriod = (m_iDutyPeriod + m_iNoiseOffset) & 0x1F;

	if (!m_bGate) {
		Noise = Square = Envelope = 1;
	}

	SetMode(m_iChannelID, Square, Noise);
	
	WriteReg((m_iChannelID - CHANID_S5B_CH1) + 8    , Volume | ((1 - Envelope) << 4));
	if (m_bGate) {
		WriteReg((m_iChannelID - CHANID_S5B_CH1) * 2    , LoPeriod);
		WriteReg((m_iChannelID - CHANID_S5B_CH1) * 2 + 1, HiPeriod);
	}
	if (!Noise)
		SetNoiseFreq(NoisePeriod);

	if (m_iChannelID == CHANID_S5B_CH3)
		UpdateRegs(m_pAPU);
}