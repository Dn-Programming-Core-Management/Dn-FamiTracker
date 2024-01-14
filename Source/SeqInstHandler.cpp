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

#include "stdafx.h"
#include "FamiTracker.h" // theApp.getSoundGenerator()
#include "APU/Types.h"
#include "FamiTrackerTypes.h"
#include "SoundGen.h"

#include "Instrument.h"
#include "SeqInstrument.h"
#include "Sequence.h"
#include "ChannelHandlerInterface.h"
#include "SeqInstHandler.h"

/*
 * Class CSeqInstHandler
 */

CSeqInstHandler::CSeqInstHandler(CChannelHandlerInterface *pInterface, int Vol, int Duty) :
	CInstHandler(pInterface, Vol),
	m_iDefaultDuty(Duty)
{
	for (std::size_t i = 0; i < sizeof(m_pSequence) / sizeof(CSequence*); i++)
		ClearSequence(static_cast<int>(i));
}

void CSeqInstHandler::LoadInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = pInst;
	auto pSeqInst = std::dynamic_pointer_cast<CSeqInstrument>(pInst);
	if (pSeqInst == nullptr) return;
	for (std::size_t i = 0; i < sizeof(m_pSequence) / sizeof(CSequence*); i++) {
		const CSequence *pSequence = pSeqInst->GetSequence(static_cast<int>(i));
		bool Enable = pSeqInst->GetSeqEnable(static_cast<int>(i)) == SEQ_STATE_RUNNING;
		if (!Enable)
			ClearSequence(static_cast<int>(i));
		else if (pSequence != m_pSequence[i] || m_iSeqState[i] == SEQ_STATE_DISABLED)
			SetupSequence(static_cast<int>(i), pSequence);
	}
}

void CSeqInstHandler::TriggerInstrument()
{
	for (std::size_t i = 0; i < sizeof(m_pSequence) / sizeof(CInstrument*); i++) if (m_pSequence[i] != nullptr) {
		m_iSeqState[i] = SEQ_STATE_RUNNING;
		m_iSeqPointer[i] = 0;
	}
	m_iVolume = m_iDefaultVolume;
	m_iNoteOffset = 0;
	m_iPitchOffset = 0;
	m_iDutyParam = m_iDefaultDuty;
	
	if (m_pInterface->IsActive()) {
		m_pInterface->SetVolume(m_iDefaultVolume);
//		m_pInterface->SetDutyPeriod(m_iDefaultDuty); // not same
	}
}

void CSeqInstHandler::ReleaseInstrument()
{
	if (m_pInterface->IsReleasing()) return;
	for (size_t i = 0; i < sizeof(m_pSequence) / sizeof(CInstrument*); i++)
		if (m_pSequence[i] != nullptr && (m_iSeqState[i] == SEQ_STATE_RUNNING || m_iSeqState[i] == SEQ_STATE_END)) {
			int ReleasePoint = m_pSequence[i]->GetReleasePoint();
			if (ReleasePoint != -1) {
				m_iSeqPointer[i] = ReleasePoint;
				m_iSeqState[i] = SEQ_STATE_RUNNING;
			}
		}
}

