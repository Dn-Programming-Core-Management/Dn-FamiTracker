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

/*

 This will mix and synthesize the APU audio using blargg's blip-buffer

 Mixing of internal audio relies on Blargg's findings

 Mixing of external channles are based on my own research:

 VRC6 (Madara): 
	Pulse channels has the same amplitude as internal-
	pulse channels on equal volume levels.

 FDS: 
	Square wave @ v = $1F: 2.4V
	  			  v = $0F: 1.25V
	(internal square wave: 1.0V)

 MMC5 (just breed): 
	2A03 square @ v = $0F: 760mV (the cart attenuates internal channels a little)
	MMC5 square @ v = $0F: 900mV

 VRC7:
	2A03 Square  @ v = $0F: 300mV (the cart attenuates internal channels a lot)
	VRC7 Patch 5 @ v = $0F: 900mV
	Did some more tests and found patch 14 @ v=15 to be 13.77dB stronger than a 50% square @ v=15

 ---

 N163 & 5B are still unknown

*/

#include "../stdafx.h"
#include <memory>
#include <cmath>
#include "Mixer.h"
#include "APU.h"
#include "2A03.h"
#include "nsfplay/xgm/devices/Sound/legacy/emu2413.h"

//#define LINEAR_MIXING

static const float LEVEL_FALL_OFF_RATE	= 0.6f;
static const int   LEVEL_FALL_OFF_DELAY = 3;

CMixer::CMixer()
{
	memset(m_iChannels, 0, sizeof(int32_t) * CHANNELS);
	memset(m_fChannelLevels, 0, sizeof(float) * CHANNELS);
	memset(m_iChanLevelFallOff, 0, sizeof(uint32_t) * CHANNELS);

	m_fLevelAPU1 = 1.0f;
	m_fLevelAPU2 = 1.0f;
	m_fLevelVRC6 = 1.0f;
	m_fLevelMMC5 = 1.0f;
	m_fLevelFDS = 1.0f;
	m_fLevelN163 = 1.0f;
	m_fLevelS5B = 1.0f;		// // // 050B

	m_iExternalChip = 0;
	m_iSampleRate = 0;
	m_iLowCut = 0;
	m_iHighCut = 0;
	m_iHighDamp = 0;
	m_fOverallVol = 1.0f;

	m_iMeterDecayRate = DECAY_SLOW;		// // // 050B
}

CMixer::~CMixer()
{
}

void CMixer::ExternalSound(int Chip)
{
	m_iExternalChip = Chip;
	UpdateSettings(m_iLowCut, m_iHighCut, m_iHighDamp, m_fOverallVol);
}

void CMixer::SetC2A03(C2A03* chip)
{
	VERIFY(chip);
	Synth2A03SS = &chip->GetApu1BlipSynth();
	Synth2A03TND = &chip->GetApu2BlipSynth();
}

void CMixer::SetNamcoMixing(bool bLinear)		// // //
{
	m_bNamcoMixing = bLinear;
}

void CMixer::SetChipLevel(chip_level_t Chip, float Level)
{
	switch (Chip) {
		case CHIP_LEVEL_APU1:
			m_fLevelAPU1 = Level;
			break;
		case CHIP_LEVEL_APU2:
			m_fLevelAPU2 = Level;
			break;
		case CHIP_LEVEL_VRC6:
			m_fLevelVRC6 = Level;
			break;
		case CHIP_LEVEL_MMC5:
			m_fLevelMMC5 = Level;
			break;
		case CHIP_LEVEL_FDS:
			m_fLevelFDS = Level;
			break;
		case CHIP_LEVEL_N163:
			m_fLevelN163 = Level;
			break;
		case CHIP_LEVEL_S5B:		// // // 050B
			m_fLevelS5B = Level;
			break;
	}
}

float CMixer::GetAttenuation() const
{
	const float ATTENUATION_VRC6 = 0.80f;
	const float ATTENUATION_VRC7 = 0.64f;
	const float ATTENUATION_MMC5 = 0.83f;
	const float ATTENUATION_FDS  = 0.90f;
	const float ATTENUATION_N163 = 0.70f;
	const float ATTENUATION_S5B  = 0.50f;		// // // 050B

	float Attenuation = 1.0f;

	// Increase headroom if some expansion chips are enabled

	if (m_iExternalChip & SNDCHIP_VRC6)
		Attenuation *= ATTENUATION_VRC6;

	if (m_iExternalChip & SNDCHIP_VRC7)
		Attenuation *= ATTENUATION_VRC7;

	if (m_iExternalChip & SNDCHIP_MMC5)
		Attenuation *= ATTENUATION_MMC5;

	if (m_iExternalChip & SNDCHIP_FDS)
		Attenuation *= ATTENUATION_FDS;

	if (m_iExternalChip & SNDCHIP_N163)
		Attenuation *= ATTENUATION_N163;

	if (m_iExternalChip & SNDCHIP_S5B)		// // // 050B
		Attenuation *= ATTENUATION_S5B;

	return Attenuation;
}

