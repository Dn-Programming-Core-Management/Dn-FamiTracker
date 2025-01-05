/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/


#pragma once

#include "SoundChip.h"
#include "Channel.h"

class CVRC6_Pulse : public CChannel {
public:
	CVRC6_Pulse(CMixer *pMixer, int ID);
	void Reset();
	void Write(uint16_t Address, uint8_t Value);
	void Process(int Time);
	double GetFrequency() const;		// // //

private:
	uint8_t	m_iDutyCycle, 
			m_iVolume, 
			m_iGate, 
			m_iEnabled;
	uint32_t	m_iPeriod;
	uint8_t	m_iPeriodLow, 
			m_iPeriodHigh;
	int32_t	m_iCounter;
	uint8_t	m_iDutyCycleCounter;
};

class CVRC6_Sawtooth : public CChannel {
public:
	CVRC6_Sawtooth(CMixer *pMixer, int ID);
	void Reset();
	void Write(uint16_t Address, uint8_t Value);
	void Process(int Time);
	double GetFrequency() const;		// // //

private:
	uint8_t	m_iPhaseAccumulator, 
			m_iPhaseInput, 
			m_iEnabled, 
			m_iResetReg;
	uint32_t	m_iPeriod;
	uint8_t	m_iPeriodLow, 
			m_iPeriodHigh;
	int32_t	m_iCounter;
};

class CVRC6 : public CSoundChip {
public:
	CVRC6(CMixer *pMixer);
	virtual ~CVRC6();
	void Reset();
	void Write(uint16_t Address, uint8_t Value);
	uint8_t Read(uint16_t Address, bool &Mapped);
	void EndFrame();
	void Process(uint32_t Time);
	double GetFreq(int Channel) const override;		// // //

private:
	CVRC6_Pulse	*m_pPulse1, *m_pPulse2;
	CVRC6_Sawtooth *m_pSawtooth;
};
