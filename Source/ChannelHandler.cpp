/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

//
// This is the base class for all classes that takes care of 
// translating notes to channel register writes.
//

#include "ChannelHandler.h"
#include "SongState.h"		// // //
#include "FTMComponentInterface.h"
#include "InstrumentManager.h"
#include "TrackerChannel.h"		// // //
#include "APU/Types.h"		// // //
#include "stdafx.h"
#include "SoundGen.h"
#include "FamiTracker.h"
#include "Settings.h"		// // //
#include "APU/APU.h"
#include "InstHandler.h"		// // //

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#include <algorithm>		// // //

/*
 * Class CChannelHandler
 *
 */

CChannelHandler::CChannelHandler(int MaxPeriod, int MaxVolume) : 
	m_iChannelID(0), 
	m_iInstTypeCurrent(INST_NONE),		// // //
	m_iInstrument(0),
	m_pNoteLookupTable(NULL),
	m_pVibratoTable(NULL),
	m_pAPU(NULL),
	m_pInstHandler(),		// // //
	m_iPitch(0),
	m_iNote(0),
	m_iInstVolume(0),
	m_iDefaultDuty(0),
	m_iDutyPeriod(0),
	m_iMaxPeriod(MaxPeriod),
	m_iMaxVolume(MaxVolume),
	m_bGate(false),
	m_bNewVibratoMode(false),
	m_bLinearPitch(false),
	m_bForceReload(false),		// // //
	m_iEffectParam(0)		// // //
{
}

CChannelHandler::~CChannelHandler()
{
}

void CChannelHandler::InitChannel(CAPU *pAPU, int *pVibTable, CSoundGen *pSoundGen)
{
	// Called from main thread

	m_pAPU = pAPU;
	m_pVibratoTable = pVibTable;
	m_pSoundGen = pSoundGen;

	m_bDelayEnabled = false;
}

void CChannelHandler::SetLinearPitch(bool bEnable)		// // //
{
	m_bLinearPitch = bEnable;
}

void CChannelHandler::SetVibratoStyle(vibrato_t Style)		// // //
{
	m_bNewVibratoMode = Style == VIBRATO_NEW;
}

void CChannelHandler::SetPitch(int Pitch)
{
	// Pitch ranges from -511 to +512
	m_iPitch = Pitch;
	if (m_iPitch == 512)
		m_iPitch = 511;
}

int CChannelHandler::GetPitch() const 
{ 
	if (m_iPitch != 0 && m_iNote != 0 && m_pNoteLookupTable != NULL) {
		// Interpolate pitch
		int LowNote  = std::max(m_iNote - PITCH_WHEEL_RANGE, 0);
		int HighNote = std::min(m_iNote + PITCH_WHEEL_RANGE, 95);
		int Freq	 = m_pNoteLookupTable[m_iNote];
		int Lower	 = m_pNoteLookupTable[LowNote];
		int Higher	 = m_pNoteLookupTable[HighNote];
		int Pitch	 = (m_iPitch < 0) ? (Freq - Lower) : (Higher - Freq);
		return (Pitch * m_iPitch) / 511;
	}

	return 0;
}

void CChannelHandler::Arpeggiate(unsigned int Note)
{
	SetPeriod(TriggerNote(Note));
}

void CChannelHandler::ForceReloadInstrument()		// // //
{
	m_bForceReload = true;
}

void CChannelHandler::ResetChannel()
{
	// Resets the channel states (volume, instrument & duty)
	// Clears channel registers

	// Instrument 
	m_iInstrument		= MAX_INSTRUMENTS;
	m_iInstTypeCurrent	= INST_NONE;		// // //
	m_pInstHandler.reset();		// // //

	// Volume 
	m_iVolume			= VOL_COLUMN_MAX;
	m_iDefaultVolume	= (VOL_COLUMN_MAX >> VOL_COLUMN_SHIFT) << VOL_COLUMN_SHIFT;		// // //

	m_iDefaultDuty		= 0;
	m_iDutyPeriod		= 0;		// // //
	m_iInstVolume		= 0;

	// Period
	m_iNote				= 0;		// // //
	m_iPeriod			= 0;

	// Effect states
	m_iEffect			= EF_NONE;		// // //
	m_iEffectParam		= 0;		// // //

	m_iPortaSpeed		= 0;
	m_iPortaTo			= 0;
	m_iArpState			= 0;
	m_iVibratoSpeed		= 0;
	m_iVibratoPhase		= !m_bNewVibratoMode ? 48 : 0;
	m_iTremoloSpeed		= 0;
	m_iTremoloPhase		= 0;
	m_iFinePitch		= 0x80;
	m_iPeriod			= 0;
	m_iVolSlide			= 0;
	m_bDelayEnabled		= false;
	m_iNoteCut			= 0;
	m_iNoteRelease		= 0;		// // //
	m_iNoteVolume		= -1;		// // //
	m_iNewVolume		= m_iDefaultVolume;		// // //
	m_iTranspose		= 0;
	m_bTransposeDown	= false;
	m_iTransposeTarget	= 0;
	m_iVibratoDepth		= 0;
	m_iTremoloDepth		= 0;

	for (int i = 0; i <= ECHO_BUFFER_LENGTH; i++)		// // //
		m_iEchoBuffer[i] = ECHO_BUFFER_NONE;

	// States
	m_bTrigger			= false;		// // //
	m_bRelease			= false;
	m_bGate				= false;

	RegisterKeyState(-1);

	// Clear channel registers
	ClearRegisters();
}

