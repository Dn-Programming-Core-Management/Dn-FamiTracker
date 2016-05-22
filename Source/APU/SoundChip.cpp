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

#include "../stdafx.h"
#include "SoundChip.h"
#include "../RegisterState.h"

CSoundChip::CSoundChip(CMixer *pMixer) :
	m_pMixer(pMixer),
	m_pRegisterLogger(new CRegisterLogger { })
{
}

CSoundChip::~CSoundChip()
{
	if (m_pRegisterLogger)
		delete m_pRegisterLogger;
}

double CSoundChip::GetFreq(int Channel) const		// // //
{
	return 0.0;
}

void CSoundChip::Log(uint16_t Address, uint8_t Value)		// // //
{
	// default logger operation
	if (m_pRegisterLogger->SetPort(Address))
		m_pRegisterLogger->Write(Value);
}

CRegisterLogger *CSoundChip::GetRegisterLogger() const		// // //
{
	return m_pRegisterLogger;
}
