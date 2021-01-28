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

#include "../stdafx.h"
#include "APU.h"
#include "FDS.h"
#include "nezplug/FDSSound.h"
#include "../RegisterState.h"		// // //

// FDS interface, actual FDS emulation is in FDSSound.cpp

CFDS::CFDS(CMixer *pMixer) : CChannel(pMixer, SNDCHIP_FDS, CHANID_FDS)
{
	m_pRegisterLogger->AddRegisterRange(0x4040, 0x408F);		// // //
	FDSSoundInstall3();
}

CFDS::~CFDS()
{
}

void CFDS::Reset()
{
	FDSSoundReset();
	FDSSoundVolume(0);
}

void CFDS::Write(uint16_t Address, uint8_t Value)
{
	FDSSoundWrite(Address, Value);
}

uint8_t CFDS::Read(uint16_t Address, bool &Mapped)
{
	Mapped = ((0x4040 <= Address && Address <= 0x407f) || (0x4090 == Address) || (0x4092 == Address));
	return FDSSoundRead(Address);
}

void CFDS::EndFrame()
{
	CChannel::EndFrame();
}

void CFDS::Process(uint32_t Time)
{
	if (!Time)
		return;

	while (Time--) {
		Mix(FDSSoundRender() >> 12);
		++m_iTime;
	}
}

double CFDS::GetFreq(int Channel) const		// // //
{
	if (Channel) return 0.;
	int Lo = m_pRegisterLogger->GetRegister(0x4082)->GetValue();
	int Hi = m_pRegisterLogger->GetRegister(0x4083)->GetValue();
	if (Hi & 0x80)
		return 0.;
	Lo |= (Hi << 8) & 0xF00;
	return CAPU::BASE_FREQ_NTSC * (Lo / 4194304.);
}
