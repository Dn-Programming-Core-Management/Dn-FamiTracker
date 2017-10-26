/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "APU.h"
#include "DPCM.h"

const uint16_t CDPCM::DMC_PERIODS_NTSC[] = {
	428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106, 84, 72, 54
};

const uint16_t CDPCM::DMC_PERIODS_PAL[]  = {
	398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118, 98, 78, 66, 50
};

CDPCM::CDPCM(CMixer *pMixer, int ID) :		// // //
	C2A03Chan(pMixer, SNDCHIP_NONE, ID),
	m_pSampleMem(new CSampleMem { })
{
	PERIOD_TABLE = DMC_PERIODS_NTSC;

	Reset();
}

CDPCM::~CDPCM()
{
	if (m_pSampleMem)		// // //
		delete m_pSampleMem;
}

void CDPCM::Reset()
{
	m_iCounter = m_iPeriod = DMC_PERIODS_NTSC[0];

	m_iBitDivider = m_iShiftReg = 0;
	m_iDMA_LoadReg = 0;
	m_iDMA_LengthReg = 0;
	m_iDMA_Address = 0;
	m_iDMA_BytesRemaining = 0;
	
	m_bTriggeredIRQ = m_bSampleFilled = false;

	// loaded with 0 on power-up.
	m_iDeltaCounter = 0;
	
	Mix(0);		// // //
	EndFrame();
}

void CDPCM::Write(uint16_t Address, uint8_t Value)
{
	switch (Address) {
		case 0x00:
			m_iPlayMode = Value & 0xC0;
			m_iPeriod = PERIOD_TABLE[Value & 0x0F];
			if ((Value & 0x80) == 0x00) 
				m_bTriggeredIRQ = false;
			break;
		case 0x01:
			m_iDeltaCounter = Value & 0x7F;
			Mix(m_iDeltaCounter);
			break;
		case 0x02:
			m_iDMA_LoadReg = Value;
			break;
		case 0x03:
			m_iDMA_LengthReg = Value;
			break;
	}
}

void CDPCM::WriteControl(uint8_t Value)
{
	if ((Value & 1) == 1) {
		if (m_iDMA_BytesRemaining == 0)
			Reload();
	}
	else {
		m_iDMA_BytesRemaining = 0;
	}

	m_bTriggeredIRQ = false;
}

uint8_t CDPCM::ReadControl() const
{
	return (m_iDMA_BytesRemaining > 0) ? 1 : 0;
}

uint8_t CDPCM::DidIRQ() const
{
	return m_bTriggeredIRQ ? 1 : 0;
}

void CDPCM::Reload()
{
	m_iDMA_Address = (m_iDMA_LoadReg << 6) | 0x4000;
	m_iDMA_BytesRemaining = (m_iDMA_LengthReg << 4) + 1;
}

CSampleMem *CDPCM::GetSampleMemory() const
{
	return m_pSampleMem;
}

void CDPCM::Process(uint32_t Time)
{
	while (Time >= m_iCounter) {
		Time	  -= m_iCounter;
		m_iTime	  += m_iCounter;
		m_iCounter = m_iPeriod;

		// DMA reader
		// Check if a new byte should be fetched
		if (!m_bSampleFilled && (m_iDMA_BytesRemaining > 0)) {

			m_iSampleBuffer = m_pSampleMem->Read(m_iDMA_Address | 0x8000);
//			m_pEmulator->ConsumeCycles(4);
			m_iDMA_Address = (m_iDMA_Address + 1) & 0x7FFF;
			--m_iDMA_BytesRemaining;
			m_bSampleFilled = true;

			if (!m_iDMA_BytesRemaining) {
				switch (m_iPlayMode) {
					case 0x00:	// Stop
						break;
					case 0x40:	// Loop
					case 0xC0:
						Reload();
						break;
					case 0x80:	// Stop and do IRQ (not when an NSF is playing)
						m_bTriggeredIRQ = true;
						break;
				}
			}
		}

		// Output unit
		if (!m_iBitDivider) {
			// Begin new output cycle
			m_iBitDivider = 8;
			if (m_bSampleFilled) {
				m_iShiftReg		= m_iSampleBuffer;
				m_bSampleFilled = false;
				m_bSilenceFlag	= false;
			}
			else {
				m_bSilenceFlag = true;
			}
		}

		if (!m_bSilenceFlag) {
			if ((m_iShiftReg & 1) == 1) {
				if (m_iDeltaCounter < 126)
					m_iDeltaCounter += 2;
			}
			else {
				if (m_iDeltaCounter > 1)
					m_iDeltaCounter -= 2;
			}
		}

		m_iShiftReg >>= 1;
		--m_iBitDivider;

		Mix(m_iDeltaCounter);
	}

	m_iCounter -= Time;
	m_iTime += Time;
}

double CDPCM::GetFrequency() const		// // //
{
	if (!m_bSampleFilled && !m_iDMA_BytesRemaining)
		return 0.;
	double Rate = PERIOD_TABLE == DMC_PERIODS_PAL ? CAPU::BASE_FREQ_PAL : CAPU::BASE_FREQ_NTSC;
	return Rate / m_iPeriod;
}