std::string CChannelHandler::GetStateString() const		// // //
{
	std::string log("Inst.: ");
	if (m_iInstrument == MAX_INSTRUMENTS) // never happens because famitracker will switch to selected inst
		log += "None";
	else
		log += {hex(m_iInstrument >> 4), hex(m_iInstrument)};
	log += "        Vol.: ";
	log += hex(m_iDefaultVolume >> VOL_COLUMN_SHIFT);
	log += "        Active effects:";
	return log + GetEffectString();
}

void CChannelHandler::ApplyChannelState(const stChannelState &State)
{
	m_iInstrument = State.Instrument;
	m_iDefaultVolume = m_iVolume = (State.Volume == MAX_VOLUME) ? VOL_COLUMN_MAX : (State.Volume << VOL_COLUMN_SHIFT);
	memcpy(m_iEchoBuffer, State.Echo, sizeof(int) * (ECHO_BUFFER_LENGTH + 1));
	if (m_iInstrument != MAX_INSTRUMENTS)
		HandleInstrument(true, true);
	if (State.Effect_LengthCounter >= 0)
		HandleEffect(EF_VOLUME, State.Effect_LengthCounter);
	for (unsigned int i = 0; i < EF_COUNT; i++)
		if (State.Effect[i] >= 0)
			HandleEffect(static_cast<effect_t>(i), State.Effect[i]);
	if (State.Effect[EF_FDS_MOD_SPEED_HI] >= 0x10)
		HandleEffect(EF_FDS_MOD_SPEED_HI, State.Effect[EF_FDS_MOD_SPEED_HI]);
	if (State.Effect_AutoFMMult >= 0)
		HandleEffect(EF_FDS_MOD_DEPTH, State.Effect_AutoFMMult);
}

std::string CChannelHandler::GetEffectString() const		// // //
{
	std::string str = GetSlideEffectString();
	
	if (m_iVibratoSpeed)
		str += MakeCommandString(EF_VIBRATO, (m_iVibratoSpeed << 4) | (m_iVibratoDepth >> 4));
	if (m_iTremoloSpeed)
		str += MakeCommandString(EF_TREMOLO, (m_iTremoloSpeed << 4) | (m_iTremoloDepth >> 4));
	if (m_iVolSlide)
		str += MakeCommandString(EF_VOLUME_SLIDE, m_iVolSlide);
	if (m_iFinePitch != 0x80)
		str += MakeCommandString(EF_PITCH, m_iFinePitch);
	if ((m_iDefaultDuty && m_iChannelID < CHANID_S5B_CH1) || (m_iDefaultDuty != 0x40 && m_iChannelID >= CHANID_S5B_CH1))
		str += MakeCommandString(EF_DUTY_CYCLE, m_iDefaultDuty);

	// run-time effects
	if (m_cDelayCounter >= 0 && m_bDelayEnabled)
		str += MakeCommandString(EF_DELAY, m_cDelayCounter + 1);
	if (m_iNoteRelease)
		str += MakeCommandString(EF_NOTE_RELEASE, m_iNoteRelease);
	if (m_iNoteVolume > 0)
		str += MakeCommandString(EF_DELAYED_VOLUME, (m_iNoteVolume << 4) | (m_iNewVolume >> VOL_COLUMN_SHIFT));
	if (m_iNoteCut)
		str += MakeCommandString(EF_NOTE_CUT, m_iNoteCut);
	if (m_iTranspose)
		str += MakeCommandString(EF_TRANSPOSE, ((m_iTranspose + (m_bTransposeDown ? 8 : 0)) << 4) | m_iTransposeTarget);

	str += GetCustomEffectString();
	return str.empty() ? std::string(" None") : str;
}

