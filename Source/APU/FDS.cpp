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
#include "FDS.h"
#include "../RegisterState.h"		// // //

#include <cstdint>
#include <cmath>

// FDS interface, actual FDS emulation is in FDSSound.cpp

CFDS::CFDS()
{
	m_pRegisterLogger->AddRegisterRange(0x4040, 0x408F);		// // //

	// Reset() is called by CAPU::SetExternalSound(), but let's call it ourself.
	Reset();
}

CFDS::~CFDS()
{
}

void CFDS::Reset()
{
	m_FDS.Reset();

	m_SynthFDS.clear();
	m_BlipFDS.clear();
}

constexpr float TWOPI = 2 * 3.141592653589793238462643383279502884f;

void CFDS::UpdateFilter(blip_eq_t eq)
{
	m_SynthFDS.treble_eq(eq); // Apply 2A03 global EQ on top of FDS's dedicated lowpass.

	// This line of code was copied from CMixer::AllocateBuffer(),
	// which does some math in order to calculate the length of the Blip_Buffer.
	// What nobody realized is that the value passed in is *always* 125 ms,
	// modulo rounding error:
	//
	// - CAPU::SetupSound() converts 16 Hz (the minimum legal frame rate) into samples.
	// - CMixer::AllocateBuffer() multiplies by 2 and converts samples into milliseconds.
	// - Blip_Buffer::set_sample_rate() converts milliseconds into samples.
	//
	// just... wtf.
	//
	// We can't access the length of the global Blip_Buffer, so leave the length as default.
	// The default value of 250 ms is more than enough.
	m_BlipFDS.set_sample_rate(eq.sample_rate);

	// BlipFDS is used to render FDS.
	// The output is lowpassed and fed directly to the global Blip_Buffer.
	// The global Blip_Buffer performs bass removal but not treble removal.
	// So BlipFDS should skip bass removal.
	m_BlipFDS.bass_freq(0);

	// Compute first-order lowpass coefficient from FDS cutoff frequency and sampling rate.
	auto sampleRate_hz = float(eq.sample_rate);
	auto cutoff_hz = 2000.f; // TODO configurable cutoff frequency
	auto cutoff_rad = TWOPI * cutoff_hz / sampleRate_hz;

	// This formula is approximate, but good enough because the FDS cutoff frequency is small
	// compared to the audio sampling rate.
	// Despite the exponential, this formula will never blow up
	// because -cutoff_rad is negative, so e^(-cutoff_rad) lies between 0 and 1.
	m_alpha = 1 - std::exp(-cutoff_rad);
}

void CFDS::SetClockRate(uint32_t Rate)
{
	m_BlipFDS.clock_rate(Rate);
}

void CFDS::Write(uint16_t Address, uint8_t Value)
{
	m_FDS.WriteRegister(Address, Value);
}

uint8_t CFDS::Read(uint16_t Address, bool &Mapped)
{
	Mapped = ((0x4040 <= Address && Address <= 0x407f) || (0x4090 == Address) || (0x4092 == Address));
	return m_FDS.ReadRegister(Address);
}

void CFDS::Process(uint32_t Time, Blip_Buffer& Output)
{
	uint32_t now = 0;

	while (true) {
		assert(now <= Time);
		if (now >= Time)
			break;

		auto tmp = m_FDS.ClockAudioMaxSkip();
		auto clock_skip = std::min(tmp, Time - now);
		if (clock_skip > 0) {
			m_FDS.SkipClockAudio(clock_skip);
			now += clock_skip;
		}

		if (tmp < (1 << 24))
			assert(tmp - m_FDS.ClockAudioMaxSkip() == clock_skip);
		assert(now <= Time);
		if (now >= Time)
			break;

		uint8_t out = m_FDS.ClockAudio();
		m_SynthFDS.update(m_iTime + now, out, &m_BlipFDS);
		now++;
	}

	m_iTime += Time;

}

void CFDS::EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer)
{
	// Precondition: m_iTime matches the clock-duration of this frame (CAPU::m_iFrameCycles).
	// I'm not sure if this is true.
	m_BlipFDS.end_frame(m_iTime);

	ASSERT(size_t(m_BlipFDS.samples_avail()) <= TempBuffer.size());

	// We need to read samples into a sound buffer of length equal to CAPU::m_pSoundBuffer.
	// TempBuffer points to the same memory as CAPU::m_pSoundBuffer (whose contents are not needed).
	auto nsamp_read = m_BlipFDS.read_samples(TempBuffer.data(), m_BlipFDS.samples_avail());

	auto unfilteredData = TempBuffer.subspan(0, nsamp_read);

	// Low-pass FDS output.
	for (auto& amplitude : unfilteredData) {
		float out = m_lowPassState + m_alpha * (float(amplitude) - m_lowPassState);
		amplitude = (int16_t) roundf(out);
		m_lowPassState = out + 1e-18f;  // prevent denormal numbers
	}

	Output.mix_samples(unfilteredData.data(), unfilteredData.size());

	m_iTime = 0;
}

double CFDS::GetFreq(int Channel) const		// // //
{
	if (Channel) return 0.;
	int Lo = m_pRegisterLogger->GetRegister(0x4082)->GetValue();
	int Hi = m_pRegisterLogger->GetRegister(0x4083)->GetValue();
	if (Hi & 0x80)
		return 0.;
	Lo |= (Hi << 8) & 0xF00;
	return CAPU::BASE_FREQ_NTSC * (Lo / 4194304.);
}

void CFDS::UpdateMixLevel(double v, unsigned int range)
{
	m_SynthFDS.volume(v, range);
}
