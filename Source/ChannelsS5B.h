/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#pragma once

//
// Derived channels, 5B
//

class CChannelHandlerS5B : public CChannelHandler, public CChannelHandlerInterfaceS5B {
public:
	CChannelHandlerS5B();
	void	ResetChannel() override;
	void	RefreshChannel() override;

	void	SetNoiseFreq(int Pitch) override final;		// // //

	int getDutyMax() const override;
protected:
	static const char MAX_DUTY;		// TODO remove class constant, move to .cpp file

	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleNote(int Note, int Octave) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	bool	CreateInstHandler(inst_type_t Type) override;		// // //
	
	int		CalculateVolume() const override;		// // //
	int		ConvertDuty(int Duty) const override;		// // //
	void	ClearRegisters() override;
	CString	GetCustomEffectString() const override;		// // //

protected:
	void WriteReg(int Reg, int Value);

	// Static functions
protected:	
	static void SetMode(int Chan, int Square, int Noise);
	void UpdateAutoEnvelope(int Period);		// // // 050B
	void UpdateRegs();		// // //

	// Static memebers
protected:
	static int s_iModes;
	static int s_iNoiseFreq;
	static int s_iNoisePrev;		// // //
	static int s_iDefaultNoise;		// // //
	static unsigned char s_iEnvFreqHi;
	static unsigned char s_iEnvFreqLo;
	static bool s_bEnvTrigger;		// // // 050B
	static int s_iEnvType;
	static int s_unused;		// // // 050B, unused

	// Instance members
protected:
	bool m_bEnvelopeEnabled;		// // // 050B
	int m_iAutoEnvelopeShift;		// // // 050B
	bool m_bUpdate;
};