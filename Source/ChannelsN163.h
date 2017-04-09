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
// Derived channels, N163
//

class CChannelHandlerN163 : public CChannelHandlerInverted, public CChannelHandlerInterfaceN163 {
public:
	CChannelHandlerN163();
	void	RefreshChannel() override;
	void	ResetChannel() override;

	void	SetWaveLength(int Length);		// // //
	void	SetWavePosition(int Pos);
	void	SetWaveCount(int Count);
	void	FillWaveRAM(const char *Buffer, int Count);

	void	SetChannelCount(int Count);		// // //

protected:
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	bool	HandleInstrument(bool Trigger, bool NewInstrument) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	void	HandleNote(int Note, int Octave) override;
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	void	SetupSlide() override;		// // //
	int		ConvertDuty(int Duty) const override;		// // //
	void	ClearRegisters() override;
	int		CalculatePeriod() const override;		// // //
	std::string	GetSlideEffectString() const override;		// // //
	std::string	GetCustomEffectString() const override;		// // //

private:
	void WriteReg(int Reg, int Value);
	void SetAddress(char Addr, bool AutoInc);
	void WriteData(char Data);
	void WriteData(int Addr, char Data);
private:
	inline int GetIndex() const { return m_iChannelID - CHANID_N163_CH1; }
private:
	bool m_bLoadWave;
	bool m_bDisableLoad;		// // //
	int m_iChannels;
	int m_iWaveLen;
	int m_iWavePos;
	int m_iWavePosOld;			// // //
	int m_iWaveCount;
protected:
	// // //

	bool m_bResetPhase;
};
