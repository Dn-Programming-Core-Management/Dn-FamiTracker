/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

#include <algorithm>		// // //
#include <vector>
#include "../stdafx.h"
#include <cstdio>
#include <memory>
#include <cmath>
#include "APU.h"
#include "2A03.h"		// // //
#include "VRC6.h"
#include "MMC5.h"
#include "FDS.h"
#include "N163.h"
#include "VRC7.h"
#include "S5B.h"

const int	 CAPU::SEQUENCER_PERIOD		= 7458;
const int	 CAPU::SEQUENCER_PERIOD_PAL	= 7458;			// ????
const uint32 CAPU::BASE_FREQ_NTSC		= 1789773;		// 72.667
const uint32 CAPU::BASE_FREQ_PAL		= 1662607;
const uint8	 CAPU::FRAME_RATE_NTSC		= 60;
const uint8	 CAPU::FRAME_RATE_PAL		= 50;

const uint8  CAPU::REG_DECAY_RATE		= 15;			// // //

const uint8 CAPU::LENGTH_TABLE[] = {
	0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
	0xA0, 0x08, 0x3C, 0x0A, 0x0E, 0x0C, 0x1A, 0x0E,
	0x0C, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xC0, 0x18, 0x48, 0x1A, 0x10, 0x1C, 0x20, 0x1E
};

static std::vector<CExternal*> ExChips;

CAPU::CAPU(IAudioCallback *pCallback, CSampleMem *pSampleMem) : 
	m_pParent(pCallback),
	m_iFrameCycles(0),
	m_pSoundBuffer(NULL),
	m_pMixer(new CMixer()),
	m_iExternalSoundChip(0),
	m_iCyclesToRun(0)
{
	m_p2A03 = new C2A03(m_pMixer);		// // //
	m_pMMC5 = new CMMC5(m_pMixer);
	m_pVRC6 = new CVRC6(m_pMixer);
	m_pVRC7 = new CVRC7(m_pMixer);
	m_pFDS = new CFDS(m_pMixer);
	m_pN163 = new CN163(m_pMixer);
	m_pS5B = new CS5B(m_pMixer);

	memset(m_iRegs2A03, 0, sizeof(m_iRegs2A03));
	memset(m_iRegsVRC6, 0, sizeof(m_iRegsVRC6));
	memset(m_iRegsVRC7, 0, sizeof(m_iRegsVRC7));
	memset(m_iRegsFDS, 0, sizeof(m_iRegsFDS));
	memset(m_iRegsMMC5, 0, sizeof(m_iRegsMMC5));
	memset(m_iRegsN163, 0, sizeof(m_iRegsN163));
	memset(m_iRegsS5B, 0, sizeof(m_iRegsS5B));

	m_p2A03->SetSampleMemory(pSampleMem);		// // //

	m_iPortVRC7 = m_iPortS5B = 0;
	m_fLevelVRC7 = m_fLevelS5B = 1.0f;

#ifdef LOGGING
	m_pLog = new CFile("apu_log.txt", CFile::modeCreate | CFile::modeWrite);
	m_iFrame = 0;
#endif
}

CAPU::~CAPU()
{
	SAFE_RELEASE(m_p2A03);		// // //
	SAFE_RELEASE(m_pMMC5);
	SAFE_RELEASE(m_pVRC6);
	SAFE_RELEASE(m_pVRC7);
	SAFE_RELEASE(m_pFDS);
	SAFE_RELEASE(m_pN163);
	SAFE_RELEASE(m_pS5B);

	SAFE_RELEASE(m_pMixer);

	SAFE_RELEASE(m_pSoundBuffer);

#ifdef LOGGING
	m_pLog->Close();
	delete m_pLog;
#endif
}

// The main APU emulation
//
// The amount of cycles that will be emulated is added by CAPU::AddCycles
//
void CAPU::Process()
{	
	while (m_iCyclesToRun > 0) {

		uint32 Time = m_iCyclesToRun;
		Time = std::min(Time, m_iSequencerClock);
		Time = std::min(Time, m_iFrameClock);

		for (auto Chip : ExChips)		// // //
			Chip->Process(Time);

		m_iFrameCycles	  += Time;
		m_iSequencerClock -= Time;
		m_iFrameClock	  -= Time;
		m_iCyclesToRun	  -= Time;

		if (m_iSequencerClock == 0) {
			m_iSequencerClock += C2A03::SEQUENCER_PERIOD;
			m_p2A03->ClockSequence();
			m_pMMC5->ClockSequence();		// // //
		}

		if (m_iFrameClock == 0)
			EndFrame();
	}
}

