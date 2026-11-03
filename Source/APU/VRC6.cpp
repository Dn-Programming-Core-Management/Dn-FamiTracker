/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2026 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/

#include "APU.h"
#include "VRC6.h"
#include "../RegisterState.h"

// Konami VRC6 external sound chip emulation

CVRC6::CVRC6()
{
	// VRC6 mapped registers
	m_pRegisterLogger->AddRegisterRange(0x9000, 0x9003);
	m_pRegisterLogger->AddRegisterRange(0xA000, 0xA002);
	m_pRegisterLogger->AddRegisterRange(0xB000, 0xB002);
}

void CVRC6::Reset()
{
	m_VRC6.Reset();

	m_SynthVRC6.clear();
}

void CVRC6::UpdateFilter(blip_eq_t eq)
{
	m_SynthVRC6.treble_eq(eq);
}

void CVRC6::Write(uint16_t Address, uint8_t Value)
{
	// VRC6 internal audio registers are freely exposed and mapped to memory.
	m_VRC6.Write(Address, Value);
}

uint8_t CVRC6::Read(uint16_t Address, bool& Mapped)
{
	// VRC6 internal audio registers are write-only.
	// Reading from this area is reading from mapped cartridge ROM instead.
	Mapped = false;
	return 0;
}

void CVRC6::EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer)
{
	m_iTime = 0;
}

// Clock the emulation core and output to the buffer.
void CVRC6::Process(uint32_t Time, Blip_Buffer& Output)
{
	uint32_t now = 0;

	auto get_output = [this](uint32_t dclocks, uint32_t now, Blip_Buffer& blip_buf) {
		m_VRC6.Tick(dclocks);

		int32_t out[2];
		m_VRC6.Render(out);
		m_SynthVRC6.update(m_iTime + now, out[0], &blip_buf);

		assert(out[0] >= -(15 + 15 + 31));

		m_ChannelLevels[0].update(m_VRC6.out[0]);
		m_ChannelLevels[1].update(m_VRC6.out[1]);
	};

	while (now < Time) {
		auto dclocks = vmin(m_VRC6.ClocksUntilLevelChange(), Time - now);
		get_output(dclocks, now, Output);
		now += dclocks;
	}

	m_iTime += Time;
}

double CVRC6::GetFreq(int Channel) const		// // //
{
	return m_VRC6.GetFreq(Channel);
}

int CVRC6::GetChannelLevel(int Channel)
{
	//     Pulse volumes                                        Sawtooth volume
	return Channel <= 1 ? m_ChannelLevels[Channel].getLevel() : m_VRC6.volume[2] >> 1;
}

int CVRC6::GetChannelLevelRange(int Channel) const
{
	return 15;
}

void CVRC6::UpdateMixLevel(double v, bool UseSurveyMix)
{
	m_SynthVRC6.volume(v, UseSurveyMix ? (15 + 15 + 31) : 500);
}