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
// Derived channels, 2A03
//

class CChannelHandler2A03 : public CChannelHandler {
public:
	CChannelHandler2A03();
	virtual void ResetChannel();

protected:
	void	HandleNoteData(stChanNote *pNoteData, int EffColumns) override;
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	bool	CreateInstHandler(inst_type_t Type) override;		// // //

protected:
	// // //
	bool	m_bHardwareEnvelope;	// // // (constant volume flag, bit 4)
	bool	m_bEnvelopeLoop;		// // // (halt length counter flag, bit 5 / triangle bit 7)
	bool	m_bResetEnvelope;		// // //
	int		m_iLengthCounter;		// // //
};

// // // 2A03 Square
class C2A03Square : public CChannelHandler2A03 {
public:
	C2A03Square();
	void	RefreshChannel() override;
	void	SetChannelID(int ID) override;		// // //
	int getDutyMax() const override;
protected:
	static const char MAX_DUTY;

	int		ConvertDuty(int Duty) const override;		// // //
	void	ClearRegisters() override;

	void	HandleNoteData(stChanNote *pNoteData, int EffColumns) override;
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleNote(int Note, int Octave) override;
	CString	GetCustomEffectString() const override;		// // //

	void	resetPhase();

	unsigned char m_iChannel;		// // //
	unsigned char m_cSweep;
	bool	m_bSweeping;
	int		m_iSweep;
	int		m_iLastPeriod;
};

// Triangle
class CTriangleChan : public CChannelHandler2A03 {
public:
	CTriangleChan();
	void	RefreshChannel() override;
	void	ResetChannel() override;		// // //
	int		GetChannelVolume() const override;		// // //
protected:
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	ClearRegisters() override;
	CString	GetCustomEffectString() const override;		// // //
private:
	int m_iLinearCounter;
};

// Noise
class CNoiseChan : public CChannelHandler2A03 {
public:
	void	RefreshChannel();
	int getDutyMax() const override;
protected:
	static const char MAX_DUTY;

	void	ClearRegisters() override;
	CString	GetCustomEffectString() const override;		// // //
	void	HandleNote(int Note, int Octave) override;
	void	SetupSlide() override;		// // //

	int		LimitPeriod(int Period) const override;		// // //
	int		LimitRawPeriod(int Period) const override;		// // //

	int		TriggerNote(int Note) override;
};

class CDSample;		// // //

// DPCM
class CDPCMChan : public CChannelHandler, public CChannelHandlerInterfaceDPCM {		// // //
public:
	CDPCMChan();		// // //
	void	RefreshChannel() override;
	int		GetChannelVolume() const override;		// // //

	void WriteDCOffset(unsigned char Delta);		// // //
	void SetLoopOffset(unsigned char Loop);		// // //
	void PlaySample(const CDSample *pSamp, int Pitch);		// // //
protected:
	void	HandleNoteData(stChanNote *pNoteData, int EffColumns) override;
	bool	HandleEffect(effect_t EffNum, unsigned char EffParam) override;		// // //
	void	HandleEmptyNote() override;
	void	HandleCut() override;
	void	HandleRelease() override;
	void	HandleNote(int Note, int Octave) override;
	bool	CreateInstHandler(inst_type_t Type) override;		// // //

	void	resetPhase();

	void triggerSample();
	void queueSample();

	void	ClearRegisters() override;
	CString	GetCustomEffectString() const override;		// // //
private:
	// DPCM variables
	unsigned char m_cDAC;
	unsigned char m_iLoop;
	unsigned char m_iOffset;
	unsigned char m_iSampleLength;
	unsigned char m_iLoopOffset;
	unsigned char m_iLoopLength;
	int mRetriggerPeriod;	// If zero, DPCM will not retrigger.
	int mRetriggerCtr;		// Time until next DPCM retrigger (frames)
	int m_iCustomPitch;
	bool mTriggerSample;		// // //
	bool mEnabled;
};