void CSeqInstHandler::UpdateInstrument()
{
	if (!m_pInterface->IsActive()) return;
	for (std::size_t i = 0; i < sizeof(m_pSequence) / sizeof(CSequence*); i++) {
		if (m_pSequence[i] == nullptr || m_pSequence[i]->GetItemCount() == 0) continue;
		int Value;
		switch (m_iSeqState[i]) {
		case SEQ_STATE_RUNNING:
			Value = m_pSequence[i]->GetItem(m_iSeqPointer[i]);
			ProcessSequence(static_cast<int>(i), m_pSequence[i]->GetSetting(), Value);
			++m_iSeqPointer[i];

			{	
				int Release = m_pSequence[i]->GetReleasePoint();
				int Items = m_pSequence[i]->GetItemCount();
				int Loop = m_pSequence[i]->GetLoopPoint();
				if (m_iSeqPointer[i] == (Release + 1) || m_iSeqPointer[i] >= Items) {
					// End point reached
					if (Loop != -1 && !(m_pInterface->IsReleasing() && Release != -1) && Loop < Release) {		// // //
						m_iSeqPointer[i] = Loop;
					}
					else if (m_iSeqPointer[i] >= Items) {
						// End of sequence 
						if (Loop >= Release && Loop != -1)		// // //
							m_iSeqPointer[i] = Loop;
						else
							m_iSeqState[i] = SEQ_STATE_END;
					}
					else if (!m_pInterface->IsReleasing()) {
						// Waiting for release
						--m_iSeqPointer[i];
					}
				}
				theApp.GetSoundGenerator()->SetSequencePlayPos(m_pSequence[i], m_iSeqPointer[i]);
			}
			break;

		case SEQ_STATE_END:
			switch (i) {
			case SEQ_ARPEGGIO:
				if (m_pSequence[i]->GetSetting() == SETTING_ARP_FIXED)
					m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote()));
				break;
			}
			m_iSeqState[i] = SEQ_STATE_HALT;
			theApp.GetSoundGenerator()->SetSequencePlayPos(m_pSequence[i], -1);
			break;

		case SEQ_STATE_HALT:
		case SEQ_STATE_DISABLED:
			break;
		}
	}
}

bool CSeqInstHandler::ProcessSequence(int Index, unsigned Setting, int Value)
{
	switch (Index) {
	// Volume modifier
	case SEQ_VOLUME:
		m_pInterface->SetVolume(Value);
		return true;
	// Arpeggiator
	case SEQ_ARPEGGIO:
		switch (Setting) {
		case SETTING_ARP_ABSOLUTE:
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote() + Value));
			return true;
		case SETTING_ARP_FIXED:
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(Value));
			return true;
		case SETTING_ARP_RELATIVE:
			m_pInterface->SetNote(m_pInterface->GetNote() + Value);
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote()));
			return true;
		case SETTING_ARP_SCHEME: // // //
			if (Value < 0) Value += 256;
			int lim = Value % 0x40, scheme = Value / 0x40;
			if (lim > ARPSCHEME_MAX)
				lim -= 64;
			{
				unsigned char Param = m_pInterface->GetArpParam();
				switch (scheme) {
				case 0: break;
				case 1: lim += Param >> 4;   break;
				case 2: lim += Param & 0x0F; break;
				case 3: lim -= Param & 0x0F; break;
				}
			}
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote() + lim));
			return true;
		}
		return false;
	// Pitch
	case SEQ_PITCH:
		switch (Setting) {		// // //
		case SETTING_PITCH_RELATIVE:
			m_pInterface->SetPeriod(m_pInterface->GetPeriod() + Value);
			return true;
		case SETTING_PITCH_ABSOLUTE:		// // // 050B
			m_pInterface->SetPeriod(m_pInterface->TriggerNote(m_pInterface->GetNote()) + Value);
			return true;
		}
		return false;
	// Hi-pitch
	case SEQ_HIPITCH:
		m_pInterface->SetPeriod(m_pInterface->GetPeriod() + (Value << 4));
		return true;
	// Duty cycling
	case SEQ_DUTYCYCLE:
		m_pInterface->SetDutyPeriod(Value);
		return true;
	}
	return false;
}

void CSeqInstHandler::SetupSequence(int Index, const CSequence *pSequence)
{
	m_iSeqState[Index]	 = SEQ_STATE_RUNNING;
	m_iSeqPointer[Index] = 0;
	m_pSequence[Index]	 = pSequence;
}

void CSeqInstHandler::ClearSequence(int Index)
{
	m_iSeqState[Index]	 = SEQ_STATE_DISABLED;
	m_iSeqPointer[Index] = 0;
	m_pSequence[Index]	 = nullptr;
}
