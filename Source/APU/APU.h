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

#ifndef APU_H
#define APU_H

//#define LOGGING

#include "../Common.h"
#include "Mixer.h"

// External classes
class C2A03;		// // //
class CVRC6;
class CVRC7;
class CFDS;
class CMMC5;
class CN163;
class CS5B;

class CExternal;

#ifdef LOGGING
class CFile;
#endif

class CAPU {
public:
	CAPU(IAudioCallback *pCallback, CSampleMem *pSampleMem);
	~CAPU();

	void	Reset();
	void	Process();
	void	AddTime(int32 Cycles);

	void	SetExternalSound(uint8 Chip);
	void	ExternalWrite(uint16 Address, uint8 Value);
	uint8	ExternalRead(uint16 Address);
	
	void	ChangeMachineRate(int Machine, int Rate);		// // //
	bool	SetupSound(int SampleRate, int NrChannels, int Speed);
	void	SetupMixer(int LowCut, int HighCut, int HighDamp, int Volume) const;

	int32	GetVol(uint8 Chan) const;
	uint16	GetReg(int Chip, int Reg) const;		// // //
	
	uint8	GetSamplePos() const;
	uint8	GetDeltaCounter() const;
	bool	DPCMPlaying() const;

	void	SetChipLevel(chip_level_t Chip, float Level);

	void	SetNamcoMixing(bool bLinear);		// // //

#ifdef LOGGING
	void	Log();
#endif

public:
	static const uint8	LENGTH_TABLE[];
	static const uint32	BASE_FREQ_NTSC;
	static const uint32	BASE_FREQ_PAL;
	static const uint8	FRAME_RATE_NTSC;
	static const uint8	FRAME_RATE_PAL;
	static const uint8	REG_DECAY_RATE;		// // //

private:
	static const int SEQUENCER_PERIOD;
	static const int SEQUENCER_PERIOD_PAL;
	
	void EndFrame();
	
	void LogExternalWrite(uint16 Address, uint8 Value);

private:
	CMixer		*m_pMixer;
	IAudioCallback *m_pParent;

	// Expansion chips
	C2A03		*m_p2A03;		// // //
	CVRC6		*m_pVRC6;
	CMMC5		*m_pMMC5;
	CFDS		*m_pFDS;
	CN163		*m_pN163;
	CVRC7		*m_pVRC7;
	CS5B		*m_pS5B;

	uint8		m_iExternalSoundChip;				// External sound chip, if used

	uint32		m_iSampleRate;						// // //
	uint32		m_iFrameCycleCount;
	uint32		m_iFrameClock;
	uint32		m_iCyclesToRun;						// Number of cycles to process

	uint32		m_iSoundBufferSamples;				// Size of buffer, in samples
	bool		m_bStereoEnabled;					// If stereo is enabled

	uint32		m_iSampleSizeShift;					// To convert samples to bytes
	uint32		m_iSoundBufferSize;					// Size of buffer, in samples
	uint32		m_iBufferPointer;					// Fill pos in buffer
	int16		*m_pSoundBuffer;					// Sound transfer buffer

	uint16		m_iRegs2A03[0x20];					// // // renamed
	uint16		m_iRegsVRC6[0x10];
	uint16		m_iRegsVRC7[0x40];
	uint16		m_iRegsFDS[0x50];
	uint16		m_iRegsMMC5[0x20];
	uint16		m_iRegsN163[0x80];
	uint16		m_iRegsS5B[0x10];
	uint8		m_iPortVRC7;						// // //
	uint8		m_iPortN163;
	uint8		m_iPortS5B;

	uint32		m_iFrameCycles;						// Cycles emulated from start of frame
	uint32		m_iSequencerClock;					// Clock for frame sequencer

	float		m_fLevelVRC7;
	float		m_fLevelS5B;

#ifdef LOGGING
	CFile		  *m_pLog;
	int			  m_iFrame;
//	unsigned char m_iRegs[32];
#endif

};

#endif /* APU_H */
