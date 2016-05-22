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


#pragma once

#include "2A03Chan.h"		// // //

class CNoise : public C2A03Chan {
public:
	CNoise(CMixer *pMixer, int ID);		// // //
	~CNoise();

	void	Reset();
	void	Write(uint16_t Address, uint8_t Value);
	void	WriteControl(uint8_t Value);
	uint8_t	ReadControl();
	void	Process(uint32_t Time);
	double	GetFrequency() const;		// // //

	void	LengthCounterUpdate();
	void	EnvelopeUpdate();

public:
	static const uint16_t	NOISE_PERIODS_NTSC[];
	static const uint16_t	NOISE_PERIODS_PAL[];

	const uint16_t *PERIOD_TABLE;

private:
	uint8_t	m_iLooping, m_iEnvelopeFix, m_iEnvelopeSpeed;
	uint8_t	m_iEnvelopeVolume, m_iFixedVolume;
	int8_t	m_iEnvelopeCounter;
	
	uint8_t	m_iSampleRate;
	uint16_t	m_iShiftReg;
};
