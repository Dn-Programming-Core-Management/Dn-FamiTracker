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

static const int DUTY_2A03_FROM_VRC6[] = {0, 0, 1, 1, 1, 1, 2, 2};		// // //
static const int DUTY_VRC6_FROM_2A03[] = {1, 3, 7, 3};		// // //

class CInstHandler;
class stChannelState;
class CSoundGen;		// // //

#include "ChannelHandlerInterface.h"

/*!
	\brief An implementation of the channel handler.
*/
class CChannelHandler : public CChannelHandlerInterface {
protected:
	/*! \brief Constructor of the channel handler.
		\param MaxPeriod The maximum pitch register value that the channel handler can attain.
		\param MaxVolume The maximum instrument volume level that the channel handler can attain.
	*/
	CChannelHandler(int MaxPeriod, int MaxVolume);

public:
	/*! \brief Destructor of the channel handler. */
	virtual ~CChannelHandler();

	/*! \brief Plays a note from pattern data.
		\param NoteData A pointer to the note data.
		\param EffColumns The number of used effect columns of the note data.
	*/
	void	PlayNote(stChanNote *pNoteData, int EffColumns);

	// Public functions
	/*! \brief Initializes the channel handler and sets up member pointers.
		\param pAPU Pointer to the sound channel object.
		\param pVibTable Pointer to the vibrato lookup table.
		\param pSoundGen Pointer to the sound generator object.
	*/
	void	InitChannel(CAPU *pAPU, int *pVibTable, CSoundGen *pSoundGen);
	/*! \brief Called by the MIDI auto-arpeggio function to play a given note value.
		\param Note The note value.
	*/
	void	Arpeggiate(unsigned int Note);
	/*! \brief Forces the instrument handler to load an instrument for the next note.
		\details This method overrides the case where the same instrument is used to play
		successive notes.
	*/
	void	ForceReloadInstrument();		// // //

	/*! \brief Updates properties of the channel handler that are obtained from the current module file. */
	/*! \brief Chooses between linear pitch space and the default pitch space provided by the sound chip.
		\param bEnable Whether linear pitch space is used.
	*/
	void	SetLinearPitch(bool bEnable);		// // //
	/*! \brief Chooses between the new and old vibrato behaviour.
		\param vibrato_t The vibrato style.
	*/
	void	SetVibratoStyle(vibrato_t bEnable);		// // //

	//
	// Public virtual functions
	//
public:
	/*! \brief Runs the channel for one tick. */
	virtual void	ProcessChannel();							// // // no longer pure
	/*! \brief Updates the sound channel's registers according to the channel handler's state. */
	virtual void	RefreshChannel() = 0;
	/*! \brief Resets the channel handler's sound state to an initial state. */
	virtual void	ResetChannel();
	/*! \brief Retrieves the channel handler's state.
		\warning The output of this method is neither guaranteed nor required to match that of
		::GetStateString defined in SoundGen.cpp.
		\return A string representing the internal state of the channel handler.
	*/
	virtual CString	GetStateString();							// // //
	/*! \brief Applies a channel state to the channel handler.
		\param State Pointer to a channel state object.
		\sa CSoundGen::ApplyGlobalState
	*/
	virtual void	ApplyChannelState(stChannelState *State);	// // //

	/*! \brief Sets the channel handler's note lookup table.
		\param pNoteLookupTable Pointer to the note lookup table.
	*/
	virtual void	SetNoteTable(unsigned int *pNoteLookupTable);
	/*! \brief Sets the MIDI pitch wheel offset.
		\param Pitch The new offset value.
	*/
	virtual void	SetPitch(int Pitch);

	/*! \brief Sets the identifier of the channel.
		\param ID The new identifier value.
	*/
	virtual void	SetChannelID(int ID) { m_iChannelID = ID; }

