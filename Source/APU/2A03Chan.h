/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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
