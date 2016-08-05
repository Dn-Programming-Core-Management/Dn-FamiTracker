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
// Derived channels, VRC7
//

enum vrc7_command_t {
	CMD_NONE, 
	CMD_NOTE_ON,
	CMD_NOTE_TRIGGER,
	CMD_NOTE_OFF, 
	CMD_NOTE_HALT,
	CMD_NOTE_RELEASE
};

class CChannelHandlerInterfaceVRC7;

class CChannelHandlerVRC7 : public CChannelHandlerInverted, public CChannelHandlerInterfaceVRC7 {		// // //
public:
	CChannelHandlerVRC7();
	void	SetChannelID(int ID) override;

	void	SetPatch(unsigned char Patch);		// // //
	void	SetCustomReg(size_t Index, unsigned char Val);		// // //

protected:
	void	HandleNoteData(stChanNote *pNoteData, int EffColumns) override;
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	void	HandleNote(int Note, int Octave) override;
	int		RunNote(int Octave, int Note) override;		// // //
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	void	SetupSlide() override;		// // //
	int		CalculateVolume() const override;
	int		CalculatePeriod() const override;		// // //

	void	UpdateNoteRelease() override;		// // //
	int		TriggerNote(int Note) override;

protected:
	void CorrectOctave();		// // //
	unsigned int GetFnum(int Note) const;

protected:
	static bool m_bRegsDirty;

protected:
	unsigned char m_iChannel;
	unsigned char m_iPatch;

	unsigned char m_iRegs[8];

	bool	m_bHold;

	vrc7_command_t m_iCommand;

	int		m_iTriggeredNote;
	int		m_iOctave;
	int		m_iOldOctave;		// // //
};

class CVRC7Channel : public CChannelHandlerVRC7 {
public:
	CVRC7Channel() : CChannelHandlerVRC7() {};
	void RefreshChannel();
protected:
	void ClearRegisters();
private:
	void RegWrite(unsigned char Reg, unsigned char Value);
};

