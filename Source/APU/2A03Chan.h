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

#include "Channel.h"

class C2A03Chan : public CChannel {		// // //
public:
	C2A03Chan(CMixer *pMixer, uint8_t Chip, uint8_t ID) : CChannel(pMixer, Chip, ID) { }

	inline uint16_t GetPeriod() const {
		return m_iPeriod;
	}

protected:
	inline void Mix(int32_t Value) {
		if (m_iLastValue != Value) {
			m_pMixer->AddValue(m_iChanId, m_iChip, Value, Value, m_iTime);
			m_iLastValue = Value;
		}
	};

protected:
	// Variables used by channels
	uint8_t		m_iControlReg;
	uint8_t		m_iEnabled;
	uint16_t	m_iPeriod;
	uint16_t	m_iLengthCounter;
	uint32_t	m_iCounter;
};
