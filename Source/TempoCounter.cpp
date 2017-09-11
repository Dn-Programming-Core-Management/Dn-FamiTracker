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

#include "TempoCounter.h"
#include "FamiTrackerDoc.h"
#include "ChannelState.h"

// // // CTempoCounter

CTempoCounter::CTempoCounter(const CFamiTrackerDoc &pDoc) :
	m_pDocument(&pDoc),
	m_iTempo(DEFAULT_MACHINE_TYPE == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL),
	m_iSpeed(DEFAULT_SPEED)
{
}

void CTempoCounter::AssignDocument(const CFamiTrackerDoc &pDoc) {
	m_pDocument = &pDoc;
}

void CTempoCounter::LoadTempo(unsigned Track) {
	m_iSpeed = m_pDocument->GetSongSpeed(Track);
	m_iTempo = m_pDocument->GetSongTempo(Track);
	
	m_iTempoAccum = 0;

	if (auto pGroove = m_pDocument->GetGroove(m_iSpeed); m_pDocument->GetSongGroove(Track) && pGroove) {		// // //
		LoadGroove(*pGroove);
		UpdateGrooveSpeed();
	}
	else {
		m_pCurrentGroove = nullptr;
		if (m_pDocument->GetSongGroove(Track))
			m_iSpeed = DEFAULT_SPEED;
		SetupSpeed();
	}
}

double CTempoCounter::GetTempo() const {
	auto Tempo = m_iTempo ? static_cast<double>(m_iTempo) : 2.5 * m_pDocument->GetFrameRate();		// // //
	auto Speed = m_pCurrentGroove ? m_pCurrentGroove->GetAverage() : static_cast<double>(m_iSpeed);
	return !m_iSpeed ? 0. : Tempo * 6. / Speed;
}

void CTempoCounter::Tick() {
	if (m_iTempoAccum <= 0) {
		int TicksPerSec = m_pDocument->GetFrameRate();
		m_iTempoAccum += (m_iTempo ? 60 * TicksPerSec : m_iSpeed) - m_iTempoRemainder;		// // //
	}
	m_iTempoAccum -= m_iTempoDecrement;
}

void CTempoCounter::StepRow() {
	if (m_pCurrentGroove)		// // //
		StepGroove();
}

bool CTempoCounter::CanStepRow() const {
	return m_iTempoAccum <= 0;
}

void CTempoCounter::DoFxx(uint8_t Param) {
	if (m_iTempo && Param >= m_pDocument->GetSpeedSplitPoint())		// // //
		m_iTempo = Param;
	else {		// // //
		m_iSpeed = Param;
		m_pCurrentGroove = nullptr;
	}
	SetupSpeed();
}

void CTempoCounter::DoOxx(uint8_t Param) {
	// currently does not support starting at arbitrary index of a groove
	if (auto pGroove = m_pDocument->GetGroove(Param)) {
		LoadGroove(*pGroove);
		StepGroove();
	}
}

void CTempoCounter::LoadSoundState(const stFullState &state) {
	if (state.Tempo != -1)
		m_iTempo = state.Tempo;
	if (state.GroovePos >= 0) {
		if (auto pGroove = m_pDocument->GetGroove(state.Speed)) {
			LoadGroove(*pGroove);
			m_iGroovePosition = state.GroovePos;
			UpdateGrooveSpeed();
		}
	}
	else {
		if (state.Speed >= 0)
			m_iSpeed = state.Speed;
		m_pCurrentGroove = nullptr;
	}
	SetupSpeed();
}

void CTempoCounter::SetupSpeed() {
	if (m_iTempo) {		// // //
		m_iTempoDecrement = (m_iTempo * 24) / m_iSpeed;
		m_iTempoRemainder = (m_iTempo * 24) % m_iSpeed;
	}
	else {
		m_iTempoDecrement = 1;
		m_iTempoRemainder = 0;
	}
}

void CTempoCounter::LoadGroove(const CGroove &Groove) {
	m_pCurrentGroove = &Groove;
	m_iGroovePosition = 0;
}

void CTempoCounter::UpdateGrooveSpeed() {
	m_iSpeed = m_pCurrentGroove->GetEntry(m_iGroovePosition);
	SetupSpeed();
}

void CTempoCounter::StepGroove() {
	UpdateGrooveSpeed();
	++m_iGroovePosition %= m_pCurrentGroove->GetSize();
}