std::string CChannelHandler::GetSlideEffectString() const		// // //
{
	switch (m_iEffect) {
	case EF_ARPEGGIO:
		if (m_iEffectParam)
			return MakeCommandString(m_iEffect, m_iEffectParam);
		break;
	case EF_PORTA_UP: case EF_PORTA_DOWN: case EF_PORTAMENTO:
		if (m_iPortaSpeed)
			return MakeCommandString(m_iEffect, m_iPortaSpeed);
		break;
	}

	return std::string();
}

std::string CChannelHandler::GetCustomEffectString() const		// // //
{
	return std::string();
}

// Handle common things before letting the channels play the notes
void CChannelHandler::PlayNote(stChanNote NoteData)		// // //
{
	// // // Handle delay commands
	if (HandleDelay(NoteData))
		return;

	// Let the channel play
	HandleNoteData(NoteData);
}

void CChannelHandler::WriteEchoBuffer(const stChanNote &NoteData, int Pos)
{
	if (Pos < 0 || Pos > ECHO_BUFFER_LENGTH) return;
	int Value;
	switch (NoteData.Note) {
	case NONE: Value = ECHO_BUFFER_NONE; break;
	case HALT: Value = ECHO_BUFFER_HALT; break;
	case ECHO: Value = ECHO_BUFFER_ECHO + NoteData.Octave; break;
	default:
		Value = MIDI_NOTE(NoteData.Octave, NoteData.Note);
		for (int i = MAX_EFFECT_COLUMNS - 1; i >= 0; --i) {
			const int Param = NoteData.EffParam[i] & 0x0F;
			if (NoteData.EffNumber[i] == EF_SLIDE_UP) {
				Value += Param;
				break;
			}
			else if (NoteData.EffNumber[i] == EF_SLIDE_DOWN) {
				Value -= Param;
				break;
			}
			else if (NoteData.EffNumber[i] == EF_TRANSPOSE) {
				// Sometimes there are not enough ticks for the transpose to take place
				if (NoteData.EffParam[i] & 0x80)
					Value -= Param;
				else
					Value += Param;
				break;
			}
		}
		Value = std::max(std::min(Value, NOTE_COUNT - 1), 0);
	}

	m_iEchoBuffer[Pos] = Value;
}

