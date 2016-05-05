/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

#include "2A03Chan.h"		// // //

class CDPCM : public C2A03Chan {
public:
	CDPCM(CMixer *pMixer, int ID);		// // //
	~CDPCM();

	void	Reset();
	void	Write(uint16_t Address, uint8_t Value);
	void	WriteControl(uint8_t Value);
	uint8_t	ReadControl() const;
	uint8_t	DidIRQ() const;
	void	Process(uint32_t Time);
	void	Reload();

	CSampleMem *GetSampleMemory() const;		// // //
	uint8_t	GetSamplePos() const { return  (m_iDMA_Address - (m_iDMA_LoadReg << 6 | 0x4000)) >> 6; };
	uint8_t	GetDeltaCounter() const { return m_iDeltaCounter; };
	bool	IsPlaying() const { return (m_iDMA_BytesRemaining > 0); };

public:
	static const uint16_t	DMC_PERIODS_NTSC[];
	static const uint16_t	DMC_PERIODS_PAL[];

	const uint16_t *PERIOD_TABLE;

private:
	uint8_t	m_iBitDivider;
	uint8_t	m_iShiftReg;
	uint8_t	m_iPlayMode;
	uint8_t	m_iDeltaCounter;
	uint8_t	m_iSampleBuffer;

	uint16_t	m_iDMA_LoadReg;
	uint16_t	m_iDMA_LengthReg;
	uint16_t	m_iDMA_Address;
	uint16_t	m_iDMA_BytesRemaining;

	bool	m_bTriggeredIRQ, m_bSampleFilled, m_bSilenceFlag;

	// Needed by FamiTracker 
	CSampleMem	*m_pSampleMem;
};
