/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
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

#include "SoundChip2.h"
#include "ChannelLevelState.h"
#include "Blip_Buffer/Blip_Buffer.h"
#include "APU/mesen/Namco163Audio.h"
#include "FamiTracker.h"
#include "Settings.h"

class CN163 : public CSoundChip2 {
public:
	CN163();
	virtual	~CN163();
	void	Reset() override;
	void UpdateFilter(blip_eq_t eq) override;
	void SetClockRate(uint32_t Rate) override;
	void	Write(uint16_t Address, uint8_t Value) override;
	uint8_t	Read(uint16_t Address, bool &Mapped) override;
	void	Process(uint32_t Time, Blip_Buffer& Output) override;
	void	EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer) override;
	double	GetFreq(int Channel) const override;
	int GetChannelLevel(int Channel) override;
	int GetChannelLevelRange(int Channel) const override;

	void UpdateN163Filter(int CutoffHz, bool DisableMultiplex);
	void UpdateMixLevel(double v, bool UseSurveyMix = false);

	void Log(uint16_t Address, uint8_t Value) override;		// // //

	void SetMixingMethod(bool bLinear);		// // //

private:
	void RecomputeN163Filter();

	int m_CutoffHz;

	// master volume attenuation
	double m_Attenuation;
	bool m_UseSurveyMix = false;

	Namco163Audio m_N163;

	Blip_Buffer m_BlipN163;
	Blip_Synth<blip_good_quality> m_SynthN163;

	// up to 8 channels of N163
	ChannelLevelState<int32_t> m_ChannelLevels[8];

	// The lower this value is, the stronger the lowpass filter is.
	float m_alpha = 0;
	float m_lowPassState = 0.f;

	uint32_t	m_iTime = 0;  // Clock counter, used as a timestamp for Blip_Buffer, resets every new frame

	int32_t m_iChannelSample[8];
	bool m_bUseLinearMixing = false;		// // //
};
