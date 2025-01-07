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

#include "SoundChip.h"
#include "Channel.h"

class CSquare;		// // //

class CMMC5 : public CSoundChip {
public:
	CMMC5(CMixer *pMixer);
	virtual ~CMMC5();

	void Reset();
	void Write(uint16_t Address, uint8_t Value);
	uint8_t Read(uint16_t Address, bool &Mapped);
	void EndFrame();
	void Process(uint32_t Time);
	double GetFreq(int Channel) const override;		// // //

	void LengthCounterUpdate();
	void EnvelopeUpdate();
	void ClockSequence();		// // //

private:	
	CSquare	*m_pSquare1;
	CSquare	*m_pSquare2;
	uint8_t	*m_pEXRAM;
	uint8_t	m_iMulLow;
	uint8_t	m_iMulHigh;
};
