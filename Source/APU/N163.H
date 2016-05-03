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

#ifndef N163_H
#define N163_H

#include "SoundChip.h"
#include "Channel.h"

class CMixer;

class CN163Chan : public CChannel {
public:
	CN163Chan(CMixer *pMixer, int ID, uint8_t *pWaveData);
	virtual ~CN163Chan();
	void Reset();
	void Write(uint16_t Address, uint8_t Value);

	void Process(uint32_t Time, uint8_t ChannelsActive, CN163 *pParent);
	void ProcessClean(uint32_t Time, uint8_t ChannelsActive);		// // //

	uint8_t ReadMem(uint8_t Reg);
	void ResetCounter();

private:
	uint32_t	m_iCounter, m_iFrequency;
	uint32_t	m_iPhase;
	uint8_t	m_iVolume;
	uint32_t	m_iWaveLength;
	uint8_t	m_iWaveOffset;
	uint8_t	*m_pWaveData;

	uint8_t m_iLastSample;
};

class CN163 : public CSoundChip {
public:
	CN163(CMixer *pMixer);
	virtual ~CN163();
	void Reset();
	void Process(uint32_t Time);
	void EndFrame();
	void Write(uint16_t Address, uint8_t Value);
	void Log(uint16_t Address, uint8_t Value);		// // //
	uint8_t Read(uint16_t Address, bool &Mapped);
	uint8_t ReadMem(uint8_t Reg);
	void Mix(int32_t Value, uint32_t Time, uint8_t ChanID);
	void SetMixingMethod(bool bLinear);		// // //

protected:
	void ProcessOld(uint32_t Time);		// // //

private:
	CN163Chan	*m_pChannels[8];

	uint8_t		*m_pWaveData;
	uint8_t		m_iExpandAddr;
	uint8_t		m_iChansInUse;

	int32_t		m_iLastValue;

	uint32_t		m_iGlobalTime;

	uint32_t		m_iChannelCntr;
	uint32_t		m_iActiveChan;
	uint32_t		m_iCycle;

	bool		m_bOldMixing;		// // //
};

#endif /* N163_H */