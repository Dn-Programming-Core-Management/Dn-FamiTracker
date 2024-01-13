/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "../stdafx.h"
#include "../FamiTracker.h"
#include "../Settings.h"
#include "APU.h"
#include "VRC7.h"
#include "../RegisterState.h"		// // //

const float  CVRC7::AMPLIFY = 4.6f;		// Mixing amplification, VRC7 patch 14 is 4, 88 times stronger than a 50 % square @ v = 15
const uint32_t CVRC7::OPLL_CLOCK = CAPU::BASE_FREQ_VRC7;	// Clock frequency

CVRC7::CVRC7()
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
	m_BlipVRC7.clear();
	if (m_pOPLLInt != NULL) {
		// update patchset and OPLL type
		OPLL_setChipType(m_pOPLLInt, ((m_UseExternalOPLLChip || m_PatchSelection > 6) ? 0 : 1));

		if (m_UseExternalOPLLChip && m_PatchSet != NULL)
			OPLL_setPatch(m_pOPLLInt, m_PatchSet);
		else
			OPLL_resetPatch(m_pOPLLInt, m_PatchSelection);

		OPLL_reset(m_pOPLLInt);
	}
}

void CVRC7::UpdateFilter(blip_eq_t eq)
{
	m_SynthVRC7.treble_eq(eq);
	m_BlipVRC7.set_sample_rate(eq.sample_rate);
	m_BlipVRC7.bass_freq(0);
}

void CVRC7::SetClockRate(uint32_t Rate)
{
	m_BlipVRC7.clock_rate(Rate);
}

void CVRC7::SetSampleSpeed(uint32_t SampleRate, double ClockRate, uint32_t FrameRate)
{
	if (m_pOPLLInt != NULL) {
		OPLL_delete(m_pOPLLInt);
	}

	m_pOPLLInt = OPLL_new(OPLL_CLOCK, SampleRate);

	OPLL_reset(m_pOPLLInt);

	m_iMaxSamples = (SampleRate / FrameRate) * 2;	// Allow some overflow

	SAFE_RELEASE_ARRAY(m_pBuffer);
	m_pBuffer = new int16_t[m_iMaxSamples];
	memset(m_pBuffer, 0, sizeof(int16_t) * m_iMaxSamples);
}

void CVRC7::SetDirectVolume(double Volume)
{
	m_DirectVolume = Volume;
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

void CVRC7::Process(uint32_t Time, Blip_Buffer& Output)
{
	// This cannot run in sync, fetch all samples at end of frame instead
	m_iTime += Time;
}

void CVRC7::EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer)
{
	uint32_t WantSamples = Output.count_samples(m_iTime);

	static int32_t LastSample = 0;

	// Generate VRC7 samples
	while (m_iBufferPtr < WantSamples) {
		int32_t RawSample = OPLL_calc(m_pOPLLInt);

		// emu2413's waveform output ranges from -4095...4095
		// fully rectified by abs(), so resulting waveform is around 0-4095
		for (int i = 0; i < 6; i++)
			m_ChannelLevels[i].update(static_cast<uint8_t>((255.0 * (OPLL_getchanvol(i) + 1.0)/4096.0)));

		// Apply direct volume, hacky workaround
		int32_t Sample = static_cast<int32_t>(double(RawSample) * m_DirectVolume);

		if (Sample > 32767)
			Sample = 32767;
		if (Sample < -32768)
			Sample = -32768;

		m_pBuffer[m_iBufferPtr++] = int16_t((Sample + LastSample) >> 1);
		LastSample = Sample;
	}

	Output.mix_samples((blip_amplitude_t*)m_pBuffer, WantSamples);

	m_iBufferPtr -= WantSamples;

	m_iTime = 0;
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

int CVRC7::GetChannelLevel(int Channel)
{
	ASSERT(0 <= Channel && Channel < 9);
	if (0 <= Channel && Channel < 6) {
		return m_ChannelLevels[Channel].getLevel();
	}
	return 0;
}

int CVRC7::GetChannelLevelRange(int Channel) const
{
	return 127;
}

void CVRC7::UpdateMixLevel(double v, bool UseSurveyMix)
{
	// The output of emu2413 is resampled. This means
	// that the emulator output suffers no multiplex hiss and
	// bit depth quantization.
	// TODO: replace emu2413 with Nuked-OPLL for better multiplexing accuracy?

	// hacky solution, since VRC7 uses asynchronous direct buffer writes
	SetDirectVolume(UseSurveyMix ? v : (v * AMPLIFY));
	
	// emu2413's waveform output ranges from -4095...4095
	m_SynthVRC7.volume(v, UseSurveyMix ? 8191 : 10000);
}

void CVRC7::UpdatePatchSet(int PatchSelection, bool UseExternalOPLLChip, uint8_t* PatchSet)
{
	m_PatchSelection = PatchSelection;
	m_UseExternalOPLLChip = UseExternalOPLLChip;
	m_PatchSet = PatchSet;
}
