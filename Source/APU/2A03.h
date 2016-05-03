/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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


#pragma once

class CMixer;

#include "External.h"
#include "Channel.h"

class CSquare;
class CTriangle;
class CNoise;
class CDPCM;

class C2A03 : public CExternal
{
public:
	C2A03(CMixer *pMixer);
	virtual ~C2A03();

	void Reset();
	void Process(uint32_t Time);
	void EndFrame();

	void Write(uint16_t Address, uint8_t Value);
	uint8_t Read(uint16_t Address, bool &Mapped);

public:
	void	ClockSequence();		// // //
	
	void	ChangeMachine(int Machine);
	
	CSampleMem *GetSampleMemory() const;		// // //
	uint8_t	GetSamplePos() const;
	uint8_t	GetDeltaCounter() const;
	bool	DPCMPlaying() const;

private:
	inline void Clock_240Hz() const;		// // //
	inline void Clock_120Hz() const;		// // //
	inline void Clock_60Hz() const;		// // //

	inline void RunAPU1(uint32_t Time);
	inline void RunAPU2(uint32_t Time);

private:
	CSquare		*m_pSquare1;
	CSquare		*m_pSquare2;
	CTriangle	*m_pTriangle;
	CNoise		*m_pNoise;
	CDPCM		*m_pDPCM;
	
	uint8_t		m_iFrameSequence;					// Frame sequence
	uint8_t		m_iFrameMode;						// 4 or 5-steps frame sequence
};
