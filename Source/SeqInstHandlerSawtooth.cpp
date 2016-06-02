/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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
#include "Sequence.h"
#include "ChannelHandlerInterface.h"
#include "SeqInstHandlerSawtooth.h"

CSeqInstHandlerSawtooth::CSeqInstHandlerSawtooth(CChannelHandlerInterface *pInterface, int Vol, int Duty) :
	CSeqInstHandler(pInterface, Vol, Duty), m_bIgnoreDuty(false)
{
}

void CSeqInstHandlerSawtooth::TriggerInstrument()
{
	CSeqInstHandler::TriggerInstrument();
	m_bIgnoreDuty = m_pSequence[SEQ_VOLUME] != nullptr &&
					m_pSequence[SEQ_VOLUME]->GetSetting() == SETTING_VOL_64_STEPS;
}

bool CSeqInstHandlerSawtooth::IsDutyIgnored() const
{
	return m_bIgnoreDuty;
}

bool CSeqInstHandlerSawtooth::ProcessSequence(int Index, unsigned Setting, int Value)
{
	switch (Index) {
	case SEQ_VOLUME:
		switch (Setting) {
		case SETTING_VOL_16_STEPS:
			m_pInterface->SetVolume(((Value & 0x0F) << 1) | ((m_pInterface->GetDutyPeriod() & 0x01) << 5));
			return true;
		case SETTING_VOL_64_STEPS:
			m_pInterface->SetVolume(Value);
			return true;
		}
		return false;
	case SEQ_DUTYCYCLE:
		if (!m_bIgnoreDuty)
			m_pInterface->SetVolume((m_pInterface->GetVolume() & 0x1F) | ((Value & 0x01) << 5));
		return true;
	}
	return CSeqInstHandler::ProcessSequence(Index, Setting, Value);
}
