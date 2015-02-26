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
#include "../Common.h"
#include "APU.h"
#include "N163.h"

/*

 Namco 163 emulation

 I found out that all channels shares the same DAC, and the chip
 switches channel at the rate of 120 kHz. This is why there is
 high pitched noise (15 kHz) when using 8 channels.

*/

//
// Namco 163 (previously called N106)
//

CN163::CN163(CMixer *pMixer) : 
	m_iChansInUse(0), 
	m_iExpandAddr(0), 
	m_iLastValue(0), 
	m_iGlobalTime(0), 
	m_iChannelCntr(0),
	m_iActiveChan(0),
	m_iCycle(0),
	m_bOldMixing(0)		// // //
{
	m_pWaveData = new uint8[0x80];

	m_pMixer = pMixer;

	m_pChannels[0] = new CN163Chan(pMixer, CHANID_N163_CHAN1, m_pWaveData);
	m_pChannels[1] = new CN163Chan(pMixer, CHANID_N163_CHAN2, m_pWaveData);
	m_pChannels[2] = new CN163Chan(pMixer, CHANID_N163_CHAN3, m_pWaveData);
	m_pChannels[3] = new CN163Chan(pMixer, CHANID_N163_CHAN4, m_pWaveData);
	m_pChannels[4] = new CN163Chan(pMixer, CHANID_N163_CHAN5, m_pWaveData);
	m_pChannels[5] = new CN163Chan(pMixer, CHANID_N163_CHAN6, m_pWaveData);
	m_pChannels[6] = new CN163Chan(pMixer, CHANID_N163_CHAN7, m_pWaveData);
	m_pChannels[7] = new CN163Chan(pMixer, CHANID_N163_CHAN8, m_pWaveData);
}

CN163::~CN163()
{
	if (m_pWaveData)
		delete [] m_pWaveData;

	for (int i = 0; i < 8; ++i)
		delete m_pChannels[i];
}

void CN163::Reset()
{
	for (int i = 0; i < 8; ++i)
		m_pChannels[i]->Reset();

	m_iLastValue = 0;

	m_iGlobalTime = 0;

	m_iChannelCntr = 0;
	m_iActiveChan = 7;
	m_iCycle = 0;
}

void CN163::SetMixingMethod(bool bLinear)		// // //
{
	m_bOldMixing = bLinear;
}

void CN163::Process(uint32 Time)
{
	if (m_bOldMixing) {		// // //
		ProcessOld(Time);
		return;
	}

	const uint32 CHAN_PERIOD = 15;		// 15 cycles/channel

	m_pMixer->SetNamcoVolume((m_iChansInUse == 0) ? 1.3f : (1.5f + float(m_iChansInUse - 1) / 1.5f));

	while (Time > 0) {

		uint32 TimeToRun = CHAN_PERIOD - m_iChannelCntr;

		if (TimeToRun > Time)
			TimeToRun = Time;

		m_pChannels[m_iActiveChan]->Process(TimeToRun, m_iChansInUse + 1, this);

		Time -= TimeToRun;
		m_iGlobalTime += TimeToRun;
		m_iChannelCntr += TimeToRun;

		if (m_iChannelCntr >= CHAN_PERIOD) {
			if (m_iActiveChan + m_iChansInUse < 8)
				m_iActiveChan = 8;
			m_iActiveChan--;
			m_iChannelCntr -= CHAN_PERIOD;
		}
	}
}

void CN163::ProcessOld(uint32 Time)		// // //
{
	m_pMixer->SetNamcoVolume((m_iChansInUse == 0) ? 1.0f : 0.75f);

	for (int i = 7 - m_iChansInUse; i < 8; ++i)
		m_pChannels[i]->ProcessClean(Time, m_iChansInUse + 1);
}

void CN163::Mix(int32 Value, uint32 Time, uint8 ChanID)
{
	// N163 amplitude:
	// One channel: 1.1V P-P
	// Two-eight channels: 800mV P-P
	// 2A03 triangle: 330mV P-P

	int32 Delta = Value - m_iLastValue;

	if (Delta)
		m_pMixer->AddValue(ChanID, SNDCHIP_N163, Delta, Value, Time + m_iGlobalTime);

	m_iLastValue = Value;
}

void CN163::EndFrame()
{
	for (int i = 0; i < 8; ++i)
		m_pChannels[i]->EndFrame();

	m_iGlobalTime = 0;
}

void CN163::Write(uint16 Address, uint8 Value)
{
	int Area = m_iExpandAddr & 0x7F;

	switch (Address) {
		case 0x4800:
			if (Area < 0x80)
				m_pWaveData[Area] = Value;

			if (Area >= 0x40) {
				int Channel = (Area & 0x3F) >> 3;
				m_pChannels[Channel]->Write(Area & 0x07, Value);

				if (Area == 0x7F)
					m_iChansInUse = (Value >> 4) & 0x07;
			}

			if (m_iExpandAddr & 0x80)
				m_iExpandAddr = (m_iExpandAddr + 1) | 0x80;
			break;

		case 0xF800:
			m_iExpandAddr = Value;
			break;
	}

}