constexpr int N163_RANGE = 1200;

void CMixer::UpdateSettings(int LowCut,	int HighCut, int HighDamp, float OverallVol)
{
	float Volume = OverallVol * GetAttenuation();

	// Blip-buffer filtering
	BlipBuffer.bass_freq(LowCut);

	blip_eq_t eq(-HighDamp, HighCut, m_iSampleRate);

	VERIFY(Synth2A03SS);
	VERIFY(Synth2A03TND);

	Synth2A03SS->treble_eq(eq);
	Synth2A03TND->treble_eq(eq);
	SynthVRC6.treble_eq(eq);
	SynthMMC5.treble_eq(eq);
	SynthS5B.treble_eq(eq);

	// N163 special filtering
	double n163_treble = 24;
	long n163_rolloff = 12000;

	if (HighDamp > n163_treble)
		n163_treble = HighDamp;

	if (n163_rolloff > HighCut)
		n163_rolloff = HighCut;

	blip_eq_t eq_n163(-n163_treble, n163_rolloff, m_iSampleRate);
	SynthN163.treble_eq(eq_n163);

	// FDS special filtering (TODO fix this for high sample rates)
	blip_eq_t fds_eq(-48, 1000, m_iSampleRate);

	SynthFDS.treble_eq(fds_eq);

	// Volume levels
	Synth2A03SS->volume(Volume * m_fLevelAPU1, 10000);
	Synth2A03TND->volume(Volume * m_fLevelAPU2, 10000);
	SynthVRC6.volume(Volume * 3.98333f * m_fLevelVRC6, 500);
	SynthMMC5.volume(Volume * 1.18421f * m_fLevelMMC5, 130);
	SynthS5B.volume(Volume * m_fLevelS5B, 1600);  // Not checked
	SynthFDS.volume(Volume * 1.00f * m_fLevelFDS, 3500);
	SynthN163.volume(Volume * 1.1f * m_fLevelN163, N163_RANGE);  // Not checked

	m_iLowCut = LowCut;
	m_iHighCut = HighCut;
	m_iHighDamp = HighDamp;
	m_fOverallVol = OverallVol;
}

void CMixer::SetNamcoVolume(float fVol)
{
	float fVolume = fVol * m_fOverallVol * GetAttenuation();

	SynthN163.volume(fVolume * 1.1f * m_fLevelN163, N163_RANGE);
}

int CMixer::GetMeterDecayRate() const		// // // 050B
{
	return m_iMeterDecayRate;
}

void CMixer::SetMeterDecayRate(int Rate)		// // // 050B
{
	m_iMeterDecayRate = Rate;
}

void CMixer::MixSamples(blip_amplitude_t *pBuffer, uint32_t Count)
{
	// For VRC7
	BlipBuffer.mix_samples(pBuffer, Count);
}

uint32_t CMixer::GetMixSampleCount(int t) const
{
	return BlipBuffer.count_samples(t);
}

bool CMixer::AllocateBuffer(unsigned int BufferLength, uint32_t SampleRate, uint8_t NrChannels)
{
	m_iSampleRate = SampleRate;
	BlipBuffer.set_sample_rate(SampleRate, (BufferLength * 1000 * 2) / SampleRate);
	return true;
}

void CMixer::SetClockRate(uint32_t Rate)
{
	// Change the clockrate
	BlipBuffer.clock_rate(Rate);
}

void CMixer::ClearBuffer()
{
	BlipBuffer.clear();

	#define X(SYNTH)  SYNTH.clear();
	FOREACH_SYNTH(X, );
	#undef X
}

int CMixer::SamplesAvail() const
{	
	return (int)BlipBuffer.samples_avail();
}