void CChannelHandler::HandleNoteData(stChanNote &NoteData)
{
	int LastInstrument = m_iInstrument;
	int Instrument = NoteData.Instrument;
	bool Trigger = (NoteData.Note != NONE) && (NoteData.Note != HALT) && (NoteData.Note != RELEASE) &&
		Instrument != HOLD_INSTRUMENT;		// // // 050B
	bool pushNone = false;

	// // // Echo buffer
	if (NoteData.Note == ECHO && NoteData.Octave <= ECHO_BUFFER_LENGTH)
	{ // retrieve buffer
		int NewNote = m_iEchoBuffer[NoteData.Octave];
		if (NewNote == ECHO_BUFFER_NONE) {
			NoteData.Note = NONE;
			pushNone = true;
		}
		else if (NewNote == ECHO_BUFFER_HALT) NoteData.Note = HALT;
		else {
			NoteData.Note = GET_NOTE(NewNote);
			NoteData.Octave = GET_OCTAVE(NewNote);
		}
	}
	if (NoteData.Note != RELEASE && (NoteData.Note != NONE) || pushNone)
	{ // push buffer
		for (int i = ECHO_BUFFER_LENGTH; i > 0; i--)
			m_iEchoBuffer[i] = m_iEchoBuffer[i - 1];
		WriteEchoBuffer(NoteData, 0);
	}
	
	// Clear the note cut effect
	if (NoteData.Note != NONE) {
		m_iNoteCut = 0;
		m_iNoteRelease = 0;		// // //
		if (Trigger && m_iNoteVolume == 0 && !m_iVolSlide) {		// // //
			m_iVolume = m_iDefaultVolume;
			m_iNoteVolume = -1;
		}
		m_iTranspose = 0;		// // //
	}

	if (Trigger && (m_iEffect == EF_SLIDE_UP || m_iEffect == EF_SLIDE_DOWN))
		m_iEffect = EF_NONE;

	// Effects
	for (int n = 0; n < MAX_EFFECT_COLUMNS; n++) {
		effect_t      EffNum   = NoteData.EffNumber[n];
		unsigned char EffParam = NoteData.EffParam[n];
		HandleEffect(EffNum, EffParam);		// // // single method
		
		// 0CC: remove this eventually like how the asm handles it
		if (EffNum == EF_VOLUME_SLIDE && !EffParam && Trigger && m_iNoteVolume == 0) {		// // //
			m_iVolume = m_iDefaultVolume;
			m_iNoteVolume = -1;
		}
	}

	// Volume
	if (NoteData.Vol < MAX_VOLUME) {
		m_iVolume = NoteData.Vol << VOL_COLUMN_SHIFT;
		m_iDefaultVolume = m_iVolume;		// // //
	}

	// Instrument
	if (NoteData.Note == HALT || NoteData.Note == RELEASE)		// // //
		Instrument = MAX_INSTRUMENTS;	// Ignore instrument for release and halt commands

	if (Instrument != MAX_INSTRUMENTS && Instrument != HOLD_INSTRUMENT)		// // // 050B
		m_iInstrument = Instrument;

	bool NewInstrument = (m_iInstrument != LastInstrument && m_iInstrument != HOLD_INSTRUMENT) ||
		(m_iInstrument == MAX_INSTRUMENTS) || m_bForceReload;		// // // 050B

	if (m_iInstrument == MAX_INSTRUMENTS) {		// // // do nothing
		// m_iInstrument = m_pSoundGen->GetDefaultInstrument();
	}
	
	switch (NoteData.Note) {		// // // set note value before loading instrument
	case NONE: case HALT: case RELEASE: break;
	default: m_iNote = RunNote(NoteData.Octave, NoteData.Note);
	}

	// Note
	switch (NoteData.Note) {
		case NONE:
			HandleEmptyNote();
			break;
		case HALT:
			m_bRelease = false;
			HandleCut();
			break;
		case RELEASE:
			HandleRelease();
			break;
		default:
			HandleNote(NoteData.Note, NoteData.Octave);
			break;
	}

	if (Trigger && (m_iEffect == EF_SLIDE_DOWN || m_iEffect == EF_SLIDE_UP))		// // //
		SetupSlide();

	if ((NewInstrument || Trigger) && m_iInstrument != MAX_INSTRUMENTS) {
		if (!HandleInstrument(Trigger, NewInstrument)) {		// // //
			// m_bForceReload = false;		// // //
			// return;
		}
	}
	m_bForceReload = false;
}

bool CChannelHandler::HandleInstrument(bool Trigger, bool NewInstrument)		// // //
{
	auto pDoc = m_pSoundGen->GetDocumentInterface();
	if (!pDoc) return false;
	std::shared_ptr<CInstrument> pInstrument = pDoc->GetInstrumentManager()->GetInstrument(m_iInstrument);
	if (!pInstrument) return false;

	// load instrument here
	inst_type_t instType = pInstrument->GetType();
	if (NewInstrument)
		CreateInstHandler(instType);
	m_iInstTypeCurrent = instType;

	if (!m_pInstHandler)
		return false;
	if (NewInstrument)
		m_pInstHandler->LoadInstrument(pInstrument);
	if (Trigger || m_bForceReload)
		m_pInstHandler->TriggerInstrument();

	return true;
}

bool CChannelHandler::CreateInstHandler(inst_type_t Type)
{
	return false;
}

void CChannelHandler::SetNoteTable(const unsigned int *pNoteLookupTable)
{
	// Installs the note lookup table
	m_pNoteLookupTable = pNoteLookupTable;
}

int CChannelHandler::TriggerNote(int Note)
{
	Note = std::min(Note, NOTE_COUNT - 1);
	Note = std::max(Note, 0);

	// Trigger a note, return note period
	RegisterKeyState(Note);

	if (m_bLinearPitch)		// // //
		return Note << LINEAR_PITCH_AMOUNT;

	if (!m_pNoteLookupTable)
		return Note;

	return m_pNoteLookupTable[Note];
}

void CChannelHandler::FinishTick()		// // //
{
	m_bTrigger = false;
}

void CChannelHandler::CutNote()
{
	// Cut currently playing note

	RegisterKeyState(-1);

	m_bGate = false;
	m_iPeriod = 0;
	m_iPortaTo = 0;
}

void CChannelHandler::ReleaseNote()
{
	// Release currently playing note

	RegisterKeyState(-1);

	if (m_pInstHandler) m_pInstHandler->ReleaseInstrument();		// // //
	m_bRelease = true;
}

int CChannelHandler::RunNote(int Octave, int Note)
{
	// Run the note and handle portamento
	int NewNote = MIDI_NOTE(Octave, Note);

	int NesFreq = TriggerNote(NewNote);

	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO && m_bGate) {		// // //
		if (m_iPeriod == 0)
			m_iPeriod = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iPeriod = NesFreq;

	m_bGate = true;

	return NewNote;
}