// End of audio frame, flush the buffer if enough samples has been produced, and start a new frame
void CAPU::EndFrame()
{
	// The APU will always output audio in 32 bit signed format
	
	for (auto Chip : ExChips)		// // //
		Chip->EndFrame();

	int SamplesAvail = m_pMixer->FinishBuffer(m_iFrameCycles);
	int ReadSamples	= m_pMixer->ReadBuffer(SamplesAvail, m_pSoundBuffer, m_bStereoEnabled);
	m_pParent->FlushBuffer(m_pSoundBuffer, ReadSamples);
	
	m_iFrameClock /*+*/= m_iFrameCycleCount;
	m_iFrameCycles = 0;

	// // //
	
	uint16 *REG_ARRAY[] = {m_iRegs2A03, m_iRegsVRC6, m_iRegsVRC7, m_iRegsFDS, m_iRegsMMC5, m_iRegsN163, m_iRegsS5B};
	static const size_t REG_SIZE[] = {sizeof(m_iRegs2A03), sizeof(m_iRegsVRC6), sizeof(m_iRegsVRC7), sizeof(m_iRegsFDS),
									  sizeof(m_iRegsMMC5), sizeof(m_iRegsN163), sizeof(m_iRegsS5B)};
	for (int i = 0; i < sizeof(REG_ARRAY) / sizeof(uint16*); i++) {
		for (size_t j = 0; j < REG_SIZE[i] / sizeof(uint16); j++) {
			if (i == 5 && j >= 0x78U - ((m_iRegsN163[0x7F] & 0x70) >> 1) && (j & 0x01) && (j & 0x07) != 0x07) { // N163 phase registers
				REG_ARRAY[i][j] = (REG_ARRAY[i][j] & 0xFF) | (CAPU::REG_DECAY_RATE << 8) | (CAPU::REG_DECAY_RATE << 12);
				continue;
			}
			uint16 hi = REG_ARRAY[i][j] & 0x0F00;
			uint16 upd = REG_ARRAY[i][j] & 0xF000;
			if (hi) hi -= 0x100;
			if (upd) upd -= 0x1000;
			REG_ARRAY[i][j] = (REG_ARRAY[i][j] & 0xFF) | hi | upd;
		}
	}
		
#ifdef LOGGING
	++m_iFrame;
#endif
}

void CAPU::Reset()
{
	// Reset APU
	//
	
	m_iSequencerClock	= C2A03::SEQUENCER_PERIOD;
	
	m_iCyclesToRun		= 0;
	m_iFrameCycles		= 0;
	m_iFrameClock		= m_iFrameCycleCount;
	
	m_pMixer->ClearBuffer();
	
	for (auto Chip : ExChips)		// // //
		Chip->Reset();

#ifdef LOGGING
	m_iFrame = 0;
#endif
}

void CAPU::SetupMixer(int LowCut, int HighCut, int HighDamp, int Volume) const
{
	// New settings
	m_pMixer->UpdateSettings(LowCut, HighCut, HighDamp, float(Volume) / 100.0f);
	m_pVRC7->SetVolume((float(Volume) / 100.0f) * m_fLevelVRC7);
	m_pS5B->SetVolume((float(Volume) / 100.0f) * m_fLevelS5B);		// // //
}

void CAPU::SetExternalSound(uint8 Chip)
{
	// Set expansion chip
	m_iExternalSoundChip = Chip;
	m_pMixer->ExternalSound(Chip);

	ExChips.clear();

	ExChips.push_back(m_p2A03);		// // //
	if (Chip & SNDCHIP_VRC6)
		ExChips.push_back(m_pVRC6);
	if (Chip & SNDCHIP_VRC7)
		ExChips.push_back(m_pVRC7);
	if (Chip & SNDCHIP_FDS)
		ExChips.push_back(m_pFDS);
	if (Chip & SNDCHIP_MMC5)
		ExChips.push_back(m_pMMC5);
	if (Chip & SNDCHIP_N163)
		ExChips.push_back(m_pN163);
	if (Chip & SNDCHIP_S5B)
		ExChips.push_back(m_pS5B);

	Reset();
}

