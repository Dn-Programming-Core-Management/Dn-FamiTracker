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

	// OPLL chip type
	bool m_UseExternalOPLLChip = false;
	// default OPLL patchset
	int m_PatchSelection = 0;
	// pointer to document OPLL patchset
	uint8_t* m_PatchSet = NULL;

	// [0..8] corresponds to the 9 channels of the YM2413. Future support for percussion mode?
	ChannelLevelState<uint8_t> m_ChannelLevels[9];

	Blip_Buffer	m_BlipVRC7;
	Blip_Synth<blip_good_quality> m_SynthVRC7;
};
