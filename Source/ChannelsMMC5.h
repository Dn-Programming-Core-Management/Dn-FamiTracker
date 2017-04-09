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

#include "ChannelHandler.h"

//
// Derived channels, MMC5
//

class CChannelHandlerMMC5 : public CChannelHandler {
public:
	CChannelHandlerMMC5();
	void	ResetChannel() override;
	void	RefreshChannel() override;

protected:
	void	HandleNoteData(stChanNote *pNoteData, int EffColumns) override;
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	int		ConvertDuty(int Duty) const override;		// // //
	void	ClearRegisters() override;
	std::string	GetCustomEffectString() const override;		// // //

protected:
	// // //
	bool m_bHardwareEnvelope;	// // // (constant volume flag, bit 4)
	bool m_bEnvelopeLoop;		// // // (halt length counter flag, bit 5 / triangle bit 7)
	bool m_bResetEnvelope;		// // //
	int  m_iLengthCounter;		// // //
	int	 m_iLastPeriod;			// // // moved to subclass
};