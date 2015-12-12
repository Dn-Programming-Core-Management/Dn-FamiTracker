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

#define CREATE_INST_HANDLER(T, ...) { if (typeid(m_pInstHandler) != typeid(T)) \
                                         SAFE_RELEASE(m_pInstHandler); \
                                    if (dynamic_cast<T*>(m_pInstHandler) == nullptr) \
                                         m_pInstHandler = new T(m_pInstInterface, __VA_ARGS__); }		// // //

class CAPU;

// Sequence states
enum seq_state_t {
	SEQ_STATE_DISABLED,
	SEQ_STATE_RUNNING,
	SEQ_STATE_END,
	SEQ_STATE_HALT
};

static const int DUTY_2A03_FROM_VRC6[] = {0, 0, 1, 1, 1, 1, 2, 2};		// // //
static const int DUTY_VRC6_FROM_2A03[] = {1, 3, 7, 3};		// // //

class CInstHandler;
class CChannelInterface;

//
// Base class for channel renderers
//
class CChannelHandler {
protected:
	CChannelHandler(int MaxPeriod, int MaxVolume);

public:
	virtual ~CChannelHandler();

	void	PlayNote(stChanNote *pNoteData, int EffColumns);	// Plays a note, calls the derived classes

	// Public functions
	void	InitChannel(CAPU *pAPU, int *pVibTable, CSoundGen *pSoundGen);
	void	Arpeggiate(unsigned int Note);
	void	ForceReloadInstrument();		// // //

	void	DocumentPropertiesChanged(CFamiTrackerDoc *pDoc);

	//
	// Public virtual functions
	//
public:
	virtual void	ProcessChannel();							// Run the instrument and effects // // // no longer pure
	virtual void	RefreshChannel() = 0;						// Update channel registers
	virtual void	ResetChannel();								// Resets all state variables to default
	virtual CString	GetStateString();							// // // Retrieve current channel state
	virtual void	ApplyChannelState(stChannelState *State);	// // //

	virtual void	SetNoteTable(unsigned int *pNoteLookupTable);
	virtual void	UpdateSequencePlayPos() {};
	virtual void	SetPitch(int Pitch);

	virtual void	SetChannelID(int ID) { m_iChannelID = ID; }

	unsigned char	GetEffectParam() const;						// // //

	// 
	// Internal virtual functions
	//
protected:
	virtual void	ClearRegisters() = 0;						// Clear channel registers
	virtual	int		TriggerNote(int Note);

	virtual void	HandleNoteData(stChanNote *pNoteData, int EffColumns);
	virtual bool	HandleInstrument(int Instrument, bool Trigger, bool NewInstrument);		// // // not pure virtual
	virtual bool	CreateInstHandler(inst_type_t Type);		// // //

	// Pure virtual functions for handling notes
	virtual void	HandleCustomEffects(effect_t EffNum, int EffParam) = 0;
	virtual void	HandleEmptyNote() = 0;
	virtual void	HandleCut() = 0;
	virtual void	HandleRelease() = 0;
	virtual void	HandleNote(int Note, int Octave) = 0;

	virtual void	SetupSlide();		// // //

	virtual int		CalculatePeriod() const;
	virtual int		CalculateVolume(bool Subtract = false) const;
	
	virtual CString	GetEffectString() const;		// // //
	virtual CString	GetSlideEffectString() const;		// // //
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

	bool	CheckCommonEffects(effect_t EffCmd, unsigned char EffParam);
	bool	HandleDelay(stChanNote *NoteData, int EffColumns);

	int		GetVibrato() const;
	int		GetTremolo() const;
	int		GetFinePitch() const;

	void	AddCycles(int count);

	void	PeriodAdd(int Step);
	void	PeriodRemove(int Step);

	void	LinearAdd(int Step);
	void	LinearRemove(int Step);

	void	WriteEchoBuffer(stChanNote *NoteData, int Pos, int EffColumns);		// // //

	void	WriteRegister(uint16 Reg, uint8 Value);
	void	WriteExternalRegister(uint16 Reg, uint8 Value);
	
	virtual int ConvertDuty(int Duty) const { return Duty; };		// // //

	// CSequenceHandler virtual methods
protected:
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
	bool			m_bForceReload;					// // //

	int				m_iNote;						// Active note
	int				m_iPeriod;						// Channel period/frequency
	int				m_iLastPeriod;					// Previous period
	int				m_iInstVolume;					// Sequence volume
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
	unsigned char	m_iArpState;
	int				m_iPortaTo;
	int				m_iPortaSpeed;

	unsigned char	m_iNoteCut;						// Note cut effect
	unsigned char	m_iNoteRelease;					// // // Note release effect
	char			m_iNoteVolume;					// // // Delayed channel volume effect
	unsigned char	m_iNewVolume;					// // //
	unsigned char	m_iTranspose;					// // // Delayed transpose effect
	bool			m_bTransposeDown;				// // //
	char			m_iTransposeTarget;				// // //
	unsigned int	m_iFinePitch;					// Fine pitch effect
	unsigned char	m_iDefaultDuty;					// Duty effect
	unsigned char	m_iVolSlide;					// Volume slide effect

	// Misc 
	CAPU			*m_pAPU;
	CSoundGen		*m_pSoundGen;

	unsigned int	*m_pNoteLookupTable;			// Note->period table
	int				*m_pVibratoTable;				// Vibrato table

	int				m_iPitch;						// Used by the pitch wheel
	
	inst_type_t		m_iInstTypeCurrent;				// // // Used for duty conversions
	CInstHandler	*m_pInstHandler;				// // //
	friend			CChannelInterface;			// // //
	CChannelInterface *m_pInstInterface;

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
	virtual CString GetSlideEffectString() const;		// // //
};

//
// Base class for channel interface
//
// move to separate file?
class CChannelInterface		// // //
{
public:
	CChannelInterface(CChannelHandler *pChan) : m_pChannel(pChan) {}
	CChannelInterface() : m_pChannel(nullptr) {}
	virtual ~CChannelInterface() {};

	inline int TriggerNote(int Note) { return m_pChannel->TriggerNote(Note); }

	inline void SetVolume(int Volume) { m_pChannel->m_iInstVolume = Volume; }
	inline void SetPeriod(int Period) { m_pChannel->SetPeriod(Period); }
	inline void SetNote(int Note) { m_pChannel->SetNote(Note); }
	inline void SetDutyPeriod(int Duty) { m_pChannel->m_iDutyPeriod = m_pChannel->ConvertDuty(Duty); }

	inline int GetVolume() const { return m_pChannel->m_iInstVolume; }
	inline int GetPeriod() const { return m_pChannel->GetPeriod(); }
	inline int GetNote() const { return m_pChannel->GetNote(); }
	inline int GetDutyPeriod() const { return m_pChannel->m_iDutyPeriod; } // getter?

	inline unsigned char GetArpParam() const { return m_pChannel->m_iEffect == EF_ARPEGGIO ? m_pChannel->m_iEffectParam : 0U; }
	
	inline bool IsActive() const { return m_pChannel->m_bGate; }
	inline bool IsReleasing() const { return m_pChannel->m_bRelease; }

private:
	CChannelHandler *const m_pChannel;
};