	// 
	// Internal virtual functions
	//
protected:
	/*! \brief Resets the sound channel's registers to an initial state. */
	virtual void	ClearRegisters() = 0;
	/*! \brief Restricts the note value within the limits of the tracker, and notifies the tracker
		view of the note.
		\param Note Input note value.
		\return The pitch register value of the restricted note value.
	*/
	virtual	int		TriggerNote(int Note);
	
	/*! \brief Processes a note.
		\details This method is called both for both notes from pattern data and the delayed note cache.
		\param NoteData A pointer to the note data.
		\param EffColumns The number of used effect columns of the note data.
	*/
	virtual void	HandleNoteData(stChanNote *pNoteData, int EffColumns);
	/*! \brief Processes an instrument.
		\details This method sets up the instrument handler, creating a new one if necessary, then
		forwards calls to the handler if it exists.
		\param Instrument The instrument index.
		\param Trigger Whether the instrument handler needs to trigger the current instrument.
		\param NewInstrument Whether the instrument handler needs to load an instrument.
		\return Whether the instrument with the given index is loaded to the instrument handler.
	*/
	virtual bool	HandleInstrument(int Instrument, bool Trigger, bool NewInstrument);		// // // not pure virtual
	/*! \brief Processes an effect command.
		\details Implementations of this method in subclasses should use the return value of the
		superclass method to determine whether an effect requires handling. Global effects are not
		handled by this method, but by CSoundGen::EvaluateGlobalEffects.
		\param EffCmd The effect type to be processed.
		\param EffParam The effect command parameter.
		\return Whether the method has processed the effect of the given type.
	*/
	virtual bool	HandleEffect(effect_t EffNum, unsigned char EffParam);		// // // not pure virtual either
	/*! \brief Creates an instrument handler of an appropriate type.
		\return Whether an instrument handler is created.
	*/
	virtual bool	CreateInstHandler(inst_type_t Type);		// // //

	// Pure virtual functions for handling notes
	/*! \brief Processes a blank note from pattern data. */
	virtual void	HandleEmptyNote() = 0;
	/*! \brief Processes a cut event from pattern data. */
	virtual void	HandleCut() = 0;
	/*! \brief Processes a release event from pattern data. */
	virtual void	HandleRelease() = 0;
	/*! \brief Processes a note from pattern data.
		\details Echo buffer retrieval takes place before this method is called.
		\param Note The note pitch.
		\param Octave The note octave.
	*/
	virtual void	HandleNote(int Note, int Octave) = 0;

	/*! \brief Instantiates a pitch slide to a destination note. */
	virtual void	SetupSlide();		// // //

	/*! \brief Obtains the current pitch register of the sound channel.
		\details This method chooses the appropriate signs for the sources of pitch offset in the
		channel handler.
		\return The current pitch register, restricted within the range of the sound channel.
	*/
	virtual int		CalculatePeriod() const;
	/*! \brief Obtains the current volume register of the sound channel.
		\details This method depends on the configuration setting for optional behaviour during
		mixing the channel volume and the instrument volume.
		\param Subtract Whether the channel mixes the channel and instrument volumes by subtracting
		or multiplying register values. Subtraction is required for sound chips that produce
		exponential volume output; multiplication applies for linear volume output.
		\return The current volume register, restricted within the range of the sound channel.
	*/
	virtual int		CalculateVolume(bool Subtract = false) const;
	/*! \brief Restricts the pitch value within the limits of the sound channel.
		\param Period Input period or frequency register value.
		\return The restricted period or frequency register value.
	*/
	virtual int		LimitPeriod(int Period) const;
	
	/*! \brief Retrieves information about common effects of the channel handler.
		\return A string representing active effects and their parameters.
	*/
	virtual CString	GetEffectString() const;		// // //
	/*! \brief Retrieves information about slide effects of the channel handler.
		\details Depending on the internal representation of CChannelHandler::m_iPitch, this method
		may be overridden in subclasses to return the proper effect parameters in the string.
		\return A string representing active effects and their parameters.
	*/
	virtual CString	GetSlideEffectString() const;		// // //
	/*! \brief Retrieves information about effects specific to the sound channel of the channel handler.
		\return A string representing active effects and their parameters.
	*/
	virtual CString	GetCustomEffectString() const;		// // //

