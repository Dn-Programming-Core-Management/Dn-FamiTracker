/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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

#include <algorithm>
#include "APU.h"
#include "S5B.h"
#include "../RegisterState.h"

// // // 050B
// Sunsoft 5B channel class

const int32_t EXP_VOLUME[32] = {
	  0,   1,   1,   2,
	  2,   3,   3,   4,
	  5,   6,   7,   9,
	 11,  13,  15,  18,
	 22,  26,  31,  37,
	 45,  53,  63,  76,
	 90, 106, 127, 151,
	180, 212, 255, 255
};

CS5BChannel::CS5BChannel(CMixer *pMixer, uint8_t ID) : CChannel(pMixer, SNDCHIP_S5B, ID),
	m_iVolume(0),
	m_iPeriod(0),
	m_iPeriodClock(0),
	m_bSquareHigh(false),
	m_bSquareDisable(false),
	m_bNoiseDisable(false)
{
}

void CS5BChannel::Process(uint32_t Time)
{
	m_iPeriodClock += Time;
	if (m_iPeriodClock >= m_iPeriod) {
		m_iPeriodClock = 0;
		m_bSquareHigh = !m_bSquareHigh;
	}
	m_iTime += Time;
}

void CS5BChannel::Reset()
{
	m_iVolume = 0;
	m_iPeriod = 0;
	m_iPeriodClock = 0;
	m_bSquareHigh = false;
	m_bSquareDisable = true;
	m_bNoiseDisable = true;
}

uint32_t CS5BChannel::GetTime()
{
	if (m_iPeriod < 2U || !m_iVolume)
		return 0xFFFFFU;
	return m_iPeriod - m_iPeriodClock;
}

void CS5BChannel::Output(uint32_t Noise, uint32_t Envelope)
{
	int Level = ((m_iVolume & 0x20) ? Envelope : m_iVolume) & 0x1F;
	int32_t Output = EXP_VOLUME[Level];
	if (!m_bSquareDisable && !m_bSquareHigh && m_iPeriod >= 2U)
		Output = 0;
	if (!m_bNoiseDisable && !Noise)
		Output = 0;
	Mix(Output);
}

double CS5BChannel::GetFrequency() const		// // //
{
	if (m_bSquareDisable || !m_iPeriod)
		return 0.;
	return CAPU::BASE_FREQ_NTSC / 2. / m_iPeriod;
}



// Sunsoft 5B chip class

CS5B::CS5B(CMixer *pMixer) : CSoundChip(pMixer),
	m_cPort(0),
	m_iCounter(0)
{
	m_pRegisterLogger->AddRegisterRange(0x00, 0x0F);		// // //

	m_pChannel[0] = new CS5BChannel(pMixer, CHANID_S5B_CH1);
	m_pChannel[1] = new CS5BChannel(pMixer, CHANID_S5B_CH2);
	m_pChannel[2] = new CS5BChannel(pMixer, CHANID_S5B_CH3);
	Reset();
}

CS5B::~CS5B()
{
	for (auto x : m_pChannel)
		if (x)
			delete x;
}

void CS5B::Reset()
{
	m_iNoiseState = 0xFFFF;
	m_iCounter = 0;
	m_iNoisePeriod = 0x1F << 5;
	m_iNoiseClock = 0;
	m_iEnvelopePeriod = 0;
	m_iEnvelopeClock = 0;
	m_iEnvelopeLevel = 0;
	m_iEnvelopeShape = 0;
	m_bEnvelopeHold = true;
	
	for (auto x : m_pChannel)
		x->Reset();
}

void CS5B::Process(uint32_t Time)
{
	while (Time > 0U) {
		uint32_t TimeToRun = Time;
		if (m_iEnvelopeClock < m_iEnvelopePeriod)
			TimeToRun = std::min<uint32_t>(m_iEnvelopePeriod - m_iEnvelopeClock, TimeToRun);
		if (m_iNoiseClock < m_iNoisePeriod)
			TimeToRun = std::min<uint32_t>(m_iNoisePeriod - m_iNoiseClock, TimeToRun);
		for (const auto x : m_pChannel)
			TimeToRun = std::min<uint32_t>(x->GetTime(), TimeToRun);

		m_iCounter += TimeToRun;
		Time -= TimeToRun;

		RunEnvelope(TimeToRun);
		RunNoise(TimeToRun);
		for (auto x : m_pChannel)
			x->Process(TimeToRun);

		for (auto x : m_pChannel)
			x->Output(m_iNoiseState & 0x01, m_iEnvelopeLevel);
	}
}

