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
#include "FamiTrackerTypes.h"		// // //
#include "APU/Types.h"		// // //
#include "Instrument.h"		// // //
#include "ChannelHandler.h"
#include "ChannelsVRC6.h"
#include "InstHandler.h"		// // //
#include "SeqInstHandler.h"		// // //
#include "SeqInstHandlerSawtooth.h"		// // //
#include "FamiTracker.h"		// // //
#include "Settings.h"		// // //

CChannelHandlerVRC6::CChannelHandlerVRC6(int MaxPeriod, int MaxVolume) :		// // //
	CChannelHandler(MaxPeriod, MaxVolume)
{
}

bool CChannelHandlerVRC6::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_DUTY_CYCLE:
		m_iDefaultDuty = m_iDutyPeriod = EffParam;
		break;

	case EF_PHASE_RESET:
		if (EffParam == 0) {
			this->resetPhase();
			// RefreshChannel gets called afterwards, on the same frame.
			// So there is no 1-frame silent gap (verified while running at 16hz).
		}
		break;
	default: return CChannelHandler::HandleEffect(EffNum, EffParam);
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
	if (!m_bRelease)
		ReleaseNote();
}

bool CChannelHandlerVRC6::CreateInstHandler(inst_type_t Type)
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


uint16_t CChannelHandlerVRC6::getAddress() {
	return ((m_iChannelID - CHANID_VRC6_PULSE1) << 12) + 0x9000;
}

void CChannelHandlerVRC6::ClearRegisters()		// // //
{
	uint16_t Address = this->getAddress();
	WriteRegister(Address, 0);
	WriteRegister(Address + 1, 0);
	WriteRegister(Address + 2, 0);
}

void CChannelHandlerVRC6::resetPhase()		// // //
{
	uint16_t Address = this->getAddress();
	WriteRegister(Address + 2, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// // // VRC6 Squares
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Square::RefreshChannel()
{
	uint16_t Address = this->getAddress();

	unsigned int Period = CalculatePeriod();
	unsigned int Volume = CalculateVolume();
	unsigned char DutyCycle = m_iDutyPeriod << 4;

	unsigned char HiFreq = (Period & 0xFF);
	unsigned char LoFreq = (Period >> 8);
	
	if (!m_bGate) {		// // //
		WriteRegister(Address, DutyCycle);
		return;
	}

	WriteRegister(Address, DutyCycle | Volume);
	WriteRegister(Address + 1, HiFreq);
	WriteRegister(Address + 2, 0x80 | LoFreq);
}

int CVRC6Square::ConvertDuty(int Duty) const		// // //
{
	switch (m_iInstTypeCurrent) {
	case INST_2A03:	return DUTY_VRC6_FROM_2A03[Duty & 0x03];
	case INST_S5B:	return 0x07;
	default:		return Duty;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC6 Sawtooth
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC6Sawtooth::RefreshChannel()
{
	if (!m_bGate) {		// // //
		WriteRegister(0xB000, 0);
		return;
	}

	unsigned int Period = CalculatePeriod();
	unsigned int Volume = CalculateVolume();		// // //

	WriteRegister(0xB000, Volume);
	WriteRegister(0xB001, Period & 0xFF);
	WriteRegister(0xB002, 0x80 | (Period >> 8));
}

bool CVRC6Sawtooth::CreateInstHandler(inst_type_t Type)		// // //
{
	switch (Type) {
	case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS:
		switch (m_iInstTypeCurrent) {
		case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B: case INST_FDS: break;
		default:
			m_pInstHandler.reset(new CSeqInstHandlerSawtooth(this, 0x0F, Type == INST_S5B ? 0x40 : 0));
			return true;
		}
	}
	return false;
}

int CVRC6Sawtooth::CalculateVolume() const		// // //
{
	bool _64_step = false;
	if (auto pHandler = dynamic_cast<CSeqInstHandlerSawtooth*>(m_pInstHandler.get()))
		_64_step = pHandler->IsDutyIgnored();

	if (_64_step) {
		if (!theApp.GetSettings()->General.bFDSOldVolume)		// // // match NSF setting
			return LimitVolume(((m_iInstVolume + 1) * ((m_iVolume >> VOL_COLUMN_SHIFT) + 1) - 1) / 16 - GetTremolo());
		return CChannelHandler::CalculateVolume();
	}

	return (CChannelHandler::CalculateVolume() << 1) | ((m_iDutyPeriod & 0x01) << 5);
}
