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

#include <memory>
#include "APU.h"
#include "Square.h"

// This is also shared with MMC5

const uint8 CSquare::DUTY_TABLE[4][16] = {
	{0, 0, 1, 1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0},
	{0, 0, 1, 1,  1, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0},
	{0, 0, 1, 1,  1, 1, 1, 1,  1, 1, 0, 0,  0, 0, 0, 0},
	{1, 1, 0, 0,  0, 0, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1}
};

CSquare::CSquare(CMixer *pMixer, int ID, int Chip) : CChannel(pMixer, ID, Chip)
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

void CSquare::Write(uint16 Address, uint8 Value)
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

void CSquare::WriteControl(uint8 Value)
{
	m_iControlReg = Value & 0x01;

	if (m_iControlReg == 0)
		m_iEnabled = 0;
}

uint8 CSquare::ReadControl()
{
	return ((m_iLengthCounter > 0) && (m_iEnabled == 1));
}

void CSquare::Process(uint32 Time)
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
		uint8 Volume = m_iEnvelopeFix ? m_iFixedVolume : m_iEnvelopeVolume;
		Mix(Valid && DUTY_TABLE[m_iDutyLength][m_iDutyCycle] ? Volume : 0);
		m_iDutyCycle = (m_iDutyCycle + 1) & 0x0F;
	}

	m_iCounter -= Time;
	m_iTime += Time;
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
