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

class CChannelHandlerFDS : public FrequencyChannelHandler, public CChannelHandlerInterfaceFDS {
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

	void	resetPhase();
	void	writeModTable();

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

	/// this code needs Rust enums.
	/// If m_bAutoModulation is true, then m_iEffModSpeedHi / (m_iEffModSpeedLo+1)
	/// act as a numerator/denominator.
	/// Otherwise, they act as high/low bytes.
	bool m_bAutoModulation;		// // //
	int m_iModulationOffset;

	int m_iEffModDepth;
	int m_iEffModSpeedHi;
	int m_iEffModSpeedLo;
};
