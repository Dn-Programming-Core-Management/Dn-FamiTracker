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


#pragma once

#include "SoundChip2.h"
#include "ChannelLevelState.h"
#include "Blip_Buffer/Blip_Buffer.h"
#include "APU/mesen/FdsAudio.h"
#include "FamiTracker.h"
#include "Settings.h"

class CMixer;

class CFDS : public CSoundChip2 {
public:
	CFDS();
	virtual ~CFDS();
	void	Reset() override;
	void UpdateFilter(blip_eq_t eq) override;
	void SetClockRate(uint32_t Rate) override;
	void	Write(uint16_t Address, uint8_t Value) override;
	uint8_t	Read(uint16_t Address, bool &Mapped) override;
	void	Process(uint32_t Time, Blip_Buffer& Output) override;
	void	EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer) override;
	double	GetFreq(int Channel) const override;		// // //
	int GetChannelLevel(int Channel) override;
	int GetChannelLevelRange(int Channel) const override;

	double	GetModFreq() const;

	void UpdateFdsFilter(int CutoffHz);
	void UpdateMixLevel(double v);

private:
	void RecomputeFdsFilter();

private:
	int m_CutoffHz;

	FdsAudio m_FDS;

	Blip_Buffer m_BlipFDS;
	Blip_Synth<blip_good_quality> m_SynthFDS;

	/// Used for GetChannelLevel().
	ChannelLevelState<uint32_t> m_ChannelLevel;

	// The lower this value is, the stronger the lowpass filter is.
	float m_alpha = 0;
	float m_lowPassState = 0.f;

	uint32_t	m_iTime = 0;  // Clock counter, used as a timestamp for Blip_Buffer, resets every new frame
};
