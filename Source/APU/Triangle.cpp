/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#include "APU.h"
#include "Triangle.h"

const uint8 CTriangle::TRIANGLE_WAVE[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 
	0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
};

CTriangle::CTriangle(CMixer *pMixer, int ID) : CChannel(pMixer, ID, SNDCHIP_NONE)
{
	m_iStepGen = 0;
	m_iLoop = 0;
	m_iLinearLoad = 0;
	m_iHalt = 0;
	m_iLinearCounter = 0;
}

CTriangle::~CTriangle()
{
}

void CTriangle::Reset()
{
	m_iEnabled = m_iControlReg = 0;
	m_iCounter = m_iLengthCounter = 0;

	Write(0, 0);
	Write(1, 0);
	Write(2, 0);
	Write(3, 0);

	EndFrame();
}

void CTriangle::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x00:
			m_iLinearLoad = Value & 0x7F;
			m_iLoop = Value & 0x80;
			break;
		case 0x01:
			break;
		case 0x02:
			m_iPeriod = Value | (m_iPeriod & 0x0700);
			break;
		case 0x03:
			m_iPeriod = ((Value & 0x07) << 8) | (m_iPeriod & 0xFF);
			m_iLengthCounter = CAPU::LENGTH_TABLE[(Value & 0xF8) >> 3];
			m_iHalt = 1;
			if (m_iControlReg)
				m_iEnabled = 1;
			break;
	}
}

void CTriangle::WriteControl(uint8 Value)
{
	m_iControlReg = Value & 1;
	
	if (m_iControlReg == 0)
		m_iEnabled = 0;
}

uint8 CTriangle::ReadControl()
{
	return ((m_iLengthCounter > 0) && (m_iEnabled == 1));
}

void CTriangle::Process(uint32 Time)
{
	if (!m_iLinearCounter || !m_iLengthCounter || !m_iEnabled) {
		m_iTime += Time;
		return;
	}
	else if (m_iPeriod <= 1) {
		/*
		// Frequency is too high to be audible
		m_iTime += Time;
		m_iStepGen = 7;
		Mix(TRIANGLE_WAVE[m_iStepGen]);
		return;
		*/
	}

	while (Time >= m_iCounter) {
		Time	  -= m_iCounter;
		m_iTime   += m_iCounter;
		m_iCounter = m_iPeriod + 1;
		Mix(TRIANGLE_WAVE[m_iStepGen]);
		m_iStepGen = (m_iStepGen + 1) & 0x1F;
	}
	
	m_iCounter -= Time;
	m_iTime += Time;
}

void CTriangle::LengthCounterUpdate()
{
	if ((m_iLoop == 0) && (m_iLengthCounter > 0)) 
		m_iLengthCounter--;
}

void CTriangle::LinearCounterUpdate()
{
	/*
		1.  If the halt flag is set, the linear counter is reloaded with the counter reload value, 
			otherwise if the linear counter is non-zero, it is decremented.

		2.  If the control flag is clear, the halt flag is cleared. 
	*/

	if (m_iHalt == 1)
		m_iLinearCounter = m_iLinearLoad;
	else
		if (m_iLinearCounter > 0)
			m_iLinearCounter--;

	if (m_iLoop == 0)
		m_iHalt = 0;
}