	// 
	// Internal non-virtual functions
	//
protected:
	/*! \brief Starts a new note.
		\details This method initiates a new pitch slide if the 3xx automatic portamento effect is
		enabled.
		\param Octave The note octave.
		\param Note The note pitch.
		\return The note value with the given pitch and octave.
	*/
	int		RunNote(int Octave, int Note);
	/*! \brief Halts the current active note. */
	void	CutNote();
	/*! \brief Releases the current active note.
		\details A note can only be released once until another new note is triggered.
	*/
	void	ReleaseNote();

	/*! \brief Notifies the tracker view that the current channel handler is playing a note.
		\param Note The note value, or -1 if no note is active.
	*/
	void	RegisterKeyState(int Note);
	
	/*! \brief Returns the pitch register offset of the channel's MIDI pitch wheel value.
		\details A positive value represents a lower pitch. The sign of the return value depends
		on whether the sound channel uses period or frequency registers.
		\return The pitch offset.
	*/
	int		GetPitch() const;
	
	/*! \brief Processes the Gxx delay effect in a given note.
		\details The method caches the note data at CChannelHandler::m_cnDelayed if a Gxx effect
		command is found. Jump effects are processed immediately and removed from the cached data.
		\param NoteData A pointer to the note data.
		\param EffColumns The number of used effect columns of the note data.
		\return Whether the note data contains a Gxx effect command.
	*/
	bool	HandleDelay(stChanNote *NoteData, int EffColumns);
	
	/*! \brief Returns the pitch register offset of the channel's 4xy vibrato effect.
		\details A positive value represents a higher pitch. The sign of the return value depends
		on whether the sound channel uses period or frequency registers.
		\return The pitch offset.
	*/
	int		GetVibrato() const;
	/*! \brief Returns the volume register offset of the channel's 7xy tremolo effect.
		\details A positive value represents a decrease in volume.
		\return The volume offset.
	*/
	int		GetTremolo() const;
	/*! \brief Returns the pitch register offset of the channel's Pxx fine pitch effect.
		\details A positive value represents a lower pitch. The sign of the return value depends
		on whether the sound channel uses period or frequency registers.
		\return The pitch offset.
	*/
	int		GetFinePitch() const;

	/*! \brief Pads CPU cycles before the next channel handler's changes to the sound registers
		are reflected.
		\param count The number of CPU cycles.
	*/
	void	AddCycles(int count);

	/*! \brief Increments the channel handler's pitch register value.
		\details This method directly adds \a Step to the pitch register value, or calls
		CChannelHandler::LinearAdd if linear pitch slides are enabled.
		\param Step The number of increments.
	*/
	void	PeriodAdd(int Step);
	/*! \brief Decrements the channel handler's pitch register value.
		\details This method directly subtracts \a Step from the pitch register value, or calls
		CChannelHandler::LinearRemove if linear pitch slides are enabled.
		\param Step The number of decrements.
	*/
	void	PeriodRemove(int Step);

	/*! \brief Increments the channel handler's pitch register value proportionately.
		\param Step The number of increments. One step corresponds to 1/512 of the current pitch
		register value.
	*/
	void	LinearAdd(int Step);
	/*! \brief Decrements the channel handler's pitch register value proportionately.
		\param Step The number of decrements. One step corresponds to 1/512 of the current pitch
		register value.
	*/
	void	LinearRemove(int Step);

	/*! \brief Pushes a note into the channel handler's echo buffer.
		\details Transposing effects in the note data are resolved immediately.
		\param NoteData A pointer to the note data.
		\param Pos The index of the echo buffer at which the note will be inserted.
		\param EffColumns The number of used effect columns of the note data.
	*/
	void	WriteEchoBuffer(stChanNote *NoteData, int Pos, int EffColumns);		// // //

