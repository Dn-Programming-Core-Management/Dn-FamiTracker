/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

typedef unsigned char		uint8;
typedef unsigned short		uint16;
typedef unsigned long		uint32;
typedef unsigned __int64	uint64;
typedef signed char			int8;
typedef signed short		int16;
typedef signed long			int32;
typedef signed __int64		int64;

#define _MAIN_H_

#define SAMPLES_IN_BYTES(x) (x << SampleSizeShift)

const int SPEED_AUTO	= 0;
const int SPEED_NTSC	= 1;
const int SPEED_PAL		= 2;


// Used to get the DPCM state
struct stDPCMState {
	int SamplePos;
	int DeltaCntr;
};

// Used to play the audio when the buffer is full
class IAudioCallback {
public:
	virtual void FlushBuffer(int16 *Buffer, uint32 Size) = 0;
};


// class for simulating CPU memory, used by the DPCM channel
class CSampleMem 
{
public:
	CSampleMem() : m_pMemory(0), m_iMemSize(0) {
	};

	uint8 Read(uint16 Address) const {
		if (!m_pMemory)
			return 0;
		uint16 Addr = (Address - 0xC000);// % m_iMemSize;
		if (Addr >= m_iMemSize)
			return 0;
		return m_pMemory[Addr];
	};

	void SetMem(const char *pPtr, int Size) {
		m_pMemory = (uint8*)pPtr;
		m_iMemSize = Size;
	};

	void Clear() {
		m_pMemory = 0;
		m_iMemSize = 0;
	}

private:
	const uint8 *m_pMemory;
	uint16 m_iMemSize;
};