void CS5B::EndFrame()
{
	for (auto x : m_pChannel)
		x->EndFrame();
	m_iCounter = 0;
}

void CS5B::Write(uint16_t Address, uint8_t Value)
{
	switch (Address) {
	case 0xC000:
		m_cPort = Value & 0x0F;
		break;
	case 0xE000:
		WriteReg(m_cPort, Value);
		break;
	}
}

uint8_t CS5B::Read(uint16_t Address, bool &Mapped)
{
	Mapped = false;
	return 0U;
}

double CS5B::GetFreq(int Channel) const		// // //
{
	switch (Channel) {
	case 0: case 1: case 2:
		return m_pChannel[Channel]->GetFrequency();
	case 3:
		if (!m_iEnvelopePeriod)
			return 0.;
		if (!(m_iEnvelopeShape & 0x08) || (m_iEnvelopeShape & 0x01))
			return 0.;
		return CAPU::BASE_FREQ_NTSC / ((m_iEnvelopeShape & 0x02) ? 64. : 32.) / m_iEnvelopePeriod;
	//case 4: TODO noise refresh rate
	}
	return 0.;
}

void CS5B::WriteReg(uint8_t Port, uint8_t Value)
{
	switch (Port) {
	case 0x00: case 0x02: case 0x04:
	{
		auto pChan = m_pChannel[Port >> 1];
		pChan->m_iPeriod = (pChan->m_iPeriod & 0xF000) | (Value << 4);
	}
		break;
	case 0x01: case 0x03: case 0x05:
	{
		auto pChan = m_pChannel[Port >> 1];
		pChan->m_iPeriod = (pChan->m_iPeriod & 0x0FF0) | ((Value & 0x0F) << 12);
	}
		break;
	case 0x06:
		m_iNoisePeriod = Value ? ((Value & 0x1F) << 5) : 0x10;
		break;
	case 0x07:
		for (int i = 0; i < 3; ++i) {
			auto pChan = m_pChannel[i];
			pChan->m_bSquareDisable = (Value & (1 << i)) != 0;
			pChan->m_bNoiseDisable = (Value & (1 << (i + 3))) != 0;
		}
		break;
	case 0x08: case 0x09: case 0x0A:
		m_pChannel[Port - 0x08]->m_iVolume = Value * 2;
		break;
	case 0x0B:
		m_iEnvelopePeriod = (m_iEnvelopePeriod & 0xFF000) | (Value << 4);
		break;
	case 0x0C:
		m_iEnvelopePeriod = (m_iEnvelopePeriod & 0x00FF0) | (Value << 12);
		break;
	case 0x0D:
		m_iEnvelopeClock = 0;
		m_iEnvelopeShape = Value;
		m_bEnvelopeHold = false;
		m_iEnvelopeLevel = (Value & 0x04) ? 0 : 0x1F;
		break;
	}
}

void CS5B::Log(uint16_t Address, uint8_t Value)		// // //
{
	switch (Address) {
	case 0xC000: m_pRegisterLogger->SetPort(Value); break;
	case 0xE000: m_pRegisterLogger->Write(Value); break;
	}
}

void CS5B::RunEnvelope(uint32_t Time)
{
	m_iEnvelopeClock += Time;
	if (m_iEnvelopeClock >= m_iEnvelopePeriod && m_iEnvelopePeriod) {
		m_iEnvelopeClock = 0;
		if (!m_bEnvelopeHold) {
			m_iEnvelopeLevel += (m_iEnvelopeShape & 0x04) ? 1 : -1;
			m_iEnvelopeLevel &= 0x3F;
		}
		if (m_iEnvelopeLevel & 0x20) {
			if (m_iEnvelopeShape & 0x08) {
				if ((m_iEnvelopeShape & 0x03) == 0x01 || (m_iEnvelopeShape & 0x03) == 0x02)
					m_iEnvelopeShape ^= 0x04;
				if (m_iEnvelopeShape & 0x01)
					m_bEnvelopeHold = true;
				m_iEnvelopeLevel = (m_iEnvelopeShape & 0x04) ? 0 : 0x1F;
			}
			else {
				m_bEnvelopeHold = true;
				m_iEnvelopeLevel = 0;
			}
		}
	}
}

void CS5B::RunNoise(uint32_t Time)
{
	m_iNoiseClock += Time;
	if (m_iNoiseClock >= m_iNoisePeriod) {
		m_iNoiseClock = 0;
		if (m_iNoiseState & 0x01)
			m_iNoiseState ^= 0x24000;
		m_iNoiseState >>= 1;
	}
}
