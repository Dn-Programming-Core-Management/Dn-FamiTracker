/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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

 Based on findings from the NESDev Wiki: https://wiki.nesdev.org/w/index.php?title=Namco_163_audio#Mixing
 Along with plgDavid's list of details on various N163 cartridge details: https://docs.google.com/spreadsheets/d/12qGAqMeQFleSPEIZFlxrPzH3MTRKXEa1LIQPXSFD4tk

 N163:
	Varies depending on die revision and components of the cartridge.
	TODO: implement INES 2.0 submapper levels
 
 5B is still unknown

*/

#include "../stdafx.h"
#include <memory>
#include <cmath>
#include "Mixer.h"
#include "APU.h"
#include "2A03.h"
#include "FDS.h"
#include "N163.h"
#include "nsfplay/xgm/devices/Sound/legacy/emu2413.h"
#include "utils/variadic_minmax.h"

//#define LINEAR_MIXING

static const float LEVEL_FALL_OFF_RATE	= 0.6f;
static const int   LEVEL_FALL_OFF_DELAY = 3;

CMixer::CMixer(CAPU * Parent)
	: m_APU(Parent)
{
	memset(m_iChannels, 0, sizeof(int32_t) * CHANNELS);
	memset(m_fChannelLevels, 0, sizeof(float) * CHANNELS);
	memset(m_iChanLevelFallOff, 0, sizeof(uint32_t) * CHANNELS);

	m_fLevelAPU1 = 1.0f;
	m_fLevelAPU2 = 1.0f;
	m_fLevelVRC6 = 1.0f;
	m_fLevelVRC7 = 1.0f;
	m_fLevelMMC5 = 1.0f;
	m_fLevelFDS = 1.0f;
	m_fLevelN163 = 1.0f;
	m_fLevelS5B = 1.0f;		// // // 050B

	m_iExternalChip = 0;
	m_iSampleRate = 0;

	m_iMeterDecayRate = DECAY_SLOW;		// // // 050B
}

CMixer::~CMixer()
{
}

