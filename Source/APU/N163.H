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

#include "external.h"
#include "Channel.h"

class CMixer;

class CN163Chan : public CExChannel {
public:
	CN163Chan(CMixer *pMixer, int ID, uint8 *pWaveData);
	virtual ~CN163Chan();
	void Reset();
	void Write(uint16 Address, uint8 Value);

	virtual inline void EndFrame();

	void Process(uint32 Time, uint8 ChannelsActive, CN163 *pParent);
	void ProcessClean(uint32 Time, uint8 ChannelsActive);		// // //

	uint8 ReadMem(uint8 Reg);
	void ResetCounter();

private:
	uint32	m_iCounter, m_iFrequency;
	uint32	m_iPhase;
	uint8	m_iVolume;
	uint32	m_iWaveLength;
	uint8	m_iWaveOffset;
	uint8	*m_pWaveData;

	uint8 m_iLastSample;
};

class CN163 : public CExternal {
public:
	CN163(CMixer *pMixer);
	virtual ~CN163();
	void Reset();
	void Process(uint32 Time);
	void EndFrame();
	void Write(uint16 Address, uint8 Value);
	uint8 Read(uint16 Address, bool &Mapped);
	uint8 ReadMem(uint8 Reg);
	void Mix(int32 Value, uint32 Time, uint8 ChanID);
	void SetMixingMethod(bool bLinear);		// // //

protected:
	void ProcessOld(uint32 Time);		// // //

private:
	CN163Chan	*m_pChannels[8];

	uint8		*m_pWaveData;
	uint8		m_iExpandAddr;
	uint8		m_iChansInUse;

	int32		m_iLastValue;

	uint32		m_iGlobalTime;

	uint32		m_iChannelCntr;
	uint32		m_iActiveChan;
	uint32		m_iCycle;

	bool		m_bOldMixing;		// // //
};

#endif /* N163_H */