/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/


#pragma once

class CMixer;

//
// This class is used to derive the audio channels
//

class CChannel {
public:
	CChannel(CMixer *pMixer, uint8_t Chip, uint8_t ID) :
		m_pMixer(pMixer), m_iChip(Chip), m_iChanId(ID), m_iTime(0), m_iLastValue(0) 
	{
	}

	virtual void EndFrame() { m_iTime = 0; }

	virtual double GetFrequency() const = 0;		// // //

protected:
	virtual void Mix(int32_t Value) {
		int32_t Delta = Value - m_iLastValue;
		if (Delta)
			m_pMixer->AddValue(m_iChanId, m_iChip, Delta, Value, m_iTime);
		m_iLastValue = Value;
	}

protected:
	CMixer		*m_pMixer;			// The mixer

	uint32_t	m_iTime;			// Cycle counter, resets every new frame
	uint8_t		m_iChanId;			// This channels unique ID
	uint8_t		m_iChip;			// Chip
	int32_t		m_iLastValue;		// Last value sent to mixer
};