void CChannelHandler::HandleNote(int Note, int Octave)		// // //
{
	m_iDutyPeriod = m_iDefaultDuty;
	m_bTrigger = true;
	m_bRelease = false;
}

void CChannelHandler::SetupSlide()		// // //
{
	#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1)

	switch (m_iEffect) {
	case EF_PORTAMENTO:
		m_iPortaSpeed = m_iEffectParam;
		if (m_bGate)		// // //
			m_iPortaTo = TriggerNote(m_iNote);
		break;
	case EF_SLIDE_UP:
		m_iNote = m_iNote + (m_iEffectParam & 0xF);
		m_iPortaSpeed = GET_SLIDE_SPEED(m_iEffectParam);
		m_iPortaTo = TriggerNote(m_iNote);
		break;
	case EF_SLIDE_DOWN:
		m_iNote = m_iNote - (m_iEffectParam & 0xF);
		m_iPortaSpeed = GET_SLIDE_SPEED(m_iEffectParam);
		m_iPortaTo = TriggerNote(m_iNote);
		break;
	}
}

bool CChannelHandler::HandleEffect(effect_t EffCmd, unsigned char EffParam)
{
	// Handle common effects for all channels

	switch (EffCmd) {
	case EF_PORTAMENTO:
		m_iEffectParam = EffParam;		// // //
		m_iEffect = EF_PORTAMENTO;
		SetupSlide();
		if (!EffParam)
			m_iPortaTo = 0;
		break;
	case EF_VIBRATO:
		m_iVibratoDepth = (EffParam & 0x0F) << 4;
		m_iVibratoSpeed = EffParam >> 4;
		if (!EffParam)
			m_iVibratoPhase = !m_bNewVibratoMode ? 48 : 0;
		break;
	case EF_TREMOLO:
		m_iTremoloDepth = (EffParam & 0x0F) << 4;
		m_iTremoloSpeed = EffParam >> 4;
		if (!EffParam)
			m_iTremoloPhase = 0;
		break;
	case EF_ARPEGGIO:
		m_iEffectParam = EffParam;		// // //
		m_iEffect = EF_ARPEGGIO;
		break;
	case EF_PITCH:
		m_iFinePitch = EffParam;
		break;
	case EF_PORTA_DOWN:
		m_iPortaSpeed = EffParam;
		m_iEffectParam = EffParam;		// // //
		m_iEffect = EF_PORTA_DOWN;
		break;
	case EF_PORTA_UP:
		m_iPortaSpeed = EffParam;
		m_iEffectParam = EffParam;		// // //
		m_iEffect = EF_PORTA_UP;
		break;
	case EF_SLIDE_UP:		// // //
		m_iEffectParam = EffParam;
		m_iEffect = EF_SLIDE_UP;
		SetupSlide();
		break;
	case EF_SLIDE_DOWN:		// // //
		m_iEffectParam = EffParam;
		m_iEffect = EF_SLIDE_DOWN;
		SetupSlide();
		break;
	case EF_VOLUME_SLIDE:
		m_iVolSlide = EffParam;
		if (!EffParam)		// // //
			m_iDefaultVolume = m_iVolume;
		break;
	case EF_NOTE_CUT:
		if (EffParam >= 0x80) return false;		// // //
		m_iNoteCut = EffParam + 1;
		break;
	case EF_NOTE_RELEASE:		// // //
		if (EffParam >= 0x80) return false;
		m_iNoteRelease = EffParam + 1;
		break;
	case EF_DELAYED_VOLUME:		// // //
		if (!(EffParam >> 4) || !(EffParam & 0xF)) break;
		m_iNoteVolume = (EffParam >> 4) + 1;
		m_iNewVolume = (EffParam & 0x0F) << VOL_COLUMN_SHIFT;
		break;
	case EF_TRANSPOSE:		// // //
		m_iTranspose = ((EffParam & 0x70) >> 4) + 1;
		m_iTransposeTarget = EffParam & 0x0F;
		m_bTransposeDown = (EffParam & 0x80) != 0;
		break;
//	case EF_TARGET_VOLUME_SLIDE:
		// TODO implement
//		break;
	default:
		return false;
	}
	
	return true;
}