	/*! \brief Writes to the internal APU.
		\param Reg The register port.
		\param Value The value to be written.
	*/
	void	WriteRegister(uint16 Reg, uint8 Value);
	/*! \brief Writes to an external sound chip.
		\param Reg The register port.
		\param Value The value to be written.
	*/
	void	WriteExternalRegister(uint16 Reg, uint8 Value);
	
	/*! \brief Converts a duty value from the current instrument into an equivalent value for the
		current sound channel.
		\details This method is only called from an instrument through the channel handler's
		interface, and does not affect the Vxx duty cycle effect.
		\param Duty Input duty value from the instrument.
		\return The converted duty value, or -1 if no sensible value exists.
		\sa CChannelHandler::SetDutyPeriod
	*/
	virtual int ConvertDuty(int Duty) const { return Duty; };		// // //

public:		// // //
	/*! \brief Sets the current pitch register of the channel.
		\warning This method overrides the current pitch register value of the channel. The channel
		handler currently has no way to allow changes due to CChannelHandler::m_iEffect and its
		interface orthogonally.
		\param Period The period or frequency register.
	*/
	void	SetPeriod(int Period);
	/*! \brief Obtains the current pitch register of the channel.
		\details This includes pitch changes due to slide effects and CChannelHandler::SetNote.
		\return The pitch register value.
	*/
	int		GetPeriod() const;
	/*! \brief Sets the current note value of the channel.
		\warning This method overrides the current note value of the channel, and hence effect commands
		that depend on changing the note value.
		\param Note The absolute note value.
	*/
	void	SetNote(int Note);
	/*! \brief Obtains the current note value of the channel.
		\details This includes pitch changes due to transposing effects and the instrument handler.
		\return The note value.
	*/
	int		GetNote() const;
	/*! \brief Sets the current instrument volume of the channel.
		\details The channel interface never controls the channel volume.
		\param Volume The instrument volume level.
	*/
	void	SetVolume(int Volume);
	/*! \brief Obtains the current instrument volume of the channel.
		\return The instrument volume level.
	*/
	int		GetVolume() const;
	/*! \brief Sets the current duty cycle value of the channel.
		\details The value received by the channel is converted according to the current instrument type.
		\param Duty The duty cycle value.
	*/
	void	SetDutyPeriod(int Duty);
	/*! \brief Obtains the current duty cycle value of the channel.
		\return The duty cycle value.
	*/
	int		GetDutyPeriod() const;
	unsigned char GetArpParam() const;		// // //
	bool	IsActive() const;
	bool	IsReleasing() const;

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
	/*! \brief The maximum number of semitones deviating from the normal note value due to the MIDI
		pitch wheel. */
	static const int PITCH_WHEEL_RANGE = 6;
	/*! \brief The number of bitwise shifts required to convert the channel volume from a note into
		the channel volume for the channel handler. */
	static const int VOL_COLUMN_SHIFT = 3;
	/*! \brief The maximum channel volume that a channel handler can attain. */
	static const int VOL_COLUMN_MAX = 0x7F;

	// Shared variables
protected:
	// Channel variables
	/*! \brief The channel identifier.
		\details This value should always be a member of chan_id_t.
	*/
	int				m_iChannelID;

	// General
	/*! \brief A flag indicating that the current active note of the channel handler has been released. */
	bool			m_bRelease;
	/*! \brief A flag indicating that the channel handler runs an active note. */
	bool			m_bGate;

	/*! \brief The current instrument index of the channel. */
	unsigned int	m_iInstrument;
	/*! \brief A flag indicating that the current instrument should be reloaded by the instrument handler. */
	bool			m_bForceReload;					// // //

	/*! \brief The current note value of the channel.
		\details Its value may be altered by transposing effects and the instrument handler.
	*/
	int				m_iNote;
	/*! \brief The current pitch register value of the channel.
		\details It may represent a period register or a frequency register, depending on the sound
		channel used. It may also be an internal representation that does not have the same resolution
		as the actual register of the sound channel.
	*/
	int				m_iPeriod;
	/*! \brief The current instrument volume of the channel.
		\details The instrument volume is limited by the maximum volume level provided in the constructor.
	*/
	int				m_iInstVolume;					// // //
	/*! \brief The current channel volume of the channel.
		\details The channel volume is always limited by CChannelHandler::VOL_COLUMN_MAX rather than
		the maximum volume level provided in the constructor.
	*/
	int				m_iVolume;

