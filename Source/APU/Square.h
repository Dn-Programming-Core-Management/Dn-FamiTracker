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

/*
 * Square wave
 *
 */


#pragma once

#include "2A03Chan.h"		// // //

class CSquare : public C2A03Chan {
public:
	CSquare(CMixer *pMixer, int ID, int Chip);
	~CSquare();

	void	Reset();
	void	Write(uint16_t Address, uint8_t Value);
	void	WriteControl(uint8_t Value);
	uint8_t	ReadControl();
	void	Process(uint32_t Time);
	double	GetFrequency() const override;		// // //

	void	LengthCounterUpdate();
	void	SweepUpdate(int Diff);
	void	EnvelopeUpdate();

public:
	static const uint8_t DUTY_TABLE[4][16];
	uint32_t CPU_RATE;		// // //

private:
	uint8_t	m_iDutyLength, m_iDutyCycle;

	uint8_t	m_iLooping, m_iEnvelopeFix, m_iEnvelopeSpeed;
	uint8_t	m_iEnvelopeVolume, m_iFixedVolume;
	int8_t	m_iEnvelopeCounter;

	uint8_t	m_iSweepEnabled, m_iSweepPeriod, m_iSweepMode, m_iSweepShift;
	int16_t	m_iSweepCounter, m_iSweepResult;
	bool	m_bSweepWritten;
};
