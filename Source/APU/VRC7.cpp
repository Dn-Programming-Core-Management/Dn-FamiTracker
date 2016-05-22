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

#include "../stdafx.h"
#include "APU.h"
#include "VRC7.h"
#include "../RegisterState.h"		// // //

const float  CVRC7::AMPLIFY	  = 4.6f;		// Mixing amplification, VRC7 patch 14 is 4,88 times stronger than a 50% square @ v=15
const uint32_t CVRC7::OPL_CLOCK = 3579545;	// Clock frequency

CVRC7::CVRC7(CMixer *pMixer) : CSoundChip(pMixer), m_pBuffer(NULL), m_pOPLLInt(NULL), m_fVolume(1.0f), m_iMaxSamples(0), m_iSoundReg(0)
{
	m_pRegisterLogger->AddRegisterRange(0x00, 0x07);		// // //
	m_pRegisterLogger->AddRegisterRange(0x10, 0x15);
	m_pRegisterLogger->AddRegisterRange(0x20, 0x25);
	m_pRegisterLogger->AddRegisterRange(0x30, 0x35);
	Reset();
}

CVRC7::~CVRC7()
{
	if (m_pOPLLInt != NULL) {
		OPLL_delete(m_pOPLLInt);
		m_pOPLLInt = NULL;
	}

	SAFE_RELEASE_ARRAY(m_pBuffer);
}

void CVRC7::Reset()
{
	m_iBufferPtr = 0;
	m_iTime = 0;
}

void CVRC7::SetSampleSpeed(uint32_t SampleRate, double ClockRate, uint32_t FrameRate)
{
	if (m_pOPLLInt != NULL) {
		OPLL_delete(m_pOPLLInt);
	}

	m_pOPLLInt = OPLL_new(OPL_CLOCK, SampleRate);

	OPLL_reset(m_pOPLLInt);
	OPLL_reset_patch(m_pOPLLInt, 1);

	m_iMaxSamples = (SampleRate / FrameRate) * 2;	// Allow some overflow

	SAFE_RELEASE_ARRAY(m_pBuffer);
	m_pBuffer = new int16_t[m_iMaxSamples];
	memset(m_pBuffer, 0, sizeof(int16_t) * m_iMaxSamples);
}

void CVRC7::SetVolume(float Volume)
{
	m_fVolume = Volume * AMPLIFY;
}

void CVRC7::Write(uint16_t Address, uint8_t Value)
{
	switch (Address) {
		case 0x9010:
			m_iSoundReg = Value;
			break;
		case 0x9030:
			OPLL_writeReg(m_pOPLLInt, m_iSoundReg, Value);
			break;
	}
}

void CVRC7::Log(uint16_t Address, uint8_t Value)		// // //
{
	switch (Address) {
	case 0x9010: m_pRegisterLogger->SetPort(Value); break;
	case 0x9030: m_pRegisterLogger->Write(Value); break;
	}
}

uint8_t CVRC7::Read(uint16_t Address, bool &Mapped)
{
	return 0;
}

void CVRC7::EndFrame()
{
	uint32_t WantSamples = m_pMixer->GetMixSampleCount(m_iTime);

	static int32_t LastSample = 0;

	// Generate VRC7 samples
	while (m_iBufferPtr < WantSamples) {
		int32_t RawSample = OPLL_calc(m_pOPLLInt);
		
		// Clipping is slightly asymmetric
		if (RawSample > 3600)
			RawSample = 3600;
		if (RawSample < -3200)
			RawSample = -3200;

		// Apply volume
		int32_t Sample = int(float(RawSample) * m_fVolume);

		if (Sample > 32767)
			Sample = 32767;
		if (Sample < -32768)
			Sample = -32768;

		m_pBuffer[m_iBufferPtr++] = int16_t((Sample + LastSample) >> 1);
		LastSample = Sample;
	}

	m_pMixer->MixSamples((blip_sample_t*)m_pBuffer, WantSamples);

	m_iBufferPtr -= WantSamples;
	m_iTime = 0;
}

void CVRC7::Process(uint32_t Time)
{
	// This cannot run in sync, fetch all samples at end of frame instead
	m_iTime += Time;
}

double CVRC7::GetFreq(int Channel) const		// // //
{
	if (Channel < 0 || Channel >= 6) return 0.;
	int Lo = m_pRegisterLogger->GetRegister(Channel | 0x10)->GetValue();
	int Hi = m_pRegisterLogger->GetRegister(Channel | 0x20)->GetValue() & 0x0F;
	Lo |= (Hi << 8) & 0x100;
	Hi >>= 1;
	return 49716. * Lo / (1 << (19 - Hi));
}
