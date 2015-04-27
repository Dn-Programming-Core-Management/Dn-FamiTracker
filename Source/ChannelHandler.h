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

class CAPU;

// Sequence states
enum seq_state_t {
	SEQ_STATE_DISABLED,
	SEQ_STATE_RUNNING,
	SEQ_STATE_END,
	SEQ_STATE_HALT
};

//
// Sequence handler class
//
class CSequenceHandler {
protected:
	CSequenceHandler();

	// Virtual methods
	virtual	int  TriggerNote(int Note) = 0;
	virtual void SetVolume(int Volume) = 0;
	virtual void SetPeriod(int Period) = 0;
	virtual int  GetPeriod() const = 0;
	virtual void SetNote(int Note) = 0;
	virtual int  GetNote() const = 0;
	virtual void SetDutyPeriod(int Period) = 0;
	virtual bool IsActive() const = 0;
	virtual bool IsReleasing() const = 0;

	// Sequence functions
	void SetupSequence(int Index, const CSequence *pSequence);
	void ClearSequence(int Index);
	void RunSequence(int Index);
	void ClearSequences();
	void ReleaseSequences();
	bool IsSequenceEqual(int Index, const CSequence *pSequence) const;
	seq_state_t GetSequenceState(int Index) const;

	unsigned char	m_iArpeggio;		// // // for arp schemes
	// 0CC: hack

private:
	void UpdateSequenceRunning(int Index, const CSequence *pSequence);
	void UpdateSequenceEnd(int Index, const CSequence *pSequence);
	void ReleaseSequence(int Index, const CSequence *pSeq);

	// Sequence variables
private:
	const CSequence	*m_pSequence[SEQ_COUNT];
	seq_state_t		m_iSeqState[SEQ_COUNT];
	int				m_iSeqPointer[SEQ_COUNT];
};

//
// Base class for channel renderers
//
class CChannelHandler : public CSequenceHandler {
protected:
	CChannelHandler(int MaxPeriod, int MaxVolume);

public:
	virtual ~CChannelHandler();

	void	PlayNote(stChanNote *pNoteData, int EffColumns);	// Plays a note, calls the derived classes

	// Public functions
	void	InitChannel(CAPU *pAPU, int *pVibTable, CSoundGen *pSoundGen);
	void	Arpeggiate(unsigned int Note);

	void	DocumentPropertiesChanged(CFamiTrackerDoc *pDoc);

	//
	// Public virtual functions
	//
public:
	virtual void	ProcessChannel() = 0;						// Run the instrument and effects
	virtual void	RefreshChannel() = 0;						// Update channel registers
	virtual void	ResetChannel();								// Resets all state variables to default
	virtual void	RetrieveChannelState(CString *log);			// // // Retrieve current channel state from previous frames

	virtual void	SetNoteTable(unsigned int *pNoteLookupTable);
	virtual void	UpdateSequencePlayPos() {};
	virtual void	SetPitch(int Pitch);

	virtual void	SetChannelID(int ID) { m_iChannelID = ID; }

	// 
	// Internal virtual functions
	//
protected:
	virtual void	ClearRegisters() = 0;						// Clear channel registers
	virtual	int		TriggerNote(int Note);

	virtual void	HandleNoteData(stChanNote *pNoteData, int EffColumns);

	// Pure virtual functions for handling notes
	virtual void	HandleCustomEffects(int EffNum, int EffParam) = 0;
	virtual bool	HandleInstrument(int Instrument, bool Trigger, bool NewInstrument) = 0;
	virtual void	HandleEmptyNote() = 0;
	virtual void	HandleCut() = 0;
	virtual void	HandleRelease() = 0;
	virtual void	HandleNote(int Note, int Octave) = 0;

	virtual void	SetupSlide();		// // //

	virtual int		CalculatePeriod() const;
	virtual int		CalculateVolume(bool Subtract = false) const;

	virtual CString	GetCustomEffectString() const;		// // //

	// 
	// Internal non-virtual functions
	//
protected:
	void	CutNote();											// Called on note cut commands
	void	ReleaseNote();										// Called on note release commands

	int		LimitPeriod(int Period) const;
	int		LimitVolume(int Volume) const;

	void	RegisterKeyState(int Note);

	int		RunNote(int Octave, int Note);
	int		GetPitch() const;

	bool	CheckCommonEffects(unsigned char EffCmd, unsigned char EffParam);
	bool	HandleDelay(stChanNote *NoteData, int EffColumns);

	int		GetVibrato() const;
	int		GetTremolo() const;
	int		GetFinePitch() const;

	void	AddCycles(int count);

