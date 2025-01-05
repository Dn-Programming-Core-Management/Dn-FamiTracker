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

#include <memory>
#include "APU.h"
#include "Square.h"

// This is also shared with MMC5

const uint8_t CSquare::DUTY_TABLE[4][16] = {
	{0, 0, 1, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0},
	{0, 0, 1, 1,  1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0},
	{0, 0, 1, 1,  1, 1, 1, 1,  1, 1, 0, 0,  0, 0, 0, 0},
	{1, 1, 0, 0,  0, 0, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1}
};

CSquare::CSquare(CMixer *pMixer, int ID, int Chip) : C2A03Chan(pMixer, Chip, ID)		// // //
{
	m_iDutyLength = 0;
	m_iDutyCycle = 0;
	m_iLooping = 0;
	m_iEnvelopeFix = 0;
	m_iEnvelopeSpeed = 0;
	m_iEnvelopeVolume = 0;
	m_iSweepEnabled = 0;
	m_iSweepPeriod = 0;
	m_iSweepMode = 0;
	m_iSweepShift = 0;
	m_iSweepCounter = 0;
	m_iSweepResult = 0;
	m_bSweepWritten = 0;

	m_iFixedVolume = 0;
	m_iEnvelopeCounter = 0;

	CPU_RATE = CAPU::BASE_FREQ_NTSC;		// // //
}

CSquare::~CSquare()
{
}

void CSquare::Reset()
{
	m_iEnabled = m_iControlReg = 0;
	m_iCounter = 0;

	m_iSweepCounter = 1;
	m_iSweepPeriod = 1;

	m_iEnvelopeCounter = 1;
	m_iEnvelopeSpeed = 1;

	Write(0, 0);
	Write(1, 0);
	Write(2, 0);
	Write(3, 0);

	SweepUpdate(false);

	Mix(0);		// // //
	EndFrame();
}

void CSquare::Write(uint16_t Address, uint8_t Value)
{
	switch (Address) {
	case 0x00:
		m_iDutyLength	 = Value >> 6;
		m_iFixedVolume	 = Value & 0x0F;
		m_iLooping	 	 = Value & 0x20;
		m_iEnvelopeFix	 = Value & 0x10;
		m_iEnvelopeSpeed = (Value & 0x0F) + 1;
		break;
	case 0x01:
		m_iSweepEnabled  = Value & 0x80;
		m_iSweepPeriod	 = ((Value >> 4) & 0x07) + 1;
		m_iSweepMode	 = Value & 0x08;		
		m_iSweepShift	 = Value & 0x07;
		m_bSweepWritten	 = true;
		break;
	case 0x02:
		m_iPeriod = Value | (m_iPeriod & 0x0700);
		break;
	case 0x03:
		m_iPeriod = ((Value & 0x07) << 8) | (m_iPeriod & 0xFF);
		m_iLengthCounter = CAPU::LENGTH_TABLE[(Value & 0xF8) >> 3];
		m_iDutyCycle = 0;
		m_iEnvelopeVolume = 0x0F;
		m_iEnvelopeCounter = m_iEnvelopeSpeed;		// // //
		if (m_iControlReg)
			m_iEnabled = 1;
		break;
	}
}

void CSquare::WriteControl(uint8_t Value)
{
	m_iControlReg = Value & 0x01;

	if (m_iControlReg == 0)
		m_iEnabled = 0;
}

uint8_t CSquare::ReadControl()
{
	return ((m_iLengthCounter > 0) && (m_iEnabled == 1));
}

void CSquare::Process(uint32_t Time)
{
	if (!m_iPeriod) {
		m_iTime += Time;
		return;
	}

	bool Valid = (m_iPeriod > 7 || (m_iPeriod > 0 && m_iChip == SNDCHIP_MMC5))		// // //
		&& (m_iEnabled != 0) && (m_iLengthCounter > 0) && (m_iSweepResult < 0x800);

	while (Time >= m_iCounter) {
		Time		-= m_iCounter;
		m_iTime		+= m_iCounter;
		m_iCounter	 = m_iPeriod + 1;
		uint8_t Volume = m_iEnvelopeFix ? m_iFixedVolume : m_iEnvelopeVolume;
		Mix(static_cast<int32_t>(Valid && DUTY_TABLE[m_iDutyLength][m_iDutyCycle] ? Volume : 0) * -1);
		m_iDutyCycle = (m_iDutyCycle + 1) & 0x0F;
	}

	m_iCounter -= Time;
	m_iTime += Time;
}

double CSquare::GetFrequency() const		// // //
{
	bool Valid = (m_iPeriod > 7 || (m_iPeriod > 0 && m_iChip == SNDCHIP_MMC5))
		&& m_iEnabled && m_iLengthCounter && m_iSweepResult < 0x800;
	if (!Valid)
		return 0.;
	return CPU_RATE / 16. / (m_iPeriod + 1.);
}

void CSquare::LengthCounterUpdate()
{
	if ((m_iLooping == 0) && (m_iLengthCounter > 0)) 
		--m_iLengthCounter;
}

void CSquare::SweepUpdate(int Diff)
{
	m_iSweepResult = (m_iPeriod >> m_iSweepShift);

	if (m_iSweepMode)
		m_iSweepResult = m_iPeriod - m_iSweepResult - Diff;
	else
		m_iSweepResult = m_iPeriod + m_iSweepResult;

	if (--m_iSweepCounter == 0) {
		m_iSweepCounter = m_iSweepPeriod;
		if (m_iSweepEnabled && (m_iPeriod > 0x07) && (m_iSweepResult < 0x800) && (m_iSweepShift > 0))
			m_iPeriod = m_iSweepResult;
	}

	if (m_bSweepWritten) {
		m_bSweepWritten = false;
		m_iSweepCounter = m_iSweepPeriod;
	}
}

void CSquare::EnvelopeUpdate()
{
	if (--m_iEnvelopeCounter == 0) {
		m_iEnvelopeCounter = m_iEnvelopeSpeed;
		if (!m_iEnvelopeFix) {
			if (m_iLooping)
				m_iEnvelopeVolume = (m_iEnvelopeVolume - 1) & 0x0F;
			else if (m_iEnvelopeVolume > 0)
				m_iEnvelopeVolume--;
		}
	}
}
