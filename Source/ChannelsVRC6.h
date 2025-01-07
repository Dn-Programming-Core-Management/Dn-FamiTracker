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
	uint16_t getAddress();
	void	ClearRegisters() override;		// // //
	void	resetPhase();
};

class CVRC6Square : public CChannelHandlerVRC6 {
public:
	CVRC6Square() : CChannelHandlerVRC6(0xFFF, 0x0F) { }
	void	RefreshChannel() override;
	int getDutyMax() const override;
protected:
	static const char MAX_DUTY;		// TODO remove class constant, move to .cpp file

	int		ConvertDuty(int Duty) const override;		// // //
};

class CVRC6Sawtooth : public CChannelHandlerVRC6 {
public:
	CVRC6Sawtooth() : CChannelHandlerVRC6(0xFFF, 0x3F) { }
	void	RefreshChannel() override;
	int getDutyMax() const override;
protected:
	static const char MAX_DUTY;		// TODO remove class constant, move to .cpp file

	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	int		CalculateVolume() const override;
};