	/*! \brief The current duty cycle value of the channel.
		\details Its exact interpretation differs across sound chips; in particular, sound channels
		supporting wave tables may treat this member as a table index.
		\warning Derived classes should be expected to handle the case where this value equals -1
		if CChannelHandler::ConvertDuty returns no sensible value for the current instrument type.
	*/
	char			m_iDutyPeriod;
	/*! \brief A queue of the most recent notes triggered by the channel.
		\details In order to represent the blank note and the note cut, the special constants
		ECHO_BUFFER_NONE and ECHO_BUFFER_HALT are defined for use with this echo buffer.
	*/
	int				m_iEchoBuffer[ECHO_BUFFER_LENGTH + 1];		// // //

	/*! \brief A sub-integer value used by linear pitch slides for improved resolution.
		\sa CChannelHandler::LinearAdd
		\sa CChannelHandler::LinearRemove
	*/
	int				m_iPeriodPart;

	/*! \brief A flag indicating the direction of the 4xy vibrato effect. */
	bool			m_bNewVibratoMode;
	/*! \brief A flag indicating that pitch bends are proportional to the current pitch register. */
	bool			m_bLinearPitch;

	// Delay effect variables
	/*! \brief A flag indicating that a note has been delayed by a Gxx effect command. */
	bool			m_bDelayEnabled;
	/*! \brief The number of ticks until the tick where the note delayed by a Gxx effect command will be played. */
	unsigned char	m_cDelayCounter;
	/*! \brief The number of used effect columns of the delayed note cache. */
	unsigned int	m_iDelayEffColumns;		
	/*! \brief A note structure holding a temporary cache of the note data delayed by a Gxx effect command. */
	stChanNote		m_cnDelayed;

	// Vibrato & tremolo
	/*! \brief The current extent of the 4xy vibrato effect.
		\details In accordance with the NSF driver, this member always occupies bits 4 - 7.
	*/
	unsigned int	m_iVibratoDepth;
	/*! \brief The current rate of the 4xy vibrato effect.
		\details The vibrato phase advances by this amount on each tick.
	*/
	unsigned int	m_iVibratoSpeed;
	/*! \brief The current phase of the 4xy vibrato effect.
		\details One full cycle of a vibrato effect contains exactly 64 phases.
	*/
	unsigned int	m_iVibratoPhase;
	
	/*! \brief The current extent of the 7xy tremolo effect.
		\details In accordance with the NSF driver, this member always occupies bits 4 - 7.
	*/
	unsigned int	m_iTremoloDepth;
	/*! \brief The current rate of the 7xy tremolo effect.
		\details The tremolo phase advances by this amount on each tick.
	*/
	unsigned int	m_iTremoloSpeed;
	/*! \brief The current phase of the 7xy tremolo effect.
		\details One full cycle of a tremolo effect contains exactly 64 phases.
	*/
	unsigned int	m_iTremoloPhase;

	/*! \brief The currently active slide effect. */
	unsigned char	m_iEffect;
	/*! \brief The effect command parameter for the active slide effect.
		\details This member is used by the instrument interface to handle arpeggio schemes.
	*/
	unsigned char	m_iEffectParam;					// // //
	/*! \brief The current state of the 0xy arpeggio effect.
		\details Each state corresponds to a different note offset. A 0xy arpeggio cycle may
		contain 2 or 3 states depending on the current effect parameter.
	*/
	unsigned char	m_iArpState;
	/*! \brief The target pitch register value if an automatic pitch slide is taking place. */
	int				m_iPortaTo;
	/*! \brief The speed of the current automatic pitch slide.
		\details The channel alters its pitch register by a value whose magnitude is no greater
		than this member value.
	*/
	int				m_iPortaSpeed;

