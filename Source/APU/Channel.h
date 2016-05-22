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

class CMixer;

//
// This class is used to derive the audio channels
//

class CChannel {
public:
	CChannel(CMixer *pMixer, uint8_t Chip, uint8_t ID) :
		m_pMixer(pMixer), m_iChip(Chip), m_iChanId(ID), m_iTime(0), m_iLastValue(0) 
	{
	}

	virtual void EndFrame() { m_iTime = 0; }

	virtual double GetFrequency() const = 0;		// // //

protected:
	virtual void Mix(int32_t Value) {
		int32_t Delta = Value - m_iLastValue;
		if (Delta)
			m_pMixer->AddValue(m_iChanId, m_iChip, Delta, Value, m_iTime);
		m_iLastValue = Value;
	}

protected:
	CMixer		*m_pMixer;			// The mixer

	uint32_t	m_iTime;			// Cycle counter, resets every new frame
	int32_t		m_iLastValue;		// Last value sent to mixer
	uint8_t		m_iChanId;			// This channels unique ID
	uint8_t		m_iChip;			// Chip
};