bool CChannelHandler::HandleDelay(stChanNote &NoteData)
{
	// Handle note delay, Gxx

	if (m_bDelayEnabled) {
		m_bDelayEnabled = false;
		HandleNoteData(m_cnDelayed);		// // //
	}
	
	// Check delay
	for (int i = 0; i < MAX_EFFECT_COLUMNS; ++i) {
		if (NoteData.EffNumber[i] == EF_DELAY && NoteData.EffParam[i] > 0) {
			m_bDelayEnabled = true;
			m_cDelayCounter = NoteData.EffParam[i];

			// Only one delay/row is allowed
			for (int j = 0; j < MAX_EFFECT_COLUMNS; ++j) {
				if (NoteData.EffNumber[j] == EF_DELAY) {		// // //
					NoteData.EffNumber[j] = EF_NONE;
					NoteData.EffParam[j] = 0;
				}
			}
			
			m_cnDelayed = NoteData;		// // //
			return true;
		}
	}

	return false;
}

void CChannelHandler::UpdateNoteCut()
{
	// Note cut (Sxx)
	if (m_iNoteCut > 0) if (!--m_iNoteCut)
		HandleCut();
}

void CChannelHandler::UpdateNoteRelease()		// // //
{
	// Note release (Lxx)
	if (m_iNoteRelease > 0) if (!--m_iNoteRelease) {
		HandleRelease();
		ReleaseNote();
	}
}

void CChannelHandler::UpdateNoteVolume()		// // //
{
	// Delayed channel volume (Mxy)
	if (m_iNoteVolume > 0) if (!--m_iNoteVolume)
		m_iVolume = m_iNewVolume;
}

void CChannelHandler::UpdateTranspose()		// // //
{
	// Delayed transpose (Txy)
	if (m_iTranspose > 0) if (!--m_iTranspose) {
		// trigger note
		SetNote(m_iNote + m_iTransposeTarget * (m_bTransposeDown ? -1 : 1));
		SetPeriod(TriggerNote(m_iNote));
	}
}

void CChannelHandler::UpdateDelay()
{
	// Delay (Gxx)
	if (m_bDelayEnabled) {
		if (!m_cDelayCounter) {
			m_bDelayEnabled = false;
			PlayNote(m_cnDelayed);		// // //
		}
		else
			m_cDelayCounter--;
	}
}

void CChannelHandler::UpdateVolumeSlide()
{
	// Volume slide (Axy)
	m_iVolume -= (m_iVolSlide & 0x0F);
	if (m_iVolume < 0)
		m_iVolume = 0;

	m_iVolume += (m_iVolSlide & 0xF0) >> 4;
	if (m_iVolume > VOL_COLUMN_MAX)
		m_iVolume = VOL_COLUMN_MAX;
}

void CChannelHandler::UpdateTargetVolumeSlide()
{
	// TODO implement
}

void CChannelHandler::UpdateVibratoTremolo()
{
	// Vibrato and tremolo
	m_iVibratoPhase = (m_iVibratoPhase + m_iVibratoSpeed) & 63;
	m_iTremoloPhase = (m_iTremoloPhase + m_iTremoloSpeed) & 63;
}

void CChannelHandler::PeriodAdd(int Step)
{
	SetPeriod(GetPeriod() + Step);		// // // uniform
}

void CChannelHandler::PeriodRemove(int Step)
{
	SetPeriod(GetPeriod() - Step);		// // //
}

