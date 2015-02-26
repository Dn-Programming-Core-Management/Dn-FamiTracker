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

#include <cstdio>
#include "APU.h"
#include "S5B.h"
#include "emu2149.h"

// Sunsoft 5B (YM2149)

PSG *psg;

float CS5B::AMPLIFY = 2.0f;

CS5B::CS5B(CMixer *pMixer) : m_iRegister(0), m_iTime(0)
{
	m_pMixer = pMixer;

	m_fVolume = AMPLIFY;

	psg = NULL;

	for (int i = 0; i < 16; i++)		// // //
		m_iLocalReg[i] = 0;
}

CS5B::~CS5B()
{
	if (psg)
		PSG_delete(psg);
}

void CS5B::Reset()
{
	m_iTime = 0;
//	PSG_reset(psg);
}

void CS5B::Process(uint32 Time)
{
	m_iTime += Time;
}

int16 m_pBuffer[4000];
uint32 m_iBufferPtr = 0;

void CS5B::EndFrame()
{
	GetMixMono();
}

void CS5B::GetMixMono()
{
	static int32 LastSample = 0;

	uint32 WantSamples = m_pMixer->GetMixSampleCount(m_iTime);

	// Generate samples
	while (m_iBufferPtr < WantSamples) {
		int32 Sample = int32(float(PSG_calc(psg)) * m_fVolume);
		m_pBuffer[m_iBufferPtr++] = int16((Sample + LastSample) >> 1);
		LastSample = Sample;
	}

	m_pMixer->MixSamples((blip_sample_t*)m_pBuffer, WantSamples);

	m_iBufferPtr -= WantSamples;
	m_iTime = 0;
}

void CS5B::Write(uint16 Address, uint8 Value)
{
	switch (Address) {
		case 0xC000:
			m_iRegister = Value & 0xF;
			break;
		case 0xE000:
			PSG_writeReg(psg, m_iRegister, Value);
			m_iLocalReg[m_iRegister] = Value;		// // //
			break;
	}
}

uint8 CS5B::Read(uint16 Address, bool &Mapped)
{
	// No reads here
	Mapped = false;
	return 0;
}

uint8 CS5B::GetLocalReg(uint8 Address)		// // //
{
	return m_iLocalReg[Address];
}

void CS5B::SetSampleSpeed(uint32 SampleRate, double ClockRate, uint32 FrameRate)
{
	if (psg != NULL) {
		PSG_delete(psg);
	}

	//PSG_init((uint32)ClockRate, SampleRate);
	psg = PSG_new((uint32)ClockRate, SampleRate);
	PSG_setVolumeMode(psg, 1);
	PSG_reset(psg);

//	psg = PSG_new();

//	PSG_setVolumeMode(psg, 1);
//	PSG_reset(psg);
}

void CS5B::SetVolume(float fVol)
{
	m_fVolume = AMPLIFY * fVol;
}
/*
void CS5B::SetChannelVolume(int Chan, int LevelL, int LevelR)
{
	PSG_set_chan_vol(Chan, LevelL, LevelR);
}
*/