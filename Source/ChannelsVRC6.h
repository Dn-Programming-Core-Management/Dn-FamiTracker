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
	CChannelHandlerVRC6();

protected:
	// // //
	virtual bool HandleEffect(effect_t EffNum, unsigned char EffParam);		// // //
	virtual void HandleEmptyNote();
	virtual void HandleCut();
	virtual void HandleRelease();
	virtual void HandleNote(int Note, int Octave);
	bool         CreateInstHandler(inst_type_t Type);		// // //
	// // //
	void ClearRegisters();		// // //
};

class CVRC6Square : public CChannelHandlerVRC6 {
public:
	CVRC6Square() : CChannelHandlerVRC6() { m_iDefaultDuty = 0; };
	void RefreshChannel();
protected:
	virtual int ConvertDuty(int Duty) const;		// // //
private:
};

class CVRC6Sawtooth : public CChannelHandlerVRC6 {
public:
	CVRC6Sawtooth() : CChannelHandlerVRC6() { m_iDefaultDuty = 0; };
	void RefreshChannel();
protected:
private:
};
