/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "ChannelsS5B.h"
#include "APU/Types.h"		// // //
#include "Sequence.h"		// // //
#include "APU/APU.h"
#include "InstHandler.h"		// // //
#include "SeqInstHandlerS5B.h"		// // //

// Static member variables, for the shared stuff in 5B
int			  CChannelHandlerS5B::m_iModes		= 0;
int			  CChannelHandlerS5B::m_iNoiseFreq	= 0;
int			  CChannelHandlerS5B::m_iNoisePrev	= -1;
int			  CChannelHandlerS5B::m_iDefaultNoise = 0;		// // //
unsigned char CChannelHandlerS5B::m_iEnvFreqHi	= 0;
unsigned char CChannelHandlerS5B::m_iEnvFreqLo	= 0;
bool		  CChannelHandlerS5B::m_bEnvTrigger	= false;		// // // 050B
int			  CChannelHandlerS5B::m_iEnvType	= 0;
int			  CChannelHandlerS5B::m_i5808B4		= 0;		// // // 050B

// Class functions

void CChannelHandlerS5B::SetMode(int Chan, int Square, int Noise)
{
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
}

void CChannelHandlerS5B::UpdateAutoEnvelope(int Period)		// // // 050B
{
	if (m_bEnvelopeEnabled && m_iAutoEnvelopeShift) {
		if (m_iAutoEnvelopeShift > 8) {
			Period >>= m_iAutoEnvelopeShift - 8 - 1;		// // // round off
			if (Period & 0x01) ++Period;
			Period >>= 1;
		}
		else if (m_iAutoEnvelopeShift < 8)
			Period <<= 8 - m_iAutoEnvelopeShift;
		m_iEnvFreqLo = Period & 0xFF;
		m_iEnvFreqHi = Period >> 8;
	}
}

void CChannelHandlerS5B::UpdateRegs()		// // //
{
	// Done only once
	if (m_iNoiseFreq != m_iNoisePrev)		// // //
		WriteReg(0x06, (m_iNoisePrev = m_iNoiseFreq) ^ 0x1F);
	WriteReg(0x07, m_iModes);
	WriteReg(0x0B, m_iEnvFreqLo);
	WriteReg(0x0C, m_iEnvFreqHi);
	if (m_bEnvTrigger)		// // // 050B
		WriteReg(0x0D, m_iEnvType);
	m_bEnvTrigger = false;
}

// Instance functions

CChannelHandlerS5B::CChannelHandlerS5B() : 
	CChannelHandler(0xFFF, 0x0F),
	m_bEnvelopeEnabled(false),		// // // 050B
	m_iAutoEnvelopeShift(0),		// // // 050B
	m_bUpdate(false)
{
	m_iDefaultDuty = S5B_MODE_SQUARE;		// // //
	m_iDefaultNoise = 0;		// // //
}

bool CChannelHandlerS5B::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_SUNSOFT_NOISE: // W
		m_iDefaultNoise = m_iNoiseFreq = EffParam & 0x1F;		// // // 050B
		break;
	case EF_SUNSOFT_ENV_HI: // I
		m_iEnvFreqHi = EffParam;
		break;
	case EF_SUNSOFT_ENV_LO: // J
		m_iEnvFreqLo = EffParam;
		break;
	case EF_SUNSOFT_ENV_TYPE: // H
		m_bEnvTrigger = true;		// // // 050B
		m_iEnvType = EffParam & 0x0F;
		m_bUpdate = true;
		m_bEnvelopeEnabled = EffParam != 0;
		m_iAutoEnvelopeShift = EffParam >> 4;
		break;
	case EF_DUTY_CYCLE:
		m_iDefaultDuty = m_iDutyPeriod = (EffParam << 6) | ((EffParam & 0x04) << 3);		// // // 050B
//		m_iDefaultDuty = m_iDutyPeriod = EffParam;		// // //
		break;
	default: return CChannelHandler::HandleEffect(EffNum, EffParam);
	}

	return true;
}

void CChannelHandlerS5B::HandleNote(int Note, int Octave)		// // //
{
	CChannelHandler::HandleNote(Note, Octave);
	m_iNoiseFreq = m_iDefaultNoise;
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
	if (!m_bRelease)
		ReleaseNote();		// // //
}

bool CChannelHandlerS5B::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS:
		switch (m_iInstTypeCurrent) {
		case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS: break;
		default:
			m_pInstHandler.reset(new CSeqInstHandlerS5B(this, 0x0F, Type == INST_S5B ? 0x40 : 0));
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

	m_iDefaultDuty = m_iDutyPeriod = S5B_MODE_SQUARE;
	m_iDefaultNoise = m_iNoiseFreq = 0;		// // //
	m_iNoisePrev = -1;		// // //
	m_bEnvelopeEnabled = false;
	m_iAutoEnvelopeShift = 0;
	m_iEnvFreqHi = 0;
	m_iEnvFreqLo = 0;
	m_iEnvType = 0;
	m_i5808B4 = 0;		// // // 050B
	m_bEnvTrigger = false;
}

int CChannelHandlerS5B::CalculateVolume() const		// // //
{
	return LimitVolume((m_iVolume >> VOL_COLUMN_SHIFT) + m_iInstVolume - 15 - GetTremolo());
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
	WriteReg(8 + m_iChannelID - CHANID_S5B_CH1, 0);		// Clear volume
}

std::string CChannelHandlerS5B::GetCustomEffectString() const		// // //
{
	std::string str;

	if (m_iEnvFreqLo)
		str += MakeCommandString(EF_SUNSOFT_ENV_LO, m_iEnvFreqLo);
	if (m_iEnvFreqHi)
		str += MakeCommandString(EF_SUNSOFT_ENV_HI, m_iEnvFreqHi);
	if (m_iEnvType)
		str += MakeCommandString(EF_SUNSOFT_ENV_TYPE, m_iEnvType);
	if (m_iDefaultNoise)
		str += MakeCommandString(EF_SUNSOFT_NOISE, m_iDefaultNoise);

	return str;
}

void CChannelHandlerS5B::RefreshChannel()
{
	int Period = CalculatePeriod();
	unsigned char LoPeriod = Period & 0xFF;
	unsigned char HiPeriod = Period >> 8;
	int Volume = CalculateVolume();

	unsigned char Noise = (m_bGate && (m_iDutyPeriod & S5B_MODE_NOISE)) ? 0 : 1;
	unsigned char Square = (m_bGate && (m_iDutyPeriod & S5B_MODE_SQUARE)) ? 0 : 1;
	unsigned char Envelope = (m_bGate && (m_iDutyPeriod & S5B_MODE_ENVELOPE)) ? 0x10 : 0; // m_bEnvelopeEnabled ? 0x10 : 0;

	UpdateAutoEnvelope(Period);		// // // 050B
	SetMode(m_iChannelID, Square, Noise);
	
	WriteReg((m_iChannelID - CHANID_S5B_CH1) * 2    , LoPeriod);
	WriteReg((m_iChannelID - CHANID_S5B_CH1) * 2 + 1, HiPeriod);
	WriteReg((m_iChannelID - CHANID_S5B_CH1) + 8    , Volume | Envelope);

	if (Envelope && (m_bTrigger || m_bUpdate))		// // // 050B
		m_bEnvTrigger = true;
	m_bUpdate = false;

	if (m_iChannelID == CHANID_S5B_CH3)
		UpdateRegs();
}

void CChannelHandlerS5B::SetNoiseFreq(int Pitch)		// // //
{
	m_iNoiseFreq = Pitch;
}