void CChannelHandler::UpdateEffects()
{
	// Handle other effects
	switch (m_iEffect) {
		case EF_ARPEGGIO:
			if (m_iEffectParam != 0) {
				switch (m_iArpState) {
					case 0:
						SetPeriod(TriggerNote(m_iNote));
						break;
					case 1:
						SetPeriod(TriggerNote(m_iNote + (m_iEffectParam >> 4)));
						if ((m_iEffectParam & 0x0F) == 0)
							++m_iArpState;
						break;
					case 2:
						SetPeriod(TriggerNote(m_iNote + (m_iEffectParam & 0x0F)));
						break;
				}
				m_iArpState = (m_iArpState + 1) % 3;
			}
			break;
		case EF_PORTAMENTO:
		case EF_SLIDE_UP:		// // //
		case EF_SLIDE_DOWN:		// // //
			// Automatic portamento
			if (m_iPortaSpeed > 0 && m_iPortaTo) {		// // //
				if (m_iPeriod > m_iPortaTo) {
					PeriodRemove(m_iPortaSpeed);
					if (m_iPeriod <= m_iPortaTo) {
						SetPeriod(m_iPortaTo);
						if (m_iEffect != EF_PORTAMENTO) {
							m_iPortaTo = 0;
							m_iPortaSpeed = 0;
							m_iEffect = EF_NONE;
						}
					}
				}
				else if (m_iPeriod < m_iPortaTo) {
					PeriodAdd(m_iPortaSpeed);
					if (m_iPeriod >= m_iPortaTo) {
						SetPeriod(m_iPortaTo);
						if (m_iEffect != EF_PORTAMENTO) {
							m_iPortaTo = 0;
							m_iPortaSpeed = 0;
							m_iEffect = EF_NONE;
						}
					}
				}
			}
			break;
			/*
		case EF_SLIDE_UP:
			if (m_iPortaSpeed > 0) {
				if (m_iPeriod > m_iPortaTo) {
					PeriodRemove(m_iPortaSpeed);
					if (m_iPeriod < m_iPortaTo) {
						SetPeriod(m_iPortaTo);
						m_iPortaTo = 0;
						m_iEffect = EF_NONE;
					}
				}
			}
			break;
		case EF_SLIDE_DOWN:
			if (m_iPortaSpeed > 0) {
				PeriodAdd(m_iPortaSpeed);
				if (m_iPeriod > m_iPortaTo) {
					SetPeriod(m_iPortaTo);
					m_iPortaTo = 0;
					m_iEffect = EF_NONE;
				}
			}
			break;
			*/
		case EF_PORTA_DOWN:
			m_bLinearPitch ? PeriodRemove(m_iPortaSpeed) : PeriodAdd(m_iPortaSpeed);		// // //
			break;
		case EF_PORTA_UP:
			m_bLinearPitch ? PeriodAdd(m_iPortaSpeed) : PeriodRemove(m_iPortaSpeed);		// // //
			break;
	}
}

void CChannelHandler::ProcessChannel()
{
	// Run all default and common channel processing
	// This gets called each frame
	//
	
	UpdateDelay();
	UpdateNoteCut();
	UpdateNoteRelease();		// // //
	UpdateNoteVolume();			// // //
	UpdateTranspose();			// // //
	UpdateVolumeSlide();
	UpdateVibratoTremolo();
	UpdateEffects();
	if (m_pInstHandler) m_pInstHandler->UpdateInstrument();		// // //
	// instruments are updated after running effects and before writing to sound registers
}

int CChannelHandler::GetVibrato() const
{
	// Vibrato offset (4xx)
	int VibFreq;

	if ((m_iVibratoPhase & 0xF0) == 0x00)
		VibFreq = m_pVibratoTable[m_iVibratoDepth + m_iVibratoPhase];
	else if ((m_iVibratoPhase & 0xF0) == 0x10)
		VibFreq = m_pVibratoTable[m_iVibratoDepth + 15 - (m_iVibratoPhase - 16)];
	else if ((m_iVibratoPhase & 0xF0) == 0x20)
		VibFreq = -m_pVibratoTable[m_iVibratoDepth + (m_iVibratoPhase - 32)];
	else if ((m_iVibratoPhase & 0xF0) == 0x30)
		VibFreq = -m_pVibratoTable[m_iVibratoDepth + 15 - (m_iVibratoPhase - 48)];

	if (!m_bNewVibratoMode) {
		VibFreq += m_pVibratoTable[m_iVibratoDepth + 15] + 1;
		VibFreq >>= 1;
	}

//	if (m_bLinearPitch)
//		VibFreq <<= 1;		// // //

	return VibFreq;
}

int CChannelHandler::GetTremolo() const
{
	// Tremolo offset (7xx)
	int TremVol = 0;
	int Phase = m_iTremoloPhase >> 1;

	if ((Phase & 0xF0) == 0x00)
		TremVol = m_pVibratoTable[m_iTremoloDepth + Phase];
	else if ((Phase & 0xF0) == 0x10)
		TremVol = m_pVibratoTable[m_iTremoloDepth + 15 - (Phase - 16)];

	return (TremVol >> 1);
}

int CChannelHandler::GetFinePitch() const
{
	// Fine pitch setting (Pxx)
	return (0x80 - m_iFinePitch);
}

int CChannelHandler::CalculatePeriod() const 
{
	int Detune = GetVibrato() - GetFinePitch() - GetPitch();
	int Period = LimitPeriod(GetPeriod() - Detune);		// // //
	if (m_bLinearPitch && m_pNoteLookupTable != nullptr) {
		Period = LimitPeriod(GetPeriod() + Detune);
		int Note = Period >> LINEAR_PITCH_AMOUNT;
		int Sub = Period % (1 << LINEAR_PITCH_AMOUNT);
		int Offset = Note < NOTE_COUNT - 1 ? m_pNoteLookupTable[Note] - m_pNoteLookupTable[Note + 1] : 0;
		Offset = Offset * Sub >> LINEAR_PITCH_AMOUNT;
		if (Sub && !Offset) Offset = 1;
		Period = m_pNoteLookupTable[Note] - Offset;
	}
	return LimitRawPeriod(Period);
}

