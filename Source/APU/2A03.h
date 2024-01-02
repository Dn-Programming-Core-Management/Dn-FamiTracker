/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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

#include "APU/nsfplay/xgm/devices/Sound/nes_apu.h"
#include "APU/nsfplay/xgm/devices/Sound/nes_dmc.h"
#include "APU/nsfplay/xgm/devices/device.h"

// class for simulating CPU memory, used by the DPCM channel
class CSampleMem : public xgm::IDevice
{
public:
	CSampleMem() : m_pMemory(0), m_iMemSize(0) {
	};

	uint8_t Read(uint16_t Address) const {
		if (!m_pMemory)
			return 0;
		uint16_t Addr = (Address - 0xC000);// % m_iMemSize;
		if (Addr >= m_iMemSize)
			return 0;
		return m_pMemory[Addr];
	};

	void SetMem(const char* pPtr, int Size) {
		m_pMemory = (uint8_t*)pPtr;
		m_iMemSize = Size;
	};

	void Clear() {
		m_pMemory = 0;
		m_iMemSize = 0;
	}

// impl xgm::IDevice

	// CSampleMem as IDevice is only used by NES_DMC.
	// It only calls Read(adr, val, id=0) and ignores the return value.

	// not called, don't care
	void Reset() override {}

	// not called, don't care
	bool Write(UINT32 adr, UINT32 val, UINT32 id) override {
		return false;
	}

	bool Read(UINT32 adr, UINT32& val, UINT32 id) override {
		val = Read((uint16_t)adr);
		return true;
	}

private:
	const uint8_t* m_pMemory;
	uint16_t m_iMemSize;
};

class C2A03 : public CSoundChip2
{
public:
	C2A03();

	void Reset() override;
	void UpdateFilter(blip_eq_t eq) override;
	void Process(uint32_t Time, Blip_Buffer& Output) override;
	void EndFrame(Blip_Buffer& Output, gsl::span<int16_t> TempBuffer) override;

	void Write(uint16_t Address, uint8_t Value) override;
	uint8_t Read(uint16_t Address, bool &Mapped) override;

	double GetFreq(int Channel) const override;		// // //
	int GetChannelLevel(int Channel) override;
	int GetChannelLevelRange(int Channel) const override;

public:
	void UpdateMixingAPU1(double v, bool UseSurveyMix = false);
	void UpdateMixingAPU2(double v, bool UseSurveyMix = false);

	void	ClockSequence();		// // //
	
	void	ChangeMachine(int Machine);
	
	CSampleMem *GetSampleMemory();		// // //
	uint8_t	GetSamplePos() const;
	uint8_t	GetDeltaCounter() const;
	bool	DPCMPlaying() const;

private:
	/// Referenced by m_Apu2.
	CSampleMem m_SampleMem;

	xgm::NES_APU m_Apu1;
	xgm::NES_DMC m_Apu2;

	// [0..4] correspond to Pulse 1, Pulse 2, Triangle, Noise, and DPCM.
	ChannelLevelState<uint8_t> m_ChannelLevels[5];

	Blip_Synth<blip_good_quality> Synth2A03SS;
	Blip_Synth<blip_good_quality> Synth2A03TND;

	uint32_t	m_iTime = 0;  // Clock counter, used as a timestamp for Blip_Buffer, resets every new frame
};
