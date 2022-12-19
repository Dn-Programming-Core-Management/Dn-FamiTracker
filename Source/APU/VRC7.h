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

#include "digital-sound-antiques/emu2413.h"

class CVRC7 : public CSoundChip2 {
public:
	CVRC7();
	virtual ~CVRC7();

	void Reset() override;
	void UpdateFilter(blip_eq_t eq) override;
	void SetClockRate(uint32_t Rate) override;
	void Process(uint32_t Time, Blip_Buffer& Output) override;
	void EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer) override;

	void Write(uint16_t Address, uint8_t Value) override;
	uint8_t Read(uint16_t Address, bool& Mapped) override;

	double GetFreq(int Channel) const override;		// // //
	int GetChannelLevel(int Channel) override;
	int GetChannelLevelRange(int Channel) const override;

	void SetSampleSpeed(uint32_t SampleRate, double ClockRate, uint32_t FrameRate);
	void SetDirectVolume(double Volume);
	void Log(uint16_t Address, uint8_t Value) override;		// // //

	void UpdateMixLevel(double v, bool UseSurveyMix = false);

	void UpdatePatchSet(int PatchSelection, bool UseExternalOPLLChip, uint8_t *PatchSet);

protected:
	static const float  AMPLIFY;
	static const uint32_t OPLL_CLOCK;

private:
	OPLL		*m_pOPLLInt = NULL;
	uint32_t	m_iTime;
	uint32_t	m_iMaxSamples = 0;

	int16_t		*m_pBuffer = NULL;
	uint32_t	m_iBufferPtr;

	uint8_t		m_iSoundReg = 0;

	double		m_DirectVolume = 1.0f;

	// [0..8] corresponds to the 9 channels of the YM2413. Future support for percussion mode?
	ChannelLevelState<uint8_t> m_ChannelLevels[9];

	Blip_Buffer	m_BlipVRC7;
	Blip_Synth<blip_good_quality> m_SynthVRC7;
};
