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

#ifndef DPCM_H
#define DPCM_H

#include "Channel.h"

class CDPCM : public CChannel {
public:
	CDPCM(CMixer *pMixer, CSampleMem *pSampleMem, int ID);
	~CDPCM();

	void	Reset();
	void	Write(uint16 Address, uint8 Value);
	void	WriteControl(uint8 Value);
	uint8	ReadControl() const;
	uint8	DidIRQ() const;
	void	Process(uint32 Time);
	void	Reload();

	uint8	GetSamplePos() const { return  (m_iDMA_Address - (m_iDMA_LoadReg << 6 | 0x4000)) >> 6; };
	uint8	GetDeltaCounter() const { return m_iDeltaCounter; };
	bool	IsPlaying() const { return (m_iDMA_BytesRemaining > 0); };

public:
	static const uint16	DMC_PERIODS_NTSC[];
	static const uint16	DMC_PERIODS_PAL[];

	const uint16 *PERIOD_TABLE;

private:
	uint8	m_iBitDivider;
	uint8	m_iShiftReg;
	uint8	m_iPlayMode;
	uint8	m_iDeltaCounter;
	uint8	m_iSampleBuffer;

	uint16	m_iDMA_LoadReg;
	uint16	m_iDMA_LengthReg;
	uint16	m_iDMA_Address;
	uint16	m_iDMA_BytesRemaining;

	bool	m_bTriggeredIRQ, m_bSampleFilled, m_bSilenceFlag;

	// Needed by FamiTracker 
	CSampleMem	*m_pSampleMem;
};

#endif /* DPCM_H */
