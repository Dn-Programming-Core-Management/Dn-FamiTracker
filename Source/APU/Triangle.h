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
 * Triangle wave generation
 *
 */


#pragma once

#include "2A03Chan.h"		// // //

class CTriangle : public C2A03Chan {
public:
	CTriangle(CMixer *pMixer, int ID);
	~CTriangle();

	void	Reset();
	void	Write(uint16_t Address, uint8_t Value);
	void	WriteControl(uint8_t Value);
	uint8_t	ReadControl();
	void	Process(uint32_t Time);
	double	GetFrequency() const;		// // //

	void	LengthCounterUpdate();
	void	LinearCounterUpdate();

public:
	uint32_t CPU_RATE;		// // //

private:
	static const uint8_t TRIANGLE_WAVE[];

private:
	uint8_t	m_iLoop, m_iLinearLoad, m_iHalt;
	uint16_t	m_iLinearCounter;
	int8_t	m_iStepGen;
};