void CMixer::ExternalSound(int Chip)
{
	m_iExternalChip = Chip;
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
		case CHIP_LEVEL_VRC7: case CHIP_LEVEL_COUNT:
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

void CMixer::RecomputeMixing()
{
	auto LowCut = m_MixerConfig.LowCut;
	auto HighCut = m_MixerConfig.HighCut;
	auto HighDamp = m_MixerConfig.HighDamp;

	float Volume = m_MixerConfig.OverallVol * GetAttenuation();

	// Blip-buffer filtering
	BlipBuffer.bass_freq(LowCut);

	blip_eq_t eq(-HighDamp, HighCut, m_iSampleRate);

	SynthVRC6.treble_eq(eq);
	SynthMMC5.treble_eq(eq);
	SynthS5B.treble_eq(eq);

	// Volume levels
	auto &chip2A03 = *m_APU->m_p2A03;
	auto &chipVRC7 = *m_APU->m_pVRC7;
	auto &chipFDS = *m_APU->m_pFDS;
	auto &chipN163 = *m_APU->m_pN163;

	// Prioritize updating the emulation config before eq filter config for N163
	chipN163.UpdateN163Filter(m_MixerConfig.N163Lowpass, m_EmulatorConfig.N163DisableMultiplexing);

	// See https://docs.google.com/document/d/19vtipTYI-vqL3-BPrE9HPjHmPpkFuIZKvWfevP3Oo_A/edit#heading=h.h70ipevgjbn7
	// for an exploration of how I came to this design.
	for (auto* chip : m_APU->m_SoundChips2) {
		chip->UpdateFilter(eq);
	}

	// Maybe the range argument, as well as the constant factor in the volume,
	// should be supplied by the CSoundChip2 subclass rather than CMixer.
	chip2A03.UpdateMixingAPU1(Volume * m_fLevelAPU1);
	chip2A03.UpdateMixingAPU2(Volume * m_fLevelAPU2);
	chipVRC7.UpdateMixLevel(Volume * m_fLevelVRC7);
	chipFDS.UpdateMixLevel(Volume * m_fLevelFDS);
	chipN163.UpdateMixLevel(Volume * m_fLevelN163);

	chipN163.UpdateN163Filter(m_MixerConfig.N163Lowpass, m_EmulatorConfig.N163DisableMultiplexing);
	chipFDS.UpdateFdsFilter(m_MixerConfig.FDSLowpass);
	chipVRC7.UpdatePatchSet(
		m_EmulatorConfig.VRC7PatchSelection,
		m_EmulatorConfig.UseExternalOPLLChip,
		m_EmulatorConfig.VRC7PatchSet);

	SynthVRC6.volume(Volume * 3.98333f * m_fLevelVRC6, 500);
	SynthMMC5.volume(Volume * 1.18421f * m_fLevelMMC5, 130);
	SynthS5B.volume(Volume * m_fLevelS5B, 1200);  // Not checked
}

int CMixer::GetMeterDecayRate() const		// // // 050B
{
	return m_iMeterDecayRate;
}

void CMixer::SetMeterDecayRate(int Rate)		// // // 050B
{
	m_iMeterDecayRate = Rate;
}

bool CMixer::AllocateBuffer(unsigned int BufferLength, uint32_t SampleRate, uint8_t NrChannels)
{
	m_iSampleRate = SampleRate;
	BlipBuffer.set_sample_rate(SampleRate, (BufferLength * 1000 * 2) / SampleRate);

	// I don't know if BlipFDS is initialized or not.
	// So I copied the above call to CMixer::UpdateSettings().
	return true;
}

void CMixer::SetClockRate(uint32_t Rate)
{
	// Change the clockrate
	BlipBuffer.clock_rate(Rate);

	// Propagate the change to any sound chips with their own Blip_Buffer.
	// Note that m_APU->m_SoundChips2 may not have been initialized yet,
	// so CAPU::SetExternalSound() does the same thing.
	for (auto * chip : m_APU->m_SoundChips2) {
		chip->SetClockRate(Rate);
	}
}

void CMixer::ClearBuffer()
{
	BlipBuffer.clear();

	// What about CSoundChip2 which owns its own Blip_Synth?
	// I've decided that CMixer should not be responsible for clearing those Blip_Synth,
	// but rather CSoundChip2::Reset() should do so.
	//
	// This works because CMixer::ClearBuffer() is only called by CAPU::Reset(),
	// which also calls CSoundChip2::Reset() on each sound chip.

	#define X(SYNTH)  SYNTH.clear();
	FOREACH_SYNTH(X, );
	#undef X
}

int CMixer::SamplesAvail() const
{
	return (int)BlipBuffer.samples_avail();
}

static int get_channel_level(CSoundChip2& chip, int channel) {
	int max = chip.GetChannelLevelRange(channel);
	int level = chip.GetChannelLevel(channel);

	// Clip out-of-bounds levels to the maximum allowed on the meter.
	level = min(level, max);

	int out = level * 16 / (max + 1);
	ASSERT(0 <= out && out <= 15);

	// Ensure that the division process never clips small levels to 0.
	if (level > 0 && out <= 0) {
		out = 1;
	}
	return out;
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

	auto& chip2A03 = *m_APU->m_p2A03;
	for (int i = 0; i < 5; i++)
		StoreChannelLevel(CHANID_SQUARE1 + i, get_channel_level(chip2A03, i));

	auto& chipFDS = *m_APU->m_pFDS;
	StoreChannelLevel(CHANID_FDS, get_channel_level(chipFDS, 0));

	auto& chipVRC7 = *m_APU->m_pVRC7;
	for (int i = 0; i < 6; ++i)
		StoreChannelLevel(CHANID_VRC7_CH1 + i, get_channel_level(chipVRC7, i));

	auto& chipN163 = *m_APU->m_pN163;
	for (int i = 0; i < 8; i++)
		StoreChannelLevel(CHANID_N163_CH1 + i, get_channel_level(chipN163, i));
}

//
// Mixing
//

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