	/*! \brief The number of ticks up to and including the tick where the Sxx delayed note cut effect
		would occur.
		\details Remains at 0 if no such effect is issued.
	*/
	unsigned char	m_iNoteCut;
	/*! \brief The number of ticks up to and including the tick where the Lxx delayed note release
		effect would occur.
		\details Remains at 0 if no such effect is issued.
	*/
	unsigned char	m_iNoteRelease;					// // //
	/*! \brief The number of ticks until the tick where the Mxy delayed channel volume effect would occur.
		\details When its value is equal to 0, the Mxy effect would take place on the current tick.
		Remains at -1 (255) if no such effect is issued.
	*/
	char			m_iNoteVolume;					// // //
	/*! \brief A cache of the channel volume when a Mxy effect command is issued. */
	char			m_iDefaultVolume;				// // //
	/*! \brief The target channel volume of the last issued Mxy effect command. */
	unsigned char	m_iNewVolume;					// // //
	/*! \brief The number of ticks up to and including the tick where the Txy delayed note transpose
		effect would occur.
		\details Remains at 0 if no such effect is issued.
	*/
	unsigned char	m_iTranspose;					// // //
	/*! \brief A flag indicating that the last issued Txy effect command transposes downward instead
		of upward. */
	bool			m_bTransposeDown;				// // //
	/*! \brief The number of notes to transpose in a Txy delayed note transpose effect. */
	char			m_iTransposeTarget;				// // //
	/*! \brief The effect command parameter of the Pxx fine pitch effect. */
	unsigned int	m_iFinePitch;
	/*! \brief The effect command parameter of the Vxx duty cycle effect. */
	unsigned char	m_iDefaultDuty;
	/*! \brief The effect command parameter of the Axy volume slide effect. */
	unsigned char	m_iVolSlide;

	// Misc
	/*! \brief A pointer to the underlying sound channel controller object. */
	CAPU			*m_pAPU;
	/*! \brief A pointer to the sound generator object. */
	CSoundGen		*m_pSoundGen;

	/*! \brief A pointer to the channel's note lookup table.
		\details The lookup table contains either period or frequency register values according to
		the sound channel. Except for the Konami VRC7, which only requires register values for a
		single octave, all other lookup tables should contain at least as many entries as the number
		of notes available in the tracker.
	*/
	unsigned int	*m_pNoteLookupTable;
	/*! \brief A pointer to the channel's vibrato lookup table.
		\details A vibrato lookup table contains as many rows as the number of vibrato depths
		available, each row containing the first quarter of the vibrato amplitude values; values for
		other 4xy vibrato effect phases are calculated within the channel handler. The 7xy tremolo
		effect shares the same lookup table.
	*/
	int				*m_pVibratoTable;

	/*! \brief The MIDI pitch wheel offset of the current channel.
		\details A positive value represents a lower pitch. The value of this member is limited
		within [-512, 511].
	*/
	int				m_iPitch;
	
	/*! \brief The instrument type of the previously loaded instrument.
		\details The channel handler uses this value to determine different actions for supporting
		instruments not native to the current sound channel.
		\sa CChannelHandler::ConvertDuty
	*/
	inst_type_t		m_iInstTypeCurrent;
	/*! \brief A pointer to the currently installed instrument handler. */
	CInstHandler	*m_pInstHandler;				// // //

	// Private variables
private:
	int				m_iMaxPeriod;
	int				m_iMaxVolume;
};

// Channel handler for channels with frequency registers
class CChannelHandlerInverted : public CChannelHandler {
protected:
	CChannelHandlerInverted(int MaxPeriod, int MaxVolume) : CChannelHandler(MaxPeriod, MaxVolume) {}
	// // //
	virtual bool	HandleEffect(effect_t EffNum, unsigned char EffParam);		// // //
	virtual int		CalculatePeriod() const;
	virtual CString	GetSlideEffectString() const;		// // //
};