uint8 CN163::Read(uint16 Address, bool &Mapped)
{
	uint16 ReadAddr;

	switch (Address) {
		case 0x4800:
			Mapped = true;
			ReadAddr = m_iExpandAddr & 0x7F;
			if (m_iExpandAddr & 0x80)
				m_iExpandAddr = ((m_iExpandAddr + 1) & 0x7F) | 0x80;
			return m_pWaveData[ReadAddr];
	}

	return 0;
}

uint8 CN163::ReadMem(uint8 Reg)
{
	int ChanArea = 0x80 - ((m_iChansInUse + 1) << 3);
	int Chan = (Reg & 0x3F) >> 3;
	
	if (Reg < ChanArea)
		return m_pWaveData[Reg & 0x7F];

	if ((Reg & 7) == 7)
		return m_pChannels[Chan]->ReadMem(Reg) | (m_pWaveData[Reg] & 0xF0);

	return m_pChannels[Chan]->ReadMem(Reg);
}

//
// N163 channels
//

CN163Chan::CN163Chan(CMixer *pMixer, int ID, uint8 *pWaveData) : 
	CExChannel(pMixer, SNDCHIP_N163, ID),
	m_pWaveData(pWaveData)
{
	Reset();
}

CN163Chan::~CN163Chan()
{
}

void CN163Chan::Reset()
{
	m_iCounter	  = 0;
	m_iWaveOffset = 0;
	m_iWaveLength = 0;
	m_iVolume	  = 0;
	m_iLastSample = 0;
	m_iPhase	  = 0;
	m_iFrequency  = 0;

	EndFrame();
}

void CN163Chan::EndFrame()
{
	m_iTime = 0;
}

void CN163Chan::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0x00:
			m_iFrequency = (m_iFrequency & 0xFFFF00) | Value;
			break;
		case 0x01:
			m_iPhase = (m_iPhase & 0xFFFF00) | Value;
			break;
		case 0x02:
			m_iFrequency = (m_iFrequency & 0xFF00FF) | (Value << 8);
			break;
		case 0x03:
			m_iPhase = (m_iPhase & 0xFF00FF) | (Value << 8);
			break;
		case 0x04:
			m_iFrequency = (m_iFrequency & 0x00FFFF) | ((Value & 3) << 16);
			m_iWaveLength = (256 - (Value & 0xFC)) << 16;
			break;
		case 0x05:
			m_iPhase = (m_iPhase & 0x00FFFF) | (Value << 16);
			break;
		case 0x06:
			m_iWaveOffset = Value;
			break;
		case 0x07:
			m_iVolume = Value & 0x0F;
			break;
	}
}

void CN163Chan::Process(uint32 Time, uint8 ChannelsActive, CN163 *pParent)
{
	uint32 TimeStamp = 0;

	pParent->Mix(m_iLastSample, TimeStamp, m_iChanId);

	if (!m_iFrequency || !m_iWaveLength) {
		m_iLastSample = 0;
		m_iTime += Time;
		return;
	}

	while (Time >= m_iCounter) {
		Time	-= m_iCounter;
		m_iTime += m_iCounter;
		TimeStamp += m_iCounter;
		m_iCounter = 15;

		m_iPhase = (m_iPhase + m_iFrequency) % m_iWaveLength;

		int WavePtr = m_iPhase >> 16;

		uint8 Sample = m_pWaveData[((WavePtr + m_iWaveOffset) & 0xFF) >> 1];

		if (WavePtr & 1)
			Sample >>= 4;

		m_iLastSample = (Sample & 0x0F) * m_iVolume;
		pParent->Mix(m_iLastSample, TimeStamp, m_iChanId);
	}

	m_iCounter -= Time;
	m_iTime += Time;
}

void CN163Chan::ProcessClean(uint32 Time, uint8 ChannelsActive)
{
	// legacy

	if (!m_iFrequency || !m_iWaveLength) {
		m_iTime += Time;
		return;
	}

	while (Time >= m_iCounter) {
		Time	-= m_iCounter;
		m_iTime += m_iCounter;
		m_iCounter = 15 * ChannelsActive;
		
		m_iPhase = (m_iPhase + m_iFrequency) % m_iWaveLength;

		int WavePtr = m_iPhase >> 16;

		uint8 Sample = m_pWaveData[((WavePtr + m_iWaveOffset) & 0xFF) >> 1];

		if (WavePtr & 1)
			Sample >>= 4;

		Mix((Sample & 0x0F) * m_iVolume);
	}
	
	m_iCounter -= Time;
	m_iTime += Time;
}

uint8 CN163Chan::ReadMem(uint8 Reg)
{
	switch (Reg & 7) {
		case 0:
			return m_iFrequency & 0xFF;
		case 1:
			return m_iPhase & 0xFF;
		case 2:
			return (m_iFrequency >> 8) & 0xFF;
		case 3:
			return (m_iPhase >> 8) & 0xFF;
		case 4:
			return (uint8)(((m_iFrequency >> 16) & 0x03) | (256 - (m_iWaveLength >> 16)));
		case 5:
			return (m_iPhase >> 16) & 0xFF;
		case 6:
			return m_iWaveOffset;
		case 7:
			return m_iVolume;
	}

	return 0;
}
