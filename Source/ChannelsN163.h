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

//
// Derived channels, N163
//

class CChannelHandlerN163 : public FrequencyChannelHandler, public CChannelHandlerInterfaceN163 {
public:
	CChannelHandlerN163();
	void	RefreshChannel() override;
	void	ResetChannel() override;

	void	SetWaveLength(int Length);		// // //
	void	SetWavePosition(int Pos);
	void	SetWaveCount(int Count);
	void	FillWaveRAM(const char *Buffer, int Count);

	void	SetChannelCount(int Count);		// // //

	int getDutyMax() const override;

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
	int		CalculatePeriod(bool MultiplyByHarmonic = true) const override;		// // //
	CString	GetSlideEffectString() const override;		// // //
	CString	GetCustomEffectString() const override;		// // //

	void	resetPhase();

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
