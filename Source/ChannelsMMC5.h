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
// Derived channels, MMC5
//

class CChannelHandlerMMC5 : public CChannelHandler {
public:
	CChannelHandlerMMC5();
	void	ResetChannel() override;
	void	RefreshChannel() override;
	int getDutyMax() const override;
protected:
	static const char MAX_DUTY;

	void	HandleNoteData(stChanNote *pNoteData, int EffColumns) override;
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	int		ConvertDuty(int Duty) const override;		// // //
	void	ClearRegisters() override;
	CString	GetCustomEffectString() const override;		// // //

	void	resetPhase();

protected:
	// // //
	bool m_bHardwareEnvelope;	// // // (constant volume flag, bit 4)
	bool m_bEnvelopeLoop;		// // // (halt length counter flag, bit 5 / triangle bit 7)
	bool m_bResetEnvelope;		// // //
	int  m_iLengthCounter;		// // //
	int	 m_iLastPeriod;			// // // moved to subclass
};