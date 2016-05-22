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
#include "S5B.h"
#include "emu2149.h"
#include "../RegisterState.h"		// // //

// Sunsoft 5B (YM2149)

PSG *psg;

float CS5B::AMPLIFY = 2.0f;

CS5B::CS5B(CMixer *pMixer) :
	CSoundChip(pMixer),		// // //
	m_iRegister(0),
	m_iTime(0),
	m_fVolume(AMPLIFY)
{
	m_pRegisterLogger->AddRegisterRange(0x00, 0x0F);		// // //
	psg = NULL;
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

void CS5B::Process(uint32_t Time)
{
	m_iTime += Time;
}

int16_t m_pBuffer[4000];
uint32_t m_iBufferPtr = 0;

void CS5B::EndFrame()
{
	GetMixMono();
}

void CS5B::GetMixMono()
{
	static int32_t LastSample = 0;

	uint32_t WantSamples = m_pMixer->GetMixSampleCount(m_iTime);

	// Generate samples
	while (m_iBufferPtr < WantSamples) {
		int32_t Sample = int32_t(float(PSG_calc(psg)) * m_fVolume);
		m_pBuffer[m_iBufferPtr++] = int16_t((Sample + LastSample) >> 1);
		LastSample = Sample;
	}

	m_pMixer->MixSamples((blip_sample_t*)m_pBuffer, WantSamples);

	m_iBufferPtr -= WantSamples;
	m_iTime = 0;
}

void CS5B::Write(uint16_t Address, uint8_t Value)
{
	switch (Address) {
		case 0xC000:
			m_iRegister = Value & 0xF;
			break;
		case 0xE000:
			PSG_writeReg(psg, m_iRegister, Value);
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

uint8_t CS5B::Read(uint16_t Address, bool &Mapped)
{
	// No reads here
	Mapped = false;
	return 0;
}

void CS5B::SetSampleSpeed(uint32_t SampleRate, double ClockRate, uint32_t FrameRate)
{
	if (psg != NULL) {
		PSG_delete(psg);
	}

	//PSG_init((uint32_t)ClockRate, SampleRate);
	psg = PSG_new((uint32_t)ClockRate, SampleRate);
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

double CS5B::GetFreq(int Channel) const		// // //
{
	if (Channel == 3) {
		int Reg = m_pRegisterLogger->GetRegister(0x0B)->GetValue() | m_pRegisterLogger->GetRegister(0x0C)->GetValue() << 8;
		if (!Reg)
			return 0.;
		int Shape = m_pRegisterLogger->GetRegister(0x0D)->GetValue();
		if (!(Shape & 0x08) || (Shape & 0x01))
			return 0.;
		return CAPU::BASE_FREQ_NTSC / ((Shape & 0x02) ? 1024. : 512.) / Reg;
	}
	else if (Channel == 4) {
		// TODO: noise refresh rate
		return 0.;
	}
	else if (Channel < 0 || Channel >= 5)
		return 0.;
	if (m_pRegisterLogger->GetRegister(0x07)->GetValue() & (1 << Channel))
		return 0.;
	int Reg = m_pRegisterLogger->GetRegister(Channel * 2)->GetValue();
	Reg |= (m_pRegisterLogger->GetRegister(Channel * 2 + 1)->GetValue() << 8) & 0xF00;
	if (!Reg)
		return 0.;
	return CAPU::BASE_FREQ_NTSC / 32. / Reg;
}

/*
void CS5B::SetChannelVolume(int Chan, int LevelL, int LevelR)
{
	PSG_set_chan_vol(Chan, LevelL, LevelR);
}
*/