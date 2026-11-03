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

#include "Types.h"
#include "MMC5.h"
#include "../RegisterState.h"		// // //

// MMC5 external sound

CMMC5::CMMC5()
{
	// MMC5 mapped registers
	m_pRegisterLogger->AddRegisterRange(0x5000, 0x5007);
	m_pRegisterLogger->AddRegisterRange(0x5015, 0x5015);
}

void CMMC5::Reset()
{
	m_MMC5.Reset();

	m_SynthMMC5.clear();
}

void CMMC5::UpdateFilter(blip_eq_t eq)
{
	m_SynthMMC5.treble_eq(eq);
}

void CMMC5::Write(uint16_t Address, uint8_t Value)
{
	// MMC5 audio registers are exposed for writing
	m_MMC5.Write(Address, Value);
}

uint8_t CMMC5::Read(uint16_t Address, bool &Mapped)
{
	// MMC5 audio registers are available for reading
	xgm::UINT32 value;
	Mapped = m_MMC5.Read(Address, value);
	return value;
}

void CMMC5::EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer)
{
	m_iTime = 0;
}

// Clock the emulation core and output to the buffer.
void CMMC5::Process(uint32_t Time, Blip_Buffer& Output)
{
	uint32_t now = 0;

	auto get_output = [this](uint32_t dclocks, uint32_t now, Blip_Buffer& blip_buf) {
		m_MMC5.Tick(dclocks);

		int32_t out[2];
		m_MMC5.Render(out);
		m_SynthMMC5.update(m_iTime + now, out[0], &blip_buf);

		m_ChannelLevels[0].update(m_MMC5.out[0]);
		m_ChannelLevels[1].update(m_MMC5.out[1]);
	};

	while (now < Time) {
		auto dclocks = vmin(m_MMC5.ClocksUntilLevelChange(), Time - now);
		get_output(dclocks, now, Output);
		now += dclocks;
	}

	m_iTime += Time;
}

double CMMC5::GetFreq(int Channel) const
{
	return m_MMC5.GetFreq(Channel);
}

int CMMC5::GetChannelLevel(int Channel)
{
	return m_ChannelLevels[Channel].getLevel();
}

int CMMC5::GetChannelLevelRange(int Channel) const
{
	return 15;
}

void CMMC5::UpdateMixLevel(double v, bool UseSurveyMix)
{
	m_SynthMMC5.volume(v, UseSurveyMix ? 8191 : 10000);
}