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
#include "../Common.h"
#include "APU.h"
#include "N163.h"
#include "../RegisterState.h"		// // //
#define _USE_MATH_DEFINES
#include <math.h>		// !! !! M_PI

//
// Namco 163
//

// N163 interface, emulation is in Namco163Sound.h

CN163::CN163()
{
	// internal N163 RAM
	m_pRegisterLogger->AddRegisterRange(0x00, 0x7F);		// // //
	Reset();
}

CN163::~CN163()
{
}

void CN163::Reset()
{
	m_N163.Reset();
	m_N163.SetMixing(m_bUseLinearMixing);

	m_iTime = 0;
	m_SynthN163.clear();
	m_BlipN163.clear();
}

void CN163::UpdateFilter(blip_eq_t eq)
{
	m_BlipN163.set_sample_rate(eq.sample_rate);
	m_SynthN163.treble_eq(eq);
	m_BlipN163.bass_freq(0);
	m_CutoffHz = 12000;
	RecomputeN163Filter();
}

void CN163::SetClockRate(uint32_t Rate)
{
	m_BlipN163.clock_rate(Rate);
}

void CN163::Write(uint16_t Address, uint8_t Value)
{
	m_N163.WriteRegister(Address, Value);
}

uint8_t CN163::Read(uint16_t Address, bool &Mapped)
{
	// Addresses for N163
	Mapped = (0x4800 <= Address && Address <= 0x4FFF);
	return m_N163.ReadRegister(Address);
}

void CN163::Process(uint32_t Time, Blip_Buffer& Output)
{
	// Mix level will dynamically change based on number of channels
	if (!m_UseSurveyMix) {
		int channels = m_N163.GetNumberOfChannels();
		double N163_volume = (channels == 0) ? 1.3f : (1.5f + float(channels) / 1.5f);
		N163_volume *= m_Attenuation;
		m_SynthN163.volume(N163_volume * 1.1, 1600);
	}

	uint32_t now = 0;

	while (true) {
		assert(now <= Time);
		if (now >= Time)
			break;


		auto tmp = m_N163.ClockAudioMaxSkip();
		auto clock_skip = std::min(tmp, Time - now);
		if (clock_skip > 0) {
			m_N163.SkipClockAudio(clock_skip);
			now += clock_skip;
		}

		if (tmp < (1 << 24))
			assert(tmp - m_N163.ClockAudioMaxSkip() == clock_skip);
		assert(now <= Time);
		if (now >= Time)
			break;

		// output master audio
		auto master_out = m_N163.ClockAudio() * -1;
		m_SynthN163.update(m_iTime + now, master_out, &m_BlipN163);
			
		// update the channel levels
		for (int i = 0; i < 8; i++)
			m_ChannelLevels[i].update((int32_t) m_N163._channelOutput[7 - i]);

		now++;
	}

	m_iTime += Time;

}

void CN163::EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer)
{
	// log phase registers
	for (int i = 0; i < 8; ++i) {
		if (i <= m_N163.GetNumberOfChannels())
			for (int j : {1, 3, 5}) {
				int RAMAddress = 0x78 - i * 8 + j;
				m_pRegisterLogger->SetPort(RAMAddress);
				m_pRegisterLogger->Write(m_N163._internalRam[RAMAddress]);
			}
	}

	m_BlipN163.end_frame(m_iTime);

	ASSERT(size_t(m_BlipN163.samples_avail()) <= TempBuffer.size());

	auto nsamp_read = m_BlipN163.read_samples(TempBuffer.data(), m_BlipN163.samples_avail());

	auto unfilteredData = TempBuffer.subspan(0, nsamp_read);

	for (auto& amplitude : unfilteredData) {
		float out = m_lowPassState + m_alpha * (float(amplitude) - m_lowPassState);
		amplitude = (int16_t)roundf(out);
		m_lowPassState = out + 1e-18f;  // prevent denormal numbers
	}

	Output.mix_samples_raw(unfilteredData.data(), static_cast<blip_nsamp_t>(unfilteredData.size()));

	m_iTime = 0;
}

double CN163::GetFreq(int Channel) const
{
	double freq = 0.0;
	if (0 <= Channel && Channel < 8) {
		return m_N163.GetChannelFrequency(7 - Channel, CAPU::BASE_FREQ_NTSC);
	}
	return 0.0;
}

int CN163::GetChannelLevel(int Channel)
{
	ASSERT(0 <= Channel && Channel < 8);
	if (0 <= Channel && Channel < 8) {
		return m_ChannelLevels[Channel].getLevel();
	}
	return 0;
}

int CN163::GetChannelLevelRange(int Channel) const
{
	ASSERT(0 <= Channel && Channel < 8);
	if (0 <= Channel && Channel < 8) {
		// _channelOutput[channel] = (sample - 8) * volume;
		// lowest output: -120
		// highest output: 105
		return 225;
	}
	return 1;
}

void CN163::UpdateN163Filter(int CutoffHz, bool DisableMultiplex)
{
	SetMixingMethod(DisableMultiplex);
	m_CutoffHz = CutoffHz;
	RecomputeN163Filter();
}

void CN163::UpdateMixLevel(double v, bool UseSurveyMix)
{
	m_Attenuation = v;
	m_UseSurveyMix = UseSurveyMix;
	if (UseSurveyMix)
		m_SynthN163.volume(m_Attenuation, 225);
	// Legacy mixing recalculates chip levels at runtime
}

void CN163::Log(uint16_t Address, uint8_t Value)		// // //
{
	switch (Address) {
	case 0xF800:
		m_pRegisterLogger->SetPort(Value & 0x7F);
		m_pRegisterLogger->SetAutoincrement((Value & 0x80) != 0);
		break;
	case 0x4800:
		m_pRegisterLogger->Write(Value);
		break;
	}
}

void CN163::SetMixingMethod(bool bLinear)		// // //
{
	m_bUseLinearMixing = bLinear;
	m_N163.SetMixing(m_bUseLinearMixing);
}

void CN163::RecomputeN163Filter()
{
	// Code taken from FDS.cpp
	auto sampleRate_hz = float(m_BlipN163.sample_rate());
	auto cutoff_rad = M_PI * 2 * (float)m_CutoffHz / sampleRate_hz;

	m_alpha = 1 - (float)std::exp(-cutoff_rad);
}