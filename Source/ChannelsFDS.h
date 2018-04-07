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

class CChannelHandlerFDS : public CChannelHandlerInverted, public CChannelHandlerInterfaceFDS {
public:
	CChannelHandlerFDS();
	virtual void RefreshChannel();
protected:
	void	HandleNoteData(stChanNote *pNoteData, int EffColumns) override;
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	int		CalculateVolume() const override;		// // //
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	void	ClearRegisters() override;
	CString	GetCustomEffectString() const override;		// // //

public:		// // //
	// FDS functions
	void SetFMSpeed(int Speed);
	void SetFMDepth(int Depth);
	void SetFMDelay(int Delay);
	// void SetFMEnable(bool Enable);
	void FillWaveRAM(const char *pBuffer);		// // //
	void FillModulationTable(const char *pBuffer);		// // //
protected:
	// FDS control variables
	int m_iModulationSpeed;
	int m_iModulationDepth;
	int m_iModulationDelay;
	// Modulation table
	char m_iModTable[32];
	char m_iWaveTable[64];
protected:
	int m_iVolModMode;		// // // 0CC: make an enum for this
	int m_iVolModRate;
	bool m_bVolModTrigger;

	bool m_bAutoModulation;		// // //
	int m_iModulationOffset;

	int m_iEffModDepth;
	int m_iEffModSpeedHi;
	int m_iEffModSpeedLo;
};
