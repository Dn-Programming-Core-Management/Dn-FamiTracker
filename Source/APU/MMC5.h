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


#pragma once

#include "SoundChip.h"
#include "ChannelLevelState.h"

#include "APU/nsfplay/xgm/devices/Sound/nes_mmc5.h"

class CMMC5 : public CSoundChip {
public:
	CMMC5();

	void	Reset() override;
	void	UpdateFilter(blip_eq_t eq) override;
	void	Write(uint16_t Address, uint8_t Value) override;
	uint8_t Read(uint16_t Address, bool &Mapped) override;
	void	Process(uint32_t Time, Blip_Buffer& Output) override;
	void	EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer) override;
	double	GetFreq(int Channel) const override;
	int		GetChannelLevel(int Channel) override;
	int		GetChannelLevelRange(int Channel) const override;
	void	UpdateMixLevel(double v, bool UseSurveyMix);

	// Report some basic information about the chip
	uint8_t GetChannelCount() const override { return 2; };							// TODO: Dynamically calculate this?
	chan_id_t GetFirstChannelID() const override { return CHANID_MMC5_SQUARE1; };	//

private:
	xgm::NES_MMC5 m_MMC5;

	Blip_Synth<blip_good_quality> m_SynthMMC5;

	ChannelLevelState<uint8_t> m_ChannelLevels[2];

	uint32_t m_iTime = 0;
};
