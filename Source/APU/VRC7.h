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

#ifndef VRC7_H
#define VRC7_H

#include "external.h"
#include "emu2413.h"

class CVRC7 : public CExternal {
public:
	CVRC7(CMixer *pMixer);
	virtual ~CVRC7();

	void Reset();
	void SetSampleSpeed(uint32_t SampleRate, double ClockRate, uint32_t FrameRate);
	void SetVolume(float Volume);
	void Write(uint16_t Address, uint8_t Value);
	uint8_t Read(uint16_t Address, bool &Mapped);
	void EndFrame();
	void Process(uint32_t Time);

protected:
	static const float  AMPLIFY;
	static const uint32_t OPL_CLOCK;

private:
	OPLL	*m_pOPLLInt;
	uint32_t	m_iTime;
	uint32_t	m_iMaxSamples;

	int16_t	*m_pBuffer;
	uint32_t	m_iBufferPtr;

	uint8_t	m_iSoundReg;

	float	m_fVolume;
};


#endif /* VRC7_H */
