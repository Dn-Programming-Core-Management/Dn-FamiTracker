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

class CChannelInterfaceFDS;

class CChannelHandlerFDS : public CChannelHandlerInverted {
public:
	CChannelHandlerFDS();
	virtual void RefreshChannel();
	friend CChannelInterfaceFDS;
protected:
	virtual void HandleNoteData(stChanNote *pNoteData, int EffColumns);
	virtual void HandleCustomEffects(int EffNum, int EffParam);
	virtual void HandleEmptyNote();
	virtual void HandleCut();
	virtual void HandleRelease();
	virtual void HandleNote(int Note, int Octave);
	bool         CreateInstHandler(inst_type_t Type);		// // //
	virtual void ClearRegisters();
	virtual CString GetCustomEffectString() const;		// // //

protected:
	// FDS functions
	void FillWaveRAM(const CInstrumentFDS *pInst);
	void FillModulationTable(const CInstrumentFDS *pInst);
private:
	void CheckWaveUpdate();
protected:
	// FDS control variables
	int m_iModulationSpeed;
	int m_iModulationDepth;
	int m_iModulationDelay;
	// FDS sequences
	//CSequence *m_pVolumeSeq;
	//CSequence *m_pArpeggioSeq;
	//CSequence *m_pPitchSeq;
	// Modulation table
	char m_iModTable[32];
	char m_iWaveTable[64];
	// Modulation
	bool m_bResetMod;
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
