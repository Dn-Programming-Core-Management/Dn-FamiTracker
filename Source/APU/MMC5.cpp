/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

//#include "APU.h"
#include "../Common.h"
#include "Types.h"
#include "Mixer.h"
#include "MMC5.h"
#include "Square.h"
#include "../RegisterState.h"		// // //

// MMC5 external sound

CMMC5::CMMC5(CMixer *pMixer) :
	CSoundChip(pMixer),		// // //
	m_pEXRAM(new uint8_t[0x400]),
	m_pSquare1(new CSquare(pMixer, CHANID_MMC5_SQUARE1, SNDCHIP_MMC5)),
	m_pSquare2(new CSquare(pMixer, CHANID_MMC5_SQUARE2, SNDCHIP_MMC5)),
	m_iMulLow(0),
	m_iMulHigh(0)
{
	m_pRegisterLogger->AddRegisterRange(0x5000, 0x5007);		// // //
	m_pRegisterLogger->AddRegisterRange(0x5015, 0x5015);
}

CMMC5::~CMMC5()
{
	if (m_pSquare1)
		delete m_pSquare1;

	if (m_pSquare2)
		delete m_pSquare2;

	if (m_pEXRAM)
		delete [] m_pEXRAM;
}

void CMMC5::Reset()
{
	m_pSquare1->Reset();
	m_pSquare2->Reset();

	m_pSquare1->Write(0x01, 0x08);
	m_pSquare2->Write(0x01, 0x08);
}

void CMMC5::Write(uint16_t Address, uint8_t Value)
{
	if (Address >= 0x5C00 && Address <= 0x5FF5) {
		m_pEXRAM[Address & 0x3FF] = Value;
		return;
	}

	switch (Address) {
		// Channel 1
		case 0x5000:
			m_pSquare1->Write(0, Value);
			break;
		case 0x5002:
			m_pSquare1->Write(2, Value);
			break;
		case 0x5003:
			m_pSquare1->Write(3, Value);
			break;
		// Channel 2
		case 0x5004:
			m_pSquare2->Write(0, Value);
			break;
		case 0x5006:
			m_pSquare2->Write(2, Value);
			break;
		case 0x5007:
			m_pSquare2->Write(3, Value);
			break;
		// Channel 3... (doesn't exist)
		// Control
		case 0x5015:
			m_pSquare1->WriteControl(Value & 1);
			m_pSquare2->WriteControl((Value >> 1) & 1);
			break;
		// Hardware multiplier
		case 0x5205:
			m_iMulLow = Value;
			break;
		case 0x5206:
			m_iMulHigh = Value;
			break;
	}
}

uint8_t CMMC5::Read(uint16_t Address, bool &Mapped)
{
	if (Address >= 0x5C00 && Address <= 0x5FF5) {
		Mapped = true;
		return m_pEXRAM[Address & 0x3FF];
	}
	
	switch (Address) {
		case 0x5205:
			Mapped = true;
			return (m_iMulLow * m_iMulHigh) & 0xFF;
		case 0x5206:
			Mapped = true;
			return (m_iMulLow * m_iMulHigh) >> 8;
	}

	return 0;
}

void CMMC5::EndFrame()
{
	m_pSquare1->EndFrame();
	m_pSquare2->EndFrame();
}

void CMMC5::Process(uint32_t Time)
{
	m_pSquare1->Process(Time);
	m_pSquare2->Process(Time);
}

double CMMC5::GetFreq(int Channel) const		// // //
{
	switch (Channel) {
	case 0: return m_pSquare1->GetFrequency();
	case 1: return m_pSquare2->GetFrequency();
	}
	return 0.;
}

void CMMC5::LengthCounterUpdate()
{
	m_pSquare1->LengthCounterUpdate();
	m_pSquare2->LengthCounterUpdate();
}

void CMMC5::EnvelopeUpdate()
{
	m_pSquare1->EnvelopeUpdate();
	m_pSquare2->EnvelopeUpdate();
}

void CMMC5::ClockSequence()
{
	EnvelopeUpdate();		// // //
	LengthCounterUpdate();		// // //
}