void CMixer::FinishBuffer(int t)
{
	BlipBuffer.end_frame(t);

	for (int i = 0; i < CHANNELS; ++i) {
		// TODO: this is more complicated than 0.5.0 beta's implementation
		if (m_iChanLevelFallOff[i] > 0) {
			if (m_iMeterDecayRate == DECAY_FAST)		// // // 050B
				m_iChanLevelFallOff[i] = 0;
			else
				--m_iChanLevelFallOff[i];
		}
		else if (m_fChannelLevels[i] > 0) {
			if (m_iMeterDecayRate == DECAY_FAST)		// // // 050B
				m_fChannelLevels[i] = 0;
			else {
				m_fChannelLevels[i] -= LEVEL_FALL_OFF_RATE;
				if (m_fChannelLevels[i] < 0)
					m_fChannelLevels[i] = 0;
			}
		}
	}

	// Get channel levels for VRC7
	for (int i = 0; i < 6; ++i)
		StoreChannelLevel(CHANID_VRC7_CH1 + i, OPLL_getchanvol(i));
}

//
// Mixing
//

void CMixer::MixN163(int Value, int Time)
{
	SynthN163.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixFDS(int Value, int Time)
{
	SynthFDS.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixVRC6(int Value, int Time)
{
	SynthVRC6.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixMMC5(int Value, int Time)
{
	SynthMMC5.offset(Time, Value, &BlipBuffer);
}

void CMixer::MixS5B(int Value, int Time)
{
	SynthS5B.offset(Time, Value, &BlipBuffer);
}

void CMixer::AddValue(int ChanID, int Chip, int Value, int AbsValue, int FrameCycles)
{
	// Add sound to mixer
	//
	
	int Delta = Value - m_iChannels[ChanID];
	StoreChannelLevel(ChanID, AbsValue);
	m_iChannels[ChanID] = Value;

	// Unless otherwise notes, Value is already a delta.
	switch (Chip) {
		case SNDCHIP_NONE:
			// 2A03 nonlinear mixing bypasses CMixer now, and talks directly to BlipBuffer
			// (obtained by CMixer::GetBuffer()).
			break;
		case SNDCHIP_N163:
			MixN163(Value, FrameCycles);
			break;
		case SNDCHIP_FDS:
			MixFDS(Value, FrameCycles);
			break;
		case SNDCHIP_MMC5:
			// Value == AbsValue.
			MixMMC5(Delta, FrameCycles);
			break;
		case SNDCHIP_VRC6:
			MixVRC6(Value, FrameCycles);
			break;
		case SNDCHIP_S5B:		// // // 050B
			MixS5B(Value, FrameCycles);
			break;
	}
}

int CMixer::ReadBuffer(void *Buffer)
{
	return BlipBuffer.read_samples((blip_amplitude_t*)Buffer, BlipBuffer.samples_avail());
}

int32_t CMixer::GetChanOutput(uint8_t Chan) const
{
	return (int32_t)m_fChannelLevels[Chan];
}

void CMixer::StoreChannelLevel(int Channel, int Value)
{
	int AbsVol = abs(Value);

	// Adjust channel levels for some channels
	if (Channel == CHANID_VRC6_SAWTOOTH)
		AbsVol = (AbsVol * 3) / 4;

	if (Channel == CHANID_DPCM)
		AbsVol /= 8;

	if (Channel == CHANID_FDS)
		AbsVol = AbsVol / 38;

	if (Channel >= CHANID_N163_CH1 && Channel <= CHANID_N163_CH8) {
		AbsVol /= 15;
		Channel = (7 - (Channel - CHANID_N163_CH1)) + CHANID_N163_CH1;
	}

	if (Channel >= CHANID_VRC7_CH1 && Channel <= CHANID_VRC7_CH6) {
		AbsVol = (int)(logf((float)AbsVol) * 3.0f);
	}

	if (Channel >= CHANID_S5B_CH1 && Channel <= CHANID_S5B_CH3) {
		AbsVol = (int)(logf((float)AbsVol) * 2.8f);
	}

	if (float(AbsVol) >= m_fChannelLevels[Channel]) {
		m_fChannelLevels[Channel] = float(AbsVol);
		m_iChanLevelFallOff[Channel] = LEVEL_FALL_OFF_DELAY;
	}
}

void CMixer::ClearChannelLevels()
{
	memset(m_fChannelLevels, 0, sizeof(float) * CHANNELS);
	memset(m_iChanLevelFallOff, 0, sizeof(uint32_t) * CHANNELS);
}

uint32_t CMixer::ResampleDuration(uint32_t Time) const
{
	return (uint32_t)BlipBuffer.resampled_duration((blip_nsamp_t)Time);
}
