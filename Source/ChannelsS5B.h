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
// Derived channels, 5B
//

class CChannelHandlerS5B : public CChannelHandler {
public:
	CChannelHandlerS5B();
	virtual void ResetChannel();
	virtual void RefreshChannel();

protected:
	// // //
	virtual bool HandleEffect(effect_t EffNum, unsigned char EffParam);		// // //
	virtual void HandleEmptyNote();
	virtual void HandleCut();
	virtual void HandleRelease();
	virtual void HandleNote(int Note, int Octave);
	bool         CreateInstHandler(inst_type_t Type);		// // //
	
	virtual int ConvertDuty(int Duty) const;		// // //
	void ClearRegisters();
	virtual CString GetCustomEffectString() const;		// // //

protected:
	void WriteReg(int Reg, int Value);

	// Static functions
protected:	
	static void SetEnvelopeHigh(int Val);
	static void SetEnvelopeLow(int Val);
	static void SetEnvelopeType(int Val);
	static void SetMode(int Chan, int Square, int Noise);
	static void SetNoiseFreq(int Freq);
	void UpdateRegs(CAPU *pAPU);		// // //

	// Static memebers
protected:
	static int m_iModes;
	static int m_iNoiseFreq;
	static unsigned char m_iEnvFreqHi;
	static unsigned char m_iEnvFreqLo;
	static int m_iEnvType;
	static bool m_bRegsDirty;

	// Instance members
protected:
	int m_iNoiseOffset;
	bool m_bEnvEnable;		// unused

	bool m_bUpdate;
/*
	unsigned char m_cSweep;
	unsigned char m_cDutyCycle, m_iDefaultDuty;

	int ModEnable[SEQ_COUNT];
	int	ModIndex[SEQ_COUNT];
	int	ModDelay[SEQ_COUNT];
	int	ModPointer[SEQ_COUNT];
	*/

};