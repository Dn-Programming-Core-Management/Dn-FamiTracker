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

#include "Types.h"
#include "../Common.h"
#include "../Blip_Buffer/blip_buffer.h"

enum chip_level_t {
	CHIP_LEVEL_APU1,
	CHIP_LEVEL_APU2,
	CHIP_LEVEL_VRC6,
	CHIP_LEVEL_VRC7,
	CHIP_LEVEL_MMC5,
	CHIP_LEVEL_FDS,
	CHIP_LEVEL_N163,
	CHIP_LEVEL_S5B,
	CHIP_LEVEL_COUNT
};

class C2A03;
class CFDS;
class CAPU;

struct MixerConfig {
	int LowCut = 0;
	int HighCut = 0;
	int HighDamp = 0;
	float OverallVol = 0;
	int FDSLowpass = 2000;
	int VRC7Patchset = 0;
};

class CMixer
{
public:
	CMixer(CAPU * Parent);
	~CMixer();

	void	ExternalSound(int Chip);

	void	AddValue(int ChanID, int Chip, int Value, int AbsValue, int FrameCycles);
	void	SetMixing(MixerConfig cfg) {
		m_MixerConfig = cfg;
	}
	void	RecomputeMixing();

	bool	AllocateBuffer(unsigned int Size, uint32_t SampleRate, uint8_t NrChannels);
	Blip_Buffer& GetBuffer() {
		return BlipBuffer;
	}
	void	SetClockRate(uint32_t Rate);
	void	ClearBuffer();
	void FinishBuffer(int t);
	int		SamplesAvail() const;
	void	MixSamples(blip_amplitude_t *pBuffer, uint32_t Count);
	uint32_t	GetMixSampleCount(int t) const;

	void	AddSample(int ChanID, int Value);
	int		ReadBuffer(void *Buffer);

	int32_t	GetChanOutput(uint8_t Chan) const;
	void	SetChipLevel(chip_level_t Chip, float Level);
	uint32_t	ResampleDuration(uint32_t Time) const;
	void	SetNamcoVolume(float fVol);

	int		GetMeterDecayRate() const;		// // // 050B
	void	SetMeterDecayRate(int Rate);		// // // 050B

private:
	void MixN163(int Value, int Time);
	void MixVRC6(int Value, int Time);
	void MixMMC5(int Value, int Time);
	void MixS5B(int Value, int Time);

	void StoreChannelLevel(int Channel, int Value);
	void ClearChannelLevels();

	float GetAttenuation() const;

private:
	// Pointer to parent/owning CAPU object.
	CAPU * m_APU;

	// Blip buffer synths

	// Should never be null during playback. CAPU creates all expansion chips,
	// even if chips are not active in current module.
	Blip_Synth<blip_good_quality> SynthVRC6;
	Blip_Synth<blip_good_quality> SynthMMC5;
	Blip_Synth<blip_good_quality> SynthN163;
	Blip_Synth<blip_good_quality> SynthS5B;		// // // 050B

	/// Only used by CMixer::ClearBuffer(), which clears the global Blip_Buffer
	/// and all Blip_Synth owned by CMixer.
	///
	/// What about CSoundChip2 which owns its own Blip_Synth?
	/// I've decided that CMixer should not be responsible for clearing those Blip_Synth,
	/// but rather CSoundChip2::Reset() should do so.
	///
	/// This works because CMixer::ClearBuffer() is only called by CAPU::Reset(),
	/// which also calls CSoundChip2::Reset() on each sound chip.
	#define FOREACH_SYNTH(X, SEP) \
		X(SynthVRC6) SEP \
		X(SynthMMC5) SEP \
		X(SynthN163) SEP \
		X(SynthS5B)

	// Blip buffer object
	Blip_Buffer	BlipBuffer;

	int32_t		m_iChannels[CHANNELS];
	uint8_t		m_iExternalChip;
	uint32_t	m_iSampleRate;

	float		m_fChannelLevels[CHANNELS];
	uint32_t	m_iChanLevelFallOff[CHANNELS];

	int			m_iMeterDecayRate;		// // // 050B
	MixerConfig m_MixerConfig;

	float		m_fLevelAPU1;
	float		m_fLevelAPU2;
	float		m_fLevelVRC6;
	float		m_fLevelMMC5;
	float		m_fLevelFDS;
	float		m_fLevelN163;
	float		m_fLevelS5B;		// // // 050B

	friend class CAPU;
};
