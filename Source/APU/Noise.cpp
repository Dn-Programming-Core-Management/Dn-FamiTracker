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

#include "APU.h"
#include "Noise.h"

const uint16_t CNoise::NOISE_PERIODS_NTSC[] = {
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

const uint16_t CNoise::NOISE_PERIODS_PAL[] = {
	4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708,  944, 1890, 3778
};

CNoise::CNoise(CMixer *pMixer, int ID) : C2A03Chan(pMixer, SNDCHIP_NONE, ID)		// // //
{
	m_iLooping = 0;
	m_iEnvelopeFix = 0;
	m_iEnvelopeSpeed = 0;
	m_iEnvelopeVolume = 0;
	m_iFixedVolume = 0;
	m_iEnvelopeCounter = 0;
	m_iSampleRate = 0;
	m_iShiftReg = 0;

	PERIOD_TABLE = NOISE_PERIODS_NTSC;
}

CNoise::~CNoise()
{
}

void CNoise::Reset()
{
	m_iEnabled = m_iControlReg = 0;
	m_iCounter = m_iLengthCounter = 0;
	
	m_iShiftReg = 1;

	m_iEnvelopeCounter = m_iEnvelopeSpeed = 1;

	Write(0, 0);
	Write(1, 0);
	Write(2, 0);
	Write(3, 0);
	
	Mix(0);		// // //
	EndFrame();
}

void CNoise::Write(uint16_t Address, uint8_t Value)
{
	switch (Address) {
	case 0x00:
		m_iLooping = Value & 0x20;
		m_iEnvelopeFix = Value & 0x10;
		m_iFixedVolume = Value & 0x0F;
		m_iEnvelopeSpeed = (Value & 0x0F) + 1;
		break;
	case 0x01:
		break;
	case 0x02:
		m_iPeriod = PERIOD_TABLE[Value & 0x0F];
		m_iSampleRate = (Value & 0x80) ? 8 : 13;
		break;
	case 0x03:
		m_iLengthCounter = CAPU::LENGTH_TABLE[(Value >> 3) & 0x1F];
		m_iEnvelopeVolume = 0x0F;
		m_iEnvelopeCounter = m_iEnvelopeSpeed;		// // //
		if (m_iControlReg)
			m_iEnabled = 1;
		break;
	}
}

void CNoise::WriteControl(uint8_t Value)
{
	m_iControlReg = Value & 1;

	if (m_iControlReg == 0) 
		m_iEnabled = 0;
}

uint8_t CNoise::ReadControl()
{
	return ((m_iLengthCounter > 0) && (m_iEnabled == 1));
}

void CNoise::Process(uint32_t Time)
{
	bool Valid = m_iEnabled && (m_iLengthCounter > 0);

	while (Time >= m_iCounter) {
		Time	  -= m_iCounter;
		m_iTime	  += m_iCounter;
		m_iCounter = m_iPeriod;
		uint8_t Volume = m_iEnvelopeFix ? m_iFixedVolume : m_iEnvelopeVolume;
		Mix(Valid && (m_iShiftReg & 1) ? Volume : 0);
		m_iShiftReg = (((m_iShiftReg << 14) ^ (m_iShiftReg << m_iSampleRate)) & 0x4000) | (m_iShiftReg >> 1);
	}

	m_iCounter -= Time;
	m_iTime += Time;
}

double CNoise::GetFrequency() const		// // //
{
	if (!m_iEnabled || !m_iLengthCounter)
		return 0.;
	double Rate = PERIOD_TABLE == NOISE_PERIODS_PAL ? CAPU::BASE_FREQ_PAL : CAPU::BASE_FREQ_NTSC;
	return Rate / m_iPeriod;
}

void CNoise::LengthCounterUpdate()
{
	if ((m_iLooping == 0) && (m_iLengthCounter > 0)) 
		--m_iLengthCounter;
}

void CNoise::EnvelopeUpdate()
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
