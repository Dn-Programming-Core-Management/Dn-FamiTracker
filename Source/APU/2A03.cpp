/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
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
#include "../Common.h"
#include <algorithm>  // std::min
#include "APU.h"
#include "2A03.h"
#include "../RegisterState.h"		// // //
#include "utils/variadic_minmax.h"
#include "nsfplay/xgm/devices/devinfo.h"		// // !!

// // // 2A03 sound chip class

C2A03::C2A03() {
	m_pRegisterLogger->AddRegisterRange(0x4000, 0x4017);		// // //

	// Disable nondeterministic behavior.
	// APU2 triangle causes a pop when playback begins.
	// This can be fixed by adding a Blip_Synth method
	// to not generate sound on the first update.
	m_Apu2.SetOption(xgm::NES_DMC::OPT_RANDOMIZE_TRI, 0);
	m_Apu2.SetOption(xgm::NES_DMC::OPT_RANDOMIZE_NOISE, 0);

	// Reset() is called by CAPU::SetExternalSound(), but let's call it ourself.
	Reset();
}

// TODO set range of Blip_Synth. Copy whatever CMixer does.

void C2A03::Reset()
{
	m_Apu2.SetAPU(&m_Apu1);
	m_Apu2.SetMemory(&m_SampleMem);

	m_Apu1.Reset();
	m_Apu2.Reset();

	Synth2A03SS.clear();
	Synth2A03TND.clear();

	// nsfplay's triangle output is nonzero after being reset. Cancel it out.
	{
		// Tick(0) is valid. https://github.com/bbbradsmith/nsfplay/commit/14cb23159584427053a6e5456bb1f9ce8d0918d5
		m_Apu2.Tick(0);

		int32_t out[2];
		m_Apu2.Render(&out[0]);

		// Eliminate pop in speakers.
		Synth2A03TND.center_dc(out[0]);

		// Eliminate pop in volume meters.
		m_ChannelLevels[2].update(m_Apu2.out[0]);
		m_ChannelLevels[2].getLevel();
	}
}

void C2A03::UpdateFilter(blip_eq_t eq)
{
	Synth2A03SS.treble_eq(eq);
	Synth2A03TND.treble_eq(eq);
}

void C2A03::Process(uint32_t Time, Blip_Buffer& Output)
{
	uint32_t now = 0;

	auto get_output = [this](uint32_t dclocks, uint32_t now, Blip_Buffer& blip_buf) {
		m_Apu2.TickFrameSequence(dclocks);
		m_Apu1.Tick(dclocks);
		m_Apu2.Tick(dclocks);

		int32_t out[2];
		m_Apu1.Render(out);
		Synth2A03SS.update(m_iTime + now, out[0], &blip_buf);

		m_Apu2.Render(out);
		Synth2A03TND.update(m_iTime + now, out[0], &blip_buf);

		// pulse 1/2
		m_ChannelLevels[0].update(m_Apu1.out[0]);
		m_ChannelLevels[1].update(m_Apu1.out[1]);

		// tri/noise/dpcm
		m_ChannelLevels[2].update(m_Apu2.out[0]);
		m_ChannelLevels[3].update(m_Apu2.out[1]);
		m_ChannelLevels[4].update(m_Apu2.out[2]);
	};

	while (now < Time) {
		// Due to how nsfplay is implemented, when ClocksUntilLevelChange() is used,
		// the result of `Tick(clocks); Render()` should be sent to Blip_Synth
		// at the instant in time *before* Tick() is called.
		// See https://docs.google.com/document/d/1BnXwR3Avol7S5YNa3d4duGdbI6GNMwuYWLHuYiMZh5Y/edit#heading=h.lnh9d8j1x3uc
		auto dclocks = vmin(
			m_Apu1.ClocksUntilLevelChange(),
			m_Apu2.ClocksUntilLevelChange(),
			Time - now);
		get_output(dclocks, now, Output);
		now += dclocks;
	}

	m_iTime += Time;
}

void C2A03::EndFrame(Blip_Buffer&, gsl::span<int16_t>)
{
	m_iTime = 0;
}

void C2A03::Write(uint16_t Address, uint8_t Value)
{
	m_Apu1.Write(Address, Value);
	m_Apu2.Write(Address, Value);
}

uint8_t C2A03::Read(uint16_t Address, bool &Mapped)
{
	switch (Address) {
	case 0x4015:
	{
		uint32_t out = 0;

		// Each Read() call updates different bits in the byte.
		Mapped |= m_Apu1.Read(Address, /*mut*/ out);
		Mapped |= m_Apu2.Read(Address, /*mut*/ out);
		ASSERT((out & 0xFF) == out);
		return out;
	}
	}
	return 0U;
}

double C2A03::GetFreq(int Channel) const		// // !!
{
	switch (Channel) {
	case 0:
		return m_Apu1.GetFrequencyPulse1();
	case 1:
		return m_Apu1.GetFrequencyPulse2();
	case 2:
		return m_Apu2.GetFrequencyTriangle();
	case 3:
		return m_Apu2.GetFrequencyNoise();
	case 4:
		return m_Apu2.GetFrequencyDPCM();
	}
	return 0.0;
}

int C2A03::GetChannelLevel(int Channel)
{
	ASSERT(0 <= Channel && Channel < 5);
	if (0 <= Channel && Channel < 5) {
		return m_ChannelLevels[Channel].getLevel();
	}
	return 0;
}

int C2A03::GetChannelLevelRange(int Channel) const
{
	ASSERT(0 <= Channel && Channel < 5);
	switch (Channel) {
	case 0: case 1: case 2: case 3:
		// pulse/tri/noise
		return 15;

	case 4:
		// dpcm
		return 127;

	default:
		// unknown channel, return 1 to avoid division by 0
		return 1;
	}
}


void C2A03::UpdateMixingAPU1(double v) {
	// NSFPlay output waveform ranges from 0 - 8191
	// should not affect legacy mixing
	Synth2A03SS.volume(v, 8191);
}

void C2A03::UpdateMixingAPU2(double v) {
	// NSFPlay output waveform ranges from 0 - 8191
	// should not affect legacy mixing
	Synth2A03TND.volume(v, 8191);
}

void C2A03::ClockSequence()
{
	// Do nothing. It's handled by nsfplay xgm.
	// TODO do not interrupt Process() calls when the frame sequencer runs.
	// TODO remove frame sequencer code from overall CAPU,
	// since the 2A03 and MMC5 have electrically separate 240Hz clocks
	// and each chip should handle it itself.
}

void C2A03::ChangeMachine(int Machine)
{
	switch (Machine) {
		case MACHINE_NTSC:
			m_Apu1.SetClock(CAPU::BASE_FREQ_NTSC);
			m_Apu2.SetClock(CAPU::BASE_FREQ_NTSC);
			m_Apu2.SetPal(false);
			break;
		case MACHINE_PAL:
			m_Apu1.SetClock(CAPU::BASE_FREQ_PAL);
			m_Apu2.SetClock(CAPU::BASE_FREQ_PAL);
			m_Apu2.SetPal(true);
			break;
	}
}

CSampleMem *C2A03::GetSampleMemory()		// // //
{
	return &m_SampleMem;
}

uint8_t C2A03::GetSamplePos() const
{
	return m_Apu2.GetSamplePos();
}

uint8_t C2A03::GetDeltaCounter() const
{
	return m_Apu2.GetDeltaCounter();
}

bool C2A03::DPCMPlaying() const
{
	return m_Apu2.IsPlaying();
}
