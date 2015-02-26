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

#ifndef MMC5_H
#define MMC5_H

#include "external.h"
#include "channel.h"

class CMMC5 : public CExternal {
public:
	CMMC5(CMixer *pMixer);
	virtual ~CMMC5();

	void Reset();
	void Write(uint16 Address, uint8 Value);
	uint8 Read(uint16 Address, bool &Mapped);
	uint8 GetLocalReg(uint8 Address);		// // //
	void EndFrame();
	void Process(uint32 Time);
	void LengthCounterUpdate();
	void EnvelopeUpdate();

private:	
	CSquare	*m_pSquare1;
	CSquare	*m_pSquare2;
	uint8	*m_pEXRAM;
	uint8	m_iMulLow;
	uint8	m_iMulHigh;

	uint8	m_iLocalReg[8];		// // //
};

#endif /* MMC5_H */