int CChannelHandler::CalculateVolume() const
{
	// Volume calculation
	return LimitVolume((m_iInstVolume * (m_iVolume >> VOL_COLUMN_SHIFT)) / 15 - GetTremolo());		// // //
}

int CChannelHandler::LimitPeriod(int Period) const		// // // virtual
{
	if (!m_bLinearPitch) return LimitRawPeriod(Period);
	return std::min(std::max(Period, 0), (NOTE_COUNT - 1) << LINEAR_PITCH_AMOUNT);
}

int CChannelHandler::LimitRawPeriod(int Period) const
{
	return std::min(std::max(Period, 0), m_iMaxPeriod);
}

int CChannelHandler::LimitVolume(int Volume) const		// // //
{
	if (!m_bGate)
		return 0;

	Volume = std::max(0, std::min(m_iMaxVolume, Volume));
	if (Volume == 0 && !theApp.GetSettings()->General.bCutVolume && m_iInstVolume > 0 && m_iVolume > 0)		// // //
		return 1;
	return Volume;
}

void CChannelHandler::WriteRegister(uint16_t Reg, uint8_t Value)
{
	m_pAPU->Write(Reg, Value);
	m_pSoundGen->WriteRegister(Reg, Value);
}

void CChannelHandler::RegisterKeyState(int Note)
{
	m_pSoundGen->RegisterKeyState(m_iChannelID, Note);
}

void CChannelHandler::SetPeriod(int Period)
{
	m_iPeriod = LimitPeriod(Period);
}

int CChannelHandler::GetPeriod() const
{
	return m_iPeriod;
}

void CChannelHandler::SetNote(int Note)
{
	m_iNote = Note;
}

int CChannelHandler::GetNote() const
{
	return m_iNote;
}

void CChannelHandler::SetVolume(int Volume)
{
	m_iInstVolume = Volume;
}

int CChannelHandler::GetVolume() const
{
	return m_iInstVolume;
}

int CChannelHandler::GetChannelVolume() const		// // //
{
	return m_iVolume;
}

void CChannelHandler::SetDutyPeriod(int Duty)
{
	m_iDutyPeriod = ConvertDuty(Duty);		// // //
}

int CChannelHandler::GetDutyPeriod() const
{
	return m_iDutyPeriod;
}

unsigned char CChannelHandler::GetArpParam() const
{
	return m_iEffect == EF_ARPEGGIO ? m_iEffectParam : 0U;
}

bool CChannelHandler::IsActive() const
{
	return m_bGate;
}

bool CChannelHandler::IsReleasing() const
{
	return m_bRelease;
}

/*
 * Class CChannelHandlerInverted
 *
 */

bool CChannelHandlerInverted::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	if (!m_bLinearPitch) switch (EffNum) {		// // //
	case EF_PORTA_UP: EffNum = EF_PORTA_DOWN; break;
	case EF_PORTA_DOWN: EffNum = EF_PORTA_UP; break;
	}
	return CChannelHandler::HandleEffect(EffNum, EffParam);
}

int CChannelHandlerInverted::CalculatePeriod() const
{
	int Period = LimitPeriod(GetPeriod() + GetVibrato() - GetFinePitch() - GetPitch());		// // //
	if (m_bLinearPitch && m_pNoteLookupTable != nullptr) {
		int Note = Period >> LINEAR_PITCH_AMOUNT;
		int Sub = Period % (1 << LINEAR_PITCH_AMOUNT);
		int Offset = Note < NOTE_COUNT - 1 ? m_pNoteLookupTable[Note + 1] - m_pNoteLookupTable[Note] : 0;
		Offset = Offset * Sub >> LINEAR_PITCH_AMOUNT;
		if (Sub && !Offset) Offset = 1;
		Period = m_pNoteLookupTable[Note] + Offset;
	}
	return LimitRawPeriod(Period);
}

std::string CChannelHandlerInverted::GetSlideEffectString() const		// // //
{
	switch (m_iEffect) {
	case EF_PORTA_UP:
		return MakeCommandString(EF_PORTA_DOWN, m_iPortaSpeed);
	case EF_PORTA_DOWN:
		return MakeCommandString(EF_PORTA_UP, m_iPortaSpeed);
	}
	return CChannelHandler::GetSlideEffectString();
}