void CAPU::ChangeMachineRate(int Machine, int Rate)		// // //
{
	// Allow to change speed on the fly
	//
	
	uint32 BaseFreq = (Machine == MACHINE_NTSC) ? BASE_FREQ_NTSC : BASE_FREQ_PAL;
	m_p2A03->ChangeMachine(Machine);

	m_pVRC7->SetSampleSpeed(m_iSampleRate, BaseFreq, Rate);
	m_pS5B->SetSampleSpeed(m_iSampleRate, BaseFreq, Rate);
	m_iFrameCycleCount = BaseFreq / Rate;
}

bool CAPU::SetupSound(int SampleRate, int NrChannels, int Machine)		// // //
{
	// Allocate a sound buffer
	//
	// Returns false if a buffer couldn't be allocated
	//
	
	uint32 BaseFreq = (Machine == MACHINE_NTSC) ? BASE_FREQ_NTSC : BASE_FREQ_PAL;
	uint8 FrameRate = (Machine == MACHINE_NTSC) ? FRAME_RATE_NTSC : FRAME_RATE_PAL;
	m_iSampleRate = SampleRate;		// // //

	m_iSoundBufferSamples = uint32(m_iSampleRate / FRAME_RATE_PAL);	// Samples / frame. Allocate for PAL, since it's more
	m_bStereoEnabled	  = (NrChannels == 2);	
	m_iSoundBufferSize	  = m_iSoundBufferSamples * NrChannels;		// Total amount of samples to allocate
	m_iSampleSizeShift	  = (NrChannels == 2) ? 1 : 0;
	m_iBufferPointer	  = 0;

	if (!m_pMixer->AllocateBuffer(m_iSoundBufferSamples, m_iSampleRate, NrChannels))		// // //
		return false;

	m_pMixer->SetClockRate(BaseFreq);

	SAFE_RELEASE_ARRAY(m_pSoundBuffer);

	m_pSoundBuffer = new int16[m_iSoundBufferSize << 1];

	if (m_pSoundBuffer == NULL)
		return false;

	ChangeMachineRate(Machine, FrameRate);		// // //

	return true;
}

void CAPU::AddTime(int32 Cycles)
{
	if (Cycles < 0)
		return;
	m_iCyclesToRun += Cycles;
}

void CAPU::ExternalWrite(uint16 Address, uint8 Value)
{
	// Data was written to an external sound chip 
	// (this doesn't really belong in the APU but are here for convenience)
	//

	Process();
	
	for (auto Chip : ExChips)		// // //
		Chip->Write(Address, Value);

	LogExternalWrite(Address, Value);
}

uint8 CAPU::ExternalRead(uint16 Address)
{
	// Data read from an external chip
	//

	uint8 Value(0);
	bool Mapped(false);

	Process();
	
	for (auto Chip : ExChips)		// // //
		if (!Mapped)
			Value = Chip->Read(Address, Mapped);

	if (!Mapped)
		Value = Address >> 8;	// open bus

	return Value;
}

// Expansion for famitracker

int32 CAPU::GetVol(uint8 Chan) const	
{
	return m_pMixer->GetChanOutput(Chan);
}

uint8 CAPU::GetSamplePos() const
{
	return m_p2A03->GetSamplePos();
}

uint8 CAPU::GetDeltaCounter() const
{
	return m_p2A03->GetDeltaCounter();
}

bool CAPU::DPCMPlaying() const
{
	return m_p2A03->DPCMPlaying();
}

#ifdef LOGGING
void CAPU::Log()
{
	CString str;
	str.Format("Frame %08i: ", m_iFrame - 1);
	str.Append("2A03 ");
	for (int i = 0; i < 0x14; ++i)
		str.AppendFormat("%02X ", GetReg(SNDCHIP_NONE, i) & 0xFF);
	if (m_iExternalSoundChip & SNDCHIP_VRC6) {		// // //
		str.Append("VRC6 ");
		for (int i = 0; i < 0x09; ++i)
			str.AppendFormat("%02X ", GetReg(SNDCHIP_VRC6, i) & 0xFF);
	}
	if (m_iExternalSoundChip & SNDCHIP_MMC5) {
		str.Append("MMC5 ");
		for (int i = 0; i < 0x08; ++i)
			str.AppendFormat("%02X ", GetReg(SNDCHIP_MMC5, i) & 0xFF);
	}
	if (m_iExternalSoundChip & SNDCHIP_N163) {
		str.Append("N163 ");
		for (int i = 0; i < 0x80; ++i)
			str.AppendFormat("%02X ", GetReg(SNDCHIP_N163, i) & 0xFF);
	}
	if (m_iExternalSoundChip & SNDCHIP_FDS) {
		str.Append("FDS ");
		for (int i = 0; i < 0x0B; ++i)
			str.AppendFormat("%02X ", GetReg(SNDCHIP_FDS, i) & 0xFF);
	}
	if (m_iExternalSoundChip & SNDCHIP_VRC7) {
		str.Append("VRC7 ");
		for (int i = 0; i < 0x40; ++i)
			str.AppendFormat("%02X ", GetReg(SNDCHIP_VRC7, i) & 0xFF);
	}
	if (m_iExternalSoundChip & SNDCHIP_S5B) {
		str.Append("S5B ");
		for (int i = 0; i < 0x10; ++i)
			str.AppendFormat("%02X ", GetReg(SNDCHIP_S5B, i) & 0xFF);
	}
	str.Append("\r\n");
	m_pLog->Write(str, str.GetLength());
}
#endif

