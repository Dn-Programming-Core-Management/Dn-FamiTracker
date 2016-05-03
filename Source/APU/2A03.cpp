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
#include "../common.h"
#include <algorithm>
#include "Mixer.h"
#include "Square.h"
#include "Triangle.h"
#include "Noise.h"
#include "DPCM.h"
#include "2A03.h"
#include "../RegisterState.h"		// // //

// // // 2A03 sound chip class

C2A03::C2A03(CMixer *pMixer) :
	CSoundChip(pMixer),
	m_pSquare1(new CSquare(m_pMixer, CHANID_SQUARE1, SNDCHIP_NONE)),
	m_pSquare2(new CSquare(m_pMixer, CHANID_SQUARE2, SNDCHIP_NONE)),
	m_pTriangle(new CTriangle(m_pMixer, CHANID_TRIANGLE)),
	m_pNoise(new CNoise(m_pMixer, CHANID_NOISE)),
	m_pDPCM(new CDPCM(m_pMixer, CHANID_DPCM))		// // //
{
	m_pRegisterLogger->AddRegisterRange(0x4000, 0x4017);		// // //
}

C2A03::~C2A03()
{
	if (m_pSquare1) delete m_pSquare1;
	if (m_pSquare2) delete m_pSquare2;
	if (m_pTriangle) delete m_pTriangle;
	if (m_pNoise) delete m_pNoise;
	if (m_pDPCM) delete m_pDPCM;
}

void C2A03::Reset()
{
	m_iFrameSequence	= 0;
	m_iFrameMode		= 0;

	m_pSquare1->Reset();
	m_pSquare2->Reset();
	m_pTriangle->Reset();
	m_pNoise->Reset();
	m_pDPCM->Reset();
}

void C2A03::Process(uint32_t Time)
{
	RunAPU1(Time);
	RunAPU2(Time);
}

void C2A03::EndFrame()
{
	m_pSquare1->EndFrame();
	m_pSquare2->EndFrame();
	m_pTriangle->EndFrame();
	m_pNoise->EndFrame();
	m_pDPCM->EndFrame();
}

void C2A03::Write(uint16_t Address, uint8_t Value)
{
	if (Address < 0x4000U || Address > 0x401FU) return;
	switch (Address) {
	case 0x4015:
		m_pSquare1->WriteControl(Value);
		m_pSquare2->WriteControl(Value >> 1);
		m_pTriangle->WriteControl(Value >> 2);
		m_pNoise->WriteControl(Value >> 3);
		m_pDPCM->WriteControl(Value >> 4);
		return;
	case 0x4017:
		m_iFrameSequence = 0;
		if (Value & 0x80) {
			m_iFrameMode = 1;
			Clock_240Hz();
			Clock_120Hz();
			Clock_60Hz();
		}
		else
			m_iFrameMode = 0;
		return;
	}

	switch (Address & 0x1C) {
		case 0x00: m_pSquare1->Write(Address & 0x03, Value); break;
		case 0x04: m_pSquare2->Write(Address & 0x03, Value); break;
		case 0x08: m_pTriangle->Write(Address & 0x03, Value); break;
		case 0x0C: m_pNoise->Write(Address & 0x03, Value); break;
		case 0x10: m_pDPCM->Write(Address & 0x03, Value); break;
	}
}

uint8_t C2A03::Read(uint16_t Address, bool &Mapped)
{
	switch (Address) {
	case 0x4015:
	{
		uint8_t RetVal;

		RetVal = m_pSquare1->ReadControl();
		RetVal |= m_pSquare2->ReadControl() << 1;
		RetVal |= m_pTriangle->ReadControl() << 2;
		RetVal |= m_pNoise->ReadControl() << 3;
		RetVal |= m_pDPCM->ReadControl() << 4;
		RetVal |= m_pDPCM->DidIRQ() << 7;

		Mapped = true;
		return RetVal;
	}
	}
	return 0U;
}

void C2A03::ClockSequence()
{
	if (m_iFrameMode == 0) {
		m_iFrameSequence = (m_iFrameSequence + 1) % 4;
		switch (m_iFrameSequence) {
			case 0: Clock_240Hz(); break;
			case 1: Clock_240Hz(); Clock_120Hz(); break;
			case 2: Clock_240Hz(); break;
			case 3: Clock_240Hz(); Clock_120Hz(); Clock_60Hz(); break;
		}
	}
	else {
		m_iFrameSequence = (m_iFrameSequence + 1) % 5;
		switch (m_iFrameSequence) {
			case 0: Clock_240Hz(); Clock_120Hz(); break;
			case 1: Clock_240Hz(); break;
			case 2: Clock_240Hz(); Clock_120Hz(); break;
			case 3: Clock_240Hz(); break;
			case 4: break;
		}
	}
}

void C2A03::ChangeMachine(int Machine)
{
	switch (Machine) {
		case MACHINE_NTSC:
			m_pNoise->PERIOD_TABLE = CNoise::NOISE_PERIODS_NTSC;
			m_pDPCM->PERIOD_TABLE = CDPCM::DMC_PERIODS_NTSC;			
			m_pMixer->SetClockRate(1789773);
			break;
		case MACHINE_PAL:
			m_pNoise->PERIOD_TABLE = CNoise::NOISE_PERIODS_PAL;
			m_pDPCM->PERIOD_TABLE = CDPCM::DMC_PERIODS_PAL;			
			m_pMixer->SetClockRate(1662607);
			break;
	}
}

inline void C2A03::Clock_240Hz() const
{
	m_pSquare1->EnvelopeUpdate();
	m_pSquare2->EnvelopeUpdate();
	m_pNoise->EnvelopeUpdate();
	m_pTriangle->LinearCounterUpdate();
}

inline void C2A03::Clock_120Hz() const
{
	m_pSquare1->SweepUpdate(1);
	m_pSquare2->SweepUpdate(0);

	m_pSquare1->LengthCounterUpdate();
	m_pSquare2->LengthCounterUpdate();
	m_pTriangle->LengthCounterUpdate();
	m_pNoise->LengthCounterUpdate();
}

inline void C2A03::Clock_60Hz() const
{
	// IRQ
}

inline void C2A03::RunAPU1(uint32_t Time)
{
	// APU pin 1
	while (Time > 0) {
		uint32_t Period = std::min(m_pSquare1->GetPeriod(), m_pSquare2->GetPeriod());
		Period = std::min<uint32_t>(std::max<uint32_t>(Period, 7), Time);
		m_pSquare1->Process(Period);
		m_pSquare2->Process(Period);
		Time -= Period;
	}
}

inline void C2A03::RunAPU2(uint32_t Time)
{
	// APU pin 2
	while (Time > 0) {
		uint32_t Period = std::min(m_pTriangle->GetPeriod(), m_pNoise->GetPeriod());
		Period = std::min<uint32_t>(Period, m_pDPCM->GetPeriod());
		Period = std::min<uint32_t>(std::max<uint32_t>(Period, 7), Time);
		m_pTriangle->Process(Period);
		m_pNoise->Process(Period);
		m_pDPCM->Process(Period);
		Time -= Period;
	}
}

CSampleMem *C2A03::GetSampleMemory() const		// // //
{
	return m_pDPCM->GetSampleMemory();
}

uint8_t C2A03::GetSamplePos() const
{
	return m_pDPCM->GetSamplePos();
}

uint8_t C2A03::GetDeltaCounter() const
{
	return m_pDPCM->GetDeltaCounter();
}

bool C2A03::DPCMPlaying() const
{
	return m_pDPCM->IsPlaying();
}