	void	PeriodAdd(int Step);
	void	PeriodRemove(int Step);

	void	LinearAdd(int Step);
	void	LinearRemove(int Step);

	bool	IsActive() const;
	bool	IsReleasing() const;

	void	WriteEchoBuffer(stChanNote *NoteData, int Pos, int EffColumns);		// // //

	void	WriteRegister(uint16 Reg, uint8 Value);
	void	WriteExternalRegister(uint16 Reg, uint8 Value);
	CString	GetEffectString() const;		// // //

	// CSequenceHandler virtual methods
protected:
	void	SetVolume(int Volume);
	void	SetPeriod(int Period);
	int		GetPeriod() const;
	void	SetNote(int Note);
	int		GetNote() const;
	void	SetDutyPeriod(int Period);

private:
	void	UpdateNoteCut();
	void	UpdateNoteVolume();		// // //
	void	UpdateTranspose();		// // //
	virtual void UpdateNoteRelease();		// // // for VRC7
	void	UpdateDelay();
	void	UpdateVolumeSlide();
	void	UpdateTargetVolumeSlide();
	void	UpdateVibratoTremolo();
	void	UpdateEffects();

public:
	// Range for the pitch wheel command (in semitones)
	static const int PITCH_WHEEL_RANGE = 6;

	static const int VOL_COLUMN_SHIFT = 3;
	static const int VOL_COLUMN_MAX = 0x7F;

	// Shared variables
protected:
	// Channel variables
	int				m_iChannelID;					// Channel ID

	// General
	bool			m_bRelease;						// Note released flag
	bool			m_bGate;						// Note gate flag

	unsigned int	m_iInstrument;					// Instrument
	unsigned int	m_iLastInstrument;				// Previous instrument

	int				m_iNote;						// Active note
	int				m_iPeriod;						// Channel period/frequency
	int				m_iLastPeriod;					// Previous period
	int				m_iSeqVolume;					// Sequence volume
	int				m_iVolume;						// Volume
	char			m_iDefaultVolume;				// // // for the delayed volume
	char			m_iDutyPeriod;
	int				m_iEchoBuffer[ECHO_BUFFER_LENGTH + 1];		// // // Echo buffer

	int				m_iPeriodPart;					// Used by linear slides

	bool			m_bNewVibratoMode;
	bool			m_bLinearPitch;

	bool			m_bPeriodUpdated;				// Flag for detecting new period value
	bool			m_bVolumeUpdate;				// Flag for detecting new volume value (currently unused)

	// Delay effect variables
	bool			m_bDelayEnabled;
	unsigned char	m_cDelayCounter;
	unsigned int	m_iDelayEffColumns;		
	stChanNote		m_cnDelayed;

	// Vibrato & tremolo
	unsigned int	m_iVibratoDepth;
	unsigned int	m_iVibratoSpeed;
	unsigned int	m_iVibratoPhase;

	unsigned int	m_iTremoloDepth;
	unsigned int	m_iTremoloSpeed;
	unsigned int	m_iTremoloPhase;

	unsigned char	m_iEffect;						// arpeggio & portamento
	unsigned char	m_iEffectParam;					// // // single effect parameter as in nsf driver
													// 0CC: replace m_iArpeggio in CSequenceHandler with this
	unsigned char	m_iArpState;
	int				m_iPortaTo;
	int				m_iPortaSpeed;

	unsigned char	m_iNoteCut;						// Note cut effect
	unsigned char	m_iNoteRelease;					// // // Note release effect
	char			m_iNoteVolume;					// // // Delayed channel volume effect
	unsigned char	m_iNewVolume;					// // //
	unsigned char	m_iTranspose;					// // // Delayed transpose counter
	char			m_iTransposeTarget;				// // // transpose
	unsigned int	m_iFinePitch;					// Fine pitch effect
	unsigned char	m_iDefaultDuty;					// Duty effect
	unsigned char	m_iVolSlide;					// Volume slide effect

	// Misc 
	CAPU			*m_pAPU;
	CSoundGen		*m_pSoundGen;

	unsigned int	*m_pNoteLookupTable;			// Note->period table
	int				*m_pVibratoTable;				// Vibrato table

	int				m_iPitch;						// Used by the pitch wheel

	// Private variables
private:
	int				m_iMaxPeriod;					// Period register limit
	int				m_iMaxVolume;					// Max channel volume

};

// Channel handler for channels with frequency registers
class CChannelHandlerInverted : public CChannelHandler {
protected:
	CChannelHandlerInverted(int MaxPeriod, int MaxVolume) : CChannelHandler(MaxPeriod, MaxVolume) {}
	// // //
	virtual int CalculatePeriod() const;
};
