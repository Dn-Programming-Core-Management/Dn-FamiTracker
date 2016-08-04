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

//
// Derived channels, VRC6
//

class CChannelHandlerVRC6 : public CChannelHandler {
public:
	CChannelHandlerVRC6(int MaxPeriod, int MaxVolume);		// // //

protected:
	// // //
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	// // //
	void	ClearRegisters() override;		// // //
};

class CVRC6Square : public CChannelHandlerVRC6 {
public:
	CVRC6Square() : CChannelHandlerVRC6(0xFFF, 0x0F) { }
	void	RefreshChannel() override;
protected:
	int		ConvertDuty(int Duty) const override;		// // //
};

class CVRC6Sawtooth : public CChannelHandlerVRC6 {
public:
	CVRC6Sawtooth() : CChannelHandlerVRC6(0xFFF, 0x3F) { }
	void	RefreshChannel() override;
protected:
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	int		CalculateVolume() const override;
};
