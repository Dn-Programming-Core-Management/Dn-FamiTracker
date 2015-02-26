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

/*
 * Random noise generation
 *
 */

#ifndef NOISE_H
#define NOISE_H

#include "Channel.h"

class CNoise : public CChannel {
public:
	CNoise(CMixer *pMixer, int ID);
	~CNoise();

	void	Reset();
	void	Write(uint16 Address, uint8 Value);
	void	WriteControl(uint8 Value);
	uint8	ReadControl();
	void	Process(uint32 Time);

	void	LengthCounterUpdate();
	void	EnvelopeUpdate();

public:
	static const uint16	NOISE_PERIODS_NTSC[];
	static const uint16	NOISE_PERIODS_PAL[];

	const uint16 *PERIOD_TABLE;

private:
	uint8	m_iLooping, m_iEnvelopeFix, m_iEnvelopeSpeed;
	uint8	m_iEnvelopeVolume, m_iFixedVolume;
	int8	m_iEnvelopeCounter;
	
	uint8	m_iSampleRate;
	uint16	m_iShiftReg;
};

#endif /* NOISE_H */