void CAPU::SetChipLevel(chip_level_t Chip, float Level)
{
	float fLevel = powf(10, Level / 20.0f);		// Convert dB to linear

	switch (Chip) {
		case CHIP_LEVEL_VRC7:
			m_fLevelVRC7 = fLevel;
			break;
		case CHIP_LEVEL_S5B:
			m_fLevelS5B = fLevel;
			break;
		default:
			m_pMixer->SetChipLevel(Chip, fLevel);
	}
}

void CAPU::SetNamcoMixing(bool bLinear)		// // //
{
	m_pMixer->SetNamcoMixing(bLinear);
	m_pN163->SetMixingMethod(bLinear);
}

void CAPU::LogExternalWrite(uint16 Address, uint8 Value)
{
#define UpdateReg(tbl, adr) uint16 old = tbl[adr] & 0xFF; \
							tbl[adr] = Value | (CAPU::REG_DECAY_RATE << 8) | (tbl[adr] & 0xF000); \
							if (Value != old) \
								tbl[adr] = (tbl[adr] & 0xFFF) | (CAPU::REG_DECAY_RATE << 12);		// // //
	
		 if (Address >= 0x4000 && Address <= 0x401F) { UpdateReg(m_iRegs2A03, Address - 0x4000); }
	else if (Address >= 0x9000 && Address <= 0x9003) { UpdateReg(m_iRegsVRC6, Address - 0x9000); }
	else if (Address >= 0xA000 && Address <= 0xA003) { UpdateReg(m_iRegsVRC6, Address - 0xA000 + 4); }
	else if (Address >= 0xB000 && Address <= 0xB003) { UpdateReg(m_iRegsVRC6, Address - 0xB000 + 8); }
	else if (Address >= 0x4040 && Address <= 0x408F) { UpdateReg(m_iRegsFDS , Address - 0x4040); }
	else if (Address >= 0x5000 && Address <= 0x501F) { UpdateReg(m_iRegsMMC5, Address - 0x5000); }
	else if (Address == 0x9010) m_iPortVRC7 = Value & 0x3F;
	else if (Address == 0xF800) m_iPortN163 = Value;
	else if (Address == 0xC000) m_iPortS5B = Value & 0x0F;
	else if (Address == 0x9030) { UpdateReg(m_iRegsVRC7, m_iPortVRC7); }
	else if (Address == 0x4800) {
		UpdateReg(m_iRegsN163, m_iPortN163 & 0x7F);
		if (m_iPortN163 & 0x80)
			m_iPortN163 = (++m_iPortN163) | 0x80;
	}
	else if (Address == 0xE000) { UpdateReg(m_iRegsS5B, m_iPortS5B); }
}

uint16 CAPU::GetReg(int Chip, int Reg) const		// // //
{
	switch (Chip) {
		case SNDCHIP_NONE:
			return m_iRegs2A03[Reg & 0x1F];
		case SNDCHIP_VRC6:
			return m_iRegsVRC6[Reg & 0x0F];
		case SNDCHIP_MMC5:
			return m_iRegsMMC5[Reg & 0x1F];
		case SNDCHIP_N163:
			return m_pN163->ReadMem(Reg & 0x7F) | (m_iRegsN163[Reg & 0x7F] & 0xFF00);
		case SNDCHIP_FDS:
			return m_iRegsFDS[Reg < 0 ? 0 : Reg % (sizeof(m_iRegsFDS) / 2)];
		case SNDCHIP_VRC7:
			return m_iRegsVRC7[Reg & 0x3F];
		case SNDCHIP_S5B:
			return m_iRegsS5B[Reg & 0x0F];
	}

	return 0;
}
