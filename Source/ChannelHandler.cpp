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

//
// This is the base class for all classes that takes care of 
// translating notes to channel register writes.
//

#include <algorithm>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "SoundGen.h"
#include "Settings.h"		// // //
#include "ChannelHandler.h"
#include "APU/APU.h"

const int BUFFER_NONE = -1;
const int BUFFER_HALT = 0x7F;
const int BUFFER_ECHO = 0x80; // used to retrieve the full echo buffer

/*
 * Class CChannelHandler
 *
 */

CChannelHandler::CChannelHandler(int MaxPeriod, int MaxVolume) : 
	m_iChannelID(0), 
	m_iInstrument(0), 
	m_iLastInstrument(MAX_INSTRUMENTS),
	m_pNoteLookupTable(NULL),
	m_pVibratoTable(NULL),
	m_pAPU(NULL),
	m_iPitch(0),
	m_iNote(0),
	m_iSeqVolume(0),
	m_iDefaultDuty(0),
	m_iDutyPeriod(0),
	m_iMaxPeriod(MaxPeriod),
	m_iMaxVolume(MaxVolume),
	m_bGate(false),
	m_bNewVibratoMode(false),
	m_bLinearPitch(false),
	m_bPeriodUpdated(false),
	m_bVolumeUpdate(false)
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

	m_iEffect = 0;

	DocumentPropertiesChanged(pSoundGen->GetDocument());

	ResetChannel();
}

void CChannelHandler::DocumentPropertiesChanged(CFamiTrackerDoc *pDoc)
{
	m_bNewVibratoMode = (pDoc->GetVibratoStyle() == VIBRATO_NEW);
	m_bLinearPitch = pDoc->GetLinearPitch();
}

int CChannelHandler::LimitPeriod(int Period) const
{
	Period = std::min(Period, m_iMaxPeriod);
	Period = std::max(Period, 0);
	return Period;
}

int CChannelHandler::LimitVolume(int Volume) const
{
	Volume = std::min(Volume, 15);
	Volume = std::max(Volume, 0);
	return Volume;
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

void CChannelHandler::ResetChannel()
{
	// Resets the channel states (volume, instrument & duty)
	// Clears channel registers

	// Instrument 
	m_iInstrument		= MAX_INSTRUMENTS;
	m_iLastInstrument	= MAX_INSTRUMENTS;

	// Volume 
	m_iVolume			= VOL_COLUMN_MAX;
	m_iDefaultVolume	= (VOL_COLUMN_MAX >> VOL_COLUMN_SHIFT) << VOL_COLUMN_SHIFT;		// // //

	m_iDefaultDuty		= 0;
	m_iSeqVolume		= 0;

	// Period
	m_iPeriod			= 0;
	m_iLastPeriod		= 0xFFFF;
	m_iPeriodPart		= 0;

	// Effect states
	m_iPortaSpeed		= 0;
	m_iPortaTo			= 0;
	m_iArpeggio			= 0;
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
	m_iVibratoDepth		= 0;
	m_iTremoloDepth		= 0;

	for (int i = 0; i <= ECHO_BUFFER_LENGTH; i++)		// // //
		m_iEchoBuffer[i] = BUFFER_NONE;

	// States
	m_bRelease			= false;
	m_bGate				= false;

	RegisterKeyState(-1);

	// Clear channel registers
	ClearRegisters();

	ClearSequences();
}

void CChannelHandler::RetrieveChannelState()		// // //
{
	CFamiTrackerDoc *pDoc = m_pSoundGen->GetDocument();
	if (!pDoc) return;

	int Track = m_pSoundGen->GetPlayerTrack();
	int Frame = m_pSoundGen->GetPlayerFrame();
	int Row = m_pSoundGen->GetPlayerRow();
	int Channel = pDoc->GetChannelPosition(m_iChannelID, pDoc->GetExpansionChip());
	int EffColumns = pDoc->GetEffColumns(Track, Channel);

	int f = Frame, r = Row, BufferPos = -1, State[EF_COUNT], Transpose[ECHO_BUFFER_LENGTH + 1] = {};
	memset(State, -1, EF_COUNT * sizeof(int));

	while (true) {
		stChanNote Note;
		pDoc->GetNoteData(Track, f, Channel, r, &Note);
		
		if (Note.Note != NONE && Note.Note != RELEASE) {
			for (int i = 0; i < std::min(BufferPos, ECHO_BUFFER_LENGTH + 1); i++) {
				if (m_iEchoBuffer[i] == BUFFER_ECHO) {
					for (int j = EffColumns; j >= 0; j--) {
						const int Param = Note.EffParam[j] & 0x0F;
						if (Note.EffNumber[j] == EF_SLIDE_UP) {
							Transpose[i] += Param; break;
						}
						else if (Note.EffNumber[j] == EF_SLIDE_DOWN) {
							Transpose[i] -= Param; break;
						}
						else if (Note.EffNumber[j] == EF_TRANSPOSE) {
							// Sometimes there are not enough ticks for the transpose to take place
							if (Note.EffParam[j] & 0x80) Transpose[i] -= Param;
							else Transpose[i] += Param;
							break;
						}
					}
					switch (Note.Note) {
					case HALT: m_iEchoBuffer[i] = BUFFER_HALT; break;
					case ECHO: m_iEchoBuffer[i] = BUFFER_ECHO + Note.Octave; break;
					default:
						int NewNote = MIDI_NOTE(Note.Octave, Note.Note) + Transpose[i];
						NewNote = std::max(std::min(NewNote, NOTE_COUNT - 1), 0);
						m_iEchoBuffer[i] = NewNote;
					}
				}
				else if (m_iEchoBuffer[i] > BUFFER_ECHO && m_iEchoBuffer[i] <= BUFFER_ECHO + ECHO_BUFFER_LENGTH)
					m_iEchoBuffer[i]--;
			}
			if (BufferPos >= 0 && BufferPos <= ECHO_BUFFER_LENGTH) {
				WriteEchoBuffer(&Note, BufferPos, EffColumns);
				for (int j = EffColumns; j >= 0; j--) { // 0CC: optimize this
					const int Param = Note.EffParam[j] & 0x0F;
					if (Note.EffNumber[j] == EF_SLIDE_UP) {
						Transpose[BufferPos] += Param; break;
					}
					else if (Note.EffNumber[j] == EF_SLIDE_DOWN) {
						Transpose[BufferPos] -= Param; break;
					}
					else if (Note.EffNumber[j] == EF_TRANSPOSE) {
						if (Note.EffParam[j] & 0x80) Transpose[BufferPos] -= Param;
						else Transpose[BufferPos] += Param;
						break;
					}
				}
			}
			BufferPos++;
		}
		if (BufferPos < 0)
			BufferPos = 0;

		if (m_iInstrument == MAX_INSTRUMENTS)
			if (Note.Instrument != MAX_INSTRUMENTS)
				m_iInstrument = Note.Instrument;

		if (m_iVolume == VOL_COLUMN_MAX)
			if (Note.Vol != MAX_VOLUME)
				m_iDefaultVolume = m_iVolume = Note.Vol << VOL_COLUMN_SHIFT;
		
		for (int c = EffColumns; c >= 0; c--)
			switch (Note.EffNumber[c]) {
			case EF_NONE: case EF_PORTAOFF:
			case EF_DPCM_PITCH: case EF_RETRIGGER:
			case EF_DELAY: case EF_DELAYED_VOLUME: case EF_NOTE_RELEASE: case EF_TRANSPOSE:
				continue; // ignore effects that cannot have memory
			case EF_JUMP: case EF_SKIP: case EF_HALT:
			case EF_SPEED: case EF_GROOVE:
				if (true) continue; // ignore global effects
			case EF_NOTE_CUT:
				if (m_iChannelID != CHANID_TRIANGLE) continue;
				else if (Note.EffParam[c] < 0x80) continue;

			case EF_VOLUME: case EF_VIBRATO: case EF_TREMOLO:
			case EF_PITCH: case EF_DUTY_CYCLE: case EF_SAMPLE_OFFSET: case EF_VOLUME_SLIDE:
			case EF_FDS_MOD_DEPTH: case EF_FDS_MOD_SPEED_HI: case EF_FDS_MOD_SPEED_LO:
			case EF_SUNSOFT_ENV_LO: case EF_SUNSOFT_ENV_HI: case EF_SUNSOFT_ENV_TYPE:
			case EF_DAC: case EF_N163_WAVE_BUFFER:
				if (State[Note.EffNumber[c]] == -1)
					State[Note.EffNumber[c]] = Note.EffParam[c];
				continue;
			case EF_SWEEPUP: case EF_SWEEPDOWN: case EF_SLIDE_UP: case EF_SLIDE_DOWN:
			case EF_PORTAMENTO: case EF_ARPEGGIO: case EF_PORTA_UP: case EF_PORTA_DOWN:
				if (State[EF_PORTAMENTO] == -1) { // anything else within can be used here
					State[EF_PORTAMENTO] = Note.EffNumber[c] == EF_PORTAMENTO ? Note.EffParam[c] : -2;
					State[EF_ARPEGGIO] = Note.EffNumber[c] == EF_ARPEGGIO ? Note.EffParam[c] : -2;
					State[EF_PORTA_UP] = Note.EffNumber[c] == EF_PORTA_UP ? Note.EffParam[c] : -2;
					State[EF_PORTA_DOWN] = Note.EffNumber[c] == EF_PORTA_DOWN ? Note.EffParam[c] : -2;
				}
				continue;
			}
		if (r) r--;
		else if (f) r = pDoc->GetFrameLength(Track, --f) - 1;
		else break;
	}

	for (unsigned int i = 0; i < EF_COUNT; i++)
		if (State[i] >= 0)
			HandleCustomEffects(i, State[i]);

	return;
}

// Handle common things before letting the channels play the notes
void CChannelHandler::PlayNote(stChanNote *pNoteData, int EffColumns)
{
	ASSERT (pNoteData != NULL);

	// Handle delay commands
	if (HandleDelay(pNoteData, EffColumns))
		return;

	// Handle global effects
	m_pSoundGen->EvaluateGlobalEffects(pNoteData, EffColumns);

	// Let the channel play
	HandleNoteData(pNoteData, EffColumns);
}

void CChannelHandler::WriteEchoBuffer(stChanNote *NoteData, int Pos, int EffColumns)
{
	if (Pos < 0 || Pos > ECHO_BUFFER_LENGTH) return;
	int Value;
	switch (NoteData->Note) {
	case NONE: Value = BUFFER_NONE; break;
	case HALT: Value = BUFFER_HALT; break;
	case ECHO: Value = BUFFER_ECHO + NoteData->Octave; break;
	default:
		Value = MIDI_NOTE(NoteData->Octave, NoteData->Note);
		for (int i = EffColumns; i >= 0; i--) {
			const int Param = NoteData->EffParam[i] & 0x0F;
			if (NoteData->EffNumber[i] == EF_SLIDE_UP) {
				Value += Param;
				break;
			}
			else if (NoteData->EffNumber[i] == EF_SLIDE_DOWN) {
				Value -= Param;
				break;
			}
			else if (NoteData->EffNumber[i] == EF_TRANSPOSE) {
				// Sometimes there are not enough ticks for the transpose to take place
				if (NoteData->EffParam[i] & 0x80)
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

void CChannelHandler::HandleNoteData(stChanNote *pNoteData, int EffColumns)
{
	int LastInstrument = m_iInstrument;
	int Instrument = pNoteData->Instrument;
	bool Trigger = (pNoteData->Note != NONE) && (pNoteData->Note != HALT) && (pNoteData->Note != RELEASE);
	bool pushNone = false;

	// // // Echo buffer
	if (pNoteData->Note == ECHO && pNoteData->Octave <= ECHO_BUFFER_LENGTH)
	{ // retrieve buffer
		int NewNote = m_iEchoBuffer[pNoteData->Octave];
		if (NewNote == BUFFER_NONE) {
			pNoteData->Note = NONE;
			pushNone = true;
		}
		else if (NewNote == BUFFER_HALT) pNoteData->Note = HALT;
		else {
			pNoteData->Note = GET_NOTE(NewNote);
			pNoteData->Octave = GET_OCTAVE(NewNote);
		}
	}
	if (pNoteData->Note != RELEASE && (pNoteData->Note != NONE) || pushNone)
	{ // push buffer
		for (int i = ECHO_BUFFER_LENGTH; i > 0; i--)
			m_iEchoBuffer[i] = m_iEchoBuffer[i - 1];
		WriteEchoBuffer(pNoteData, 0, EffColumns);
	}
	
	// Clear the note cut effect
	if (pNoteData->Note != NONE) {
		m_iNoteCut = 0;
		m_iNoteRelease = 0;		// // //
		if (Trigger && m_iNoteVolume == 0 && !m_iVolSlide) {		// // //
			m_iVolume = m_iDefaultVolume;
			m_iNoteVolume = -1;
		}
		m_iTranspose = 0;		// // //
	}

	// Effects
	for (int n = 0; n < EffColumns; n++) {
		unsigned char EffNum   = pNoteData->EffNumber[n];
		unsigned char EffParam = pNoteData->EffParam[n];
		HandleCustomEffects(EffNum, EffParam);
		
		// 0CC: remove this eventually like how the asm handles it
		if (EffNum == EF_VOLUME_SLIDE && !EffParam && Trigger && m_iNoteVolume == 0) {		// // //
			m_iVolume = m_iDefaultVolume;
			m_iNoteVolume = -1;
		}
	}

	// Volume
	if (pNoteData->Vol < MAX_VOLUME) {
		m_iVolume = pNoteData->Vol << VOL_COLUMN_SHIFT;
		m_iDefaultVolume = m_iVolume;		// // //
	}

	// Instrument
	if (pNoteData->Note == HALT || pNoteData->Note == RELEASE)		// // //
		Instrument = MAX_INSTRUMENTS;	// Ignore instrument for release and halt commands

	if (Instrument != MAX_INSTRUMENTS)
		m_iInstrument = Instrument;

	bool NewInstrument = (m_iInstrument != LastInstrument) || (m_iInstrument == MAX_INSTRUMENTS);

	if (m_iInstrument == MAX_INSTRUMENTS) {
		// No instrument selected, default to 0
		// 0CC: retrieve
		m_iInstrument = m_pSoundGen->GetDefaultInstrument();
	}

	if (NewInstrument || Trigger) {
		if (!HandleInstrument(m_iInstrument, Trigger, NewInstrument))
			return;
	}

	// Clear release flag
	if (pNoteData->Note != RELEASE && pNoteData->Note != NONE) {
		m_bRelease = false;
	}

	// Note
	switch (pNoteData->Note) {
		case NONE:
			HandleEmptyNote();
			break;
		case HALT:
			HandleCut();
			break;
		case RELEASE:
			HandleRelease();
			break;
		default:
			HandleNote(pNoteData->Note, pNoteData->Octave);
			break;
	}
}

void CChannelHandler::SetNoteTable(unsigned int *pNoteLookupTable)
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

	if (!m_pNoteLookupTable)
		return Note;

	return m_pNoteLookupTable[Note];
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

	m_bRelease = true;
}

int CChannelHandler::RunNote(int Octave, int Note)
{
	// Run the note and handle portamento
	int NewNote = MIDI_NOTE(Octave, Note);
	if (m_iChannelID == CHANID_NOISE)		// // //
		NewNote = (NewNote & 0x0F) | 0x100;

	int NesFreq = TriggerNote(NewNote);

	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO) {
		if (m_iPeriod == 0)
			m_iPeriod = NesFreq;
		m_iPortaTo = NesFreq;
	}
	else
		m_iPeriod = NesFreq;

	m_bGate = true;

	return NewNote;
}

void CChannelHandler::SetupSlide(int Type, int EffParam)
{
	#define GET_SLIDE_SPEED(x) (((x & 0xF0) >> 3) + 1)

	m_iPortaSpeed = GET_SLIDE_SPEED(EffParam);
	m_iEffect = Type;

	if (Type == EF_SLIDE_UP)
		m_iNote = m_iNote + (EffParam & 0xF);
	else
		m_iNote = m_iNote - (EffParam & 0xF);
	
	if (m_iChannelID == CHANID_NOISE) {		// // //
		m_iNote = m_iNote % 0x10 + 0x100;
	}

	m_iPortaTo = TriggerNote(m_iNote);
}

bool CChannelHandler::CheckCommonEffects(unsigned char EffCmd, unsigned char EffParam)
{
	// Handle common effects for all channels

	switch (EffCmd) {
		case EF_PORTAMENTO:
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTAMENTO;
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
			m_iArpeggio = EffParam;
			m_iEffect = EF_ARPEGGIO;
			break;
		case EF_PITCH:
			m_iFinePitch = EffParam;
			break;
		case EF_PORTA_DOWN:
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTA_DOWN;
			break;
		case EF_PORTA_UP:
			m_iPortaSpeed = EffParam;
			m_iEffect = EF_PORTA_UP;
			break;
		case EF_VOLUME_SLIDE:
			m_iVolSlide = EffParam;
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
			m_iTranspose = (EffParam >> 4) + 1;
			m_iTransposeTarget = EffParam & 0x0F;
			break;
//		case EF_TARGET_VOLUME_SLIDE:
			// TODO implement
//			break;
		default:
			return false;
	}
	
	return true;
}

bool CChannelHandler::HandleDelay(stChanNote *pNoteData, int EffColumns)
{
	// Handle note delay, Gxx

	if (m_bDelayEnabled) {
		m_bDelayEnabled = false;
		HandleNoteData(&m_cnDelayed, m_iDelayEffColumns);
	}
	
	// Check delay
	for (int i = 0; i < EffColumns; ++i) {
		if (pNoteData->EffNumber[i] == EF_DELAY && pNoteData->EffParam[i] > 0) {
			m_bDelayEnabled = true;
			m_cDelayCounter = pNoteData->EffParam[i];
			m_iDelayEffColumns = EffColumns;
			memcpy(&m_cnDelayed, pNoteData, sizeof(stChanNote));

			// Only one delay/row is allowed. Remove global effects
			for (int j = 0; j < EffColumns; ++j) {
				switch (m_cnDelayed.EffNumber[j]) {
					case EF_DELAY:
						m_cnDelayed.EffNumber[j] = EF_NONE;
						m_cnDelayed.EffParam[j] = 0;
						break;
					case EF_JUMP:
						m_pSoundGen->SetJumpPattern(m_cnDelayed.EffParam[j]);
						m_cnDelayed.EffNumber[j] = EF_NONE;
						m_cnDelayed.EffParam[j] = 0;
						break;
					case EF_SKIP:
						m_pSoundGen->SetSkipRow(m_cnDelayed.EffParam[j]);
						m_cnDelayed.EffNumber[j] = EF_NONE;
						m_cnDelayed.EffParam[j] = 0;
						break;
				}
			}
			return true;
		}
	}

	return false;
}

void CChannelHandler::UpdateNoteCut()
{
	// Note cut ()
	if (m_iNoteCut > 0) {
		m_iNoteCut--;
		if (m_iNoteCut == 0) {
			HandleCut();
		}
	}
}

void CChannelHandler::UpdateNoteRelease()		// // //
{
	// Note release (Lxx)
	if (m_iNoteRelease > 0) {
		m_iNoteRelease--;
		if (m_iNoteRelease == 0) {
			HandleRelease();
			ReleaseNote();
		}
	}
}

void CChannelHandler::UpdateNoteVolume()		// // //
{
	// Delayed channel volume (Mxy)
	if (m_iNoteVolume > 0) {
		m_iNoteVolume--;
		if (m_iNoteVolume == 0) {
			m_iVolume = m_iNewVolume;
		}
	}
}

void CChannelHandler::UpdateTranspose()		// // //
{
	// Delayed transpose (Txy)
	if (m_iTranspose != 0 && m_iTranspose != 8) {
		m_iTranspose--;
		if (!(m_iTranspose % 0x08)) {
			// trigger note
			SetNote(GetNote() + m_iTransposeTarget * (m_iTranspose ? -1 : 1));
			SetPeriod(TriggerNote(m_iNote));
		}
	}
}

void CChannelHandler::UpdateDelay()
{
	// Delay (Gxx)
	if (m_bDelayEnabled) {
		if (!m_cDelayCounter) {
			m_bDelayEnabled = false;
			PlayNote(&m_cnDelayed, m_iDelayEffColumns);
		}
		else
			m_cDelayCounter--;
	}
}

void CChannelHandler::UpdateVolumeSlide()
{
	// Volume slide (Axx)
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

void CChannelHandler::LinearAdd(int Step)
{
	m_iPeriod = (m_iPeriod << 5) | m_iPeriodPart;
	int value = (m_iPeriod * Step) / 512;
	if (value == 0)
		value = 1;
	m_iPeriod += value;
	m_iPeriodPart = m_iPeriod & 0x1F;
	m_iPeriod >>= 5;
}

void CChannelHandler::LinearRemove(int Step)
{
	m_iPeriod = (m_iPeriod << 5) | m_iPeriodPart;
	int value = (m_iPeriod * Step) / 512;
	if (value == 0)
		value = 1;
	m_iPeriod -= value;
	m_iPeriodPart = m_iPeriod & 0x1F;
	m_iPeriod >>= 5;
}

void CChannelHandler::PeriodAdd(int Step)
{
	if (m_bLinearPitch)
		LinearAdd(Step);
	else
		SetPeriod(GetPeriod() + Step);
}

void CChannelHandler::PeriodRemove(int Step)
{
	if (m_bLinearPitch)
		LinearRemove(Step);
	else
		SetPeriod(GetPeriod() - Step);
}

void CChannelHandler::UpdateEffects()
{
	// Handle other effects
	switch (m_iEffect) {
		case EF_ARPEGGIO:
			if (m_iArpeggio != 0) {
				switch (m_iArpState) {
					case 0:
						SetPeriod(TriggerNote(m_iNote));
						break;
					case 1:
						SetPeriod(TriggerNote(m_iNote + (m_iArpeggio >> 4)));
						if ((m_iArpeggio & 0x0F) == 0)
							++m_iArpState;
						break;
					case 2:
						SetPeriod(TriggerNote(m_iNote + (m_iArpeggio & 0x0F)));
						break;
				}
				m_iArpState = (m_iArpState + 1) % 3;
			}
			break;
		case EF_PORTAMENTO:
			// Automatic portamento
			if (m_iPortaSpeed > 0 && m_iPortaTo > 0) {
				if (m_iPeriod > m_iPortaTo) {
					PeriodRemove(m_iPortaSpeed);
					if (m_iPeriod < m_iPortaTo)
						SetPeriod(m_iPortaTo);
				}
				else if (m_iPeriod < m_iPortaTo) {
					PeriodAdd(m_iPortaSpeed);
					if (m_iPeriod > m_iPortaTo)
						SetPeriod(m_iPortaTo);
				}
			}
			break;
		case EF_SLIDE_UP:
			if (m_iPortaSpeed > 0) {
				PeriodRemove(m_iPortaSpeed);
				if (m_iPeriod < m_iPortaTo) {
					SetPeriod(m_iPortaTo);
					m_iPortaTo = 0;
					m_iEffect = EF_NONE;
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
		case EF_PORTA_DOWN:
			if (GetPeriod() > 0)
				PeriodAdd(m_iPortaSpeed);
			break;
		case EF_PORTA_UP:
			if (GetPeriod() > 0)
				PeriodRemove(m_iPortaSpeed);
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
}

bool CChannelHandler::IsActive() const
{
	return m_bGate;
}

bool CChannelHandler::IsReleasing() const
{
	return m_bRelease;
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

	if (m_bLinearPitch)
		VibFreq = (GetPeriod() * VibFreq) / 128;

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
	return LimitPeriod(GetPeriod() - GetVibrato() + GetFinePitch() + GetPitch());
}

int CChannelHandler::CalculateVolume(bool Subtract) const
{
	// Volume calculation
	int Volume = m_iVolume >> VOL_COLUMN_SHIFT;
	
	if (m_iChannelID == CHANID_FDS && !theApp.GetSettings()->General.bFDSOldVolume) {		// // // match NSF setting
		if (!m_iSeqVolume || !m_iVolume)
			Volume = 0;
		else
			Volume = (m_iSeqVolume * (Volume + 1)) / 16;
	}
	else if (Subtract)
		Volume = Volume + m_iSeqVolume - 15;
	else
		Volume = (m_iSeqVolume * Volume) / 15;
	Volume -= GetTremolo();
	Volume = std::max(Volume, 0);
	Volume = std::min(Volume, m_iMaxVolume);

	if (m_iSeqVolume > 0 && m_iVolume > 0 && Volume == 0 && !theApp.GetSettings()->General.bCutVolume)		// // //
		Volume = 1;

	if (!m_bGate)
		Volume = 0;

	return Volume;
}

void CChannelHandler::AddCycles(int count)
{
	m_pSoundGen->AddCycles(count);
}

void CChannelHandler::WriteRegister(uint16 Reg, uint8 Value)
{
	m_pAPU->Write(Reg, Value);
	m_pSoundGen->WriteRegister(Reg, Value);
}

void CChannelHandler::WriteExternalRegister(uint16 Reg, uint8 Value)
{
	m_pAPU->ExternalWrite(Reg, Value);
	m_pSoundGen->WriteExternalRegister(Reg, Value);
}

void CChannelHandler::RegisterKeyState(int Note)
{
	m_pSoundGen->RegisterKeyState(m_iChannelID, Note);
}

void CChannelHandler::SetVolume(int Volume)
{
	m_iSeqVolume = Volume;
}

void CChannelHandler::SetPeriod(int Period)
{
	m_iPeriod = LimitPeriod(Period);
	if (m_iChannelID == CHANID_NOISE)		// // //
		m_iPeriod = (m_iPeriod & 0x0F) | 0x100;
	m_bPeriodUpdated = true;
}

int CChannelHandler::GetPeriod() const
{
	return m_iPeriod;
}

void CChannelHandler::SetNote(int Note)
{
	m_iNote = Note;
	if (m_iChannelID == CHANID_NOISE)		// // //
		m_iNote = (m_iNote & 0x0F) | 0x100;
}

int CChannelHandler::GetNote() const
{
	return m_iNote;
}

void CChannelHandler::SetDutyPeriod(int Period)
{
	m_iDutyPeriod = Period;
}

/*
 * Class CChannelHandlerInverted
 *
 */

void CChannelHandlerInverted::SetupSlide(int Type, int EffParam)
{
	CChannelHandler::SetupSlide(Type, EffParam);

	// Invert slide effects
	if (m_iEffect == EF_SLIDE_DOWN)
		m_iEffect = EF_SLIDE_UP;
	else
		m_iEffect = EF_SLIDE_DOWN;
}

int CChannelHandlerInverted::CalculatePeriod() const 
{
	return LimitPeriod(GetPeriod() + GetVibrato() - GetFinePitch() - GetPitch());
}

/*
 * Class CSequenceHandler
 *
 */

CSequenceHandler::CSequenceHandler()
{
	ClearSequences();
}

// Sequence routines

void CSequenceHandler::SetupSequence(int Index, const CSequence *pSequence)
{
	m_iSeqState[Index]	 = SEQ_STATE_RUNNING;
	m_iSeqPointer[Index] = 0;
	m_pSequence[Index]	 = pSequence;
}

void CSequenceHandler::ClearSequence(int Index)
{
	m_iSeqState[Index]	 = SEQ_STATE_DISABLED;
	m_iSeqPointer[Index] = 0;
	m_pSequence[Index]	 = NULL;
}

void CSequenceHandler::UpdateSequenceRunning(int Index, const CSequence *pSequence)
{
	int Value = pSequence->GetItem(m_iSeqPointer[Index]);

	switch (Index) {
		// Volume modifier
		case SEQ_VOLUME:
			SetVolume(Value);
			break;
		// Arpeggiator
		case SEQ_ARPEGGIO:
			switch (pSequence->GetSetting()) {
				case SETTING_ARP_ABSOLUTE:
					SetPeriod(TriggerNote(GetNote() + Value));
					break;
				case SETTING_ARP_FIXED:
					SetPeriod(TriggerNote(Value));
					break;
				case SETTING_ARP_RELATIVE:
					SetNote(GetNote() + Value);
					SetPeriod(TriggerNote(GetNote()));
					break;
					case SETTING_ARP_SCHEME: // // //
						if (Value < 0) Value += 256;
						int lim = Value % 0x40, scheme = Value / 0x40;
						if (lim > 36)
							lim -= 64;
						switch (scheme) {
							case 0:
								break;
							case 1:
								lim += m_iArpeggio >> 4;
								break;
							case 2:
								lim += m_iArpeggio & 0x0F;
								break;
							case 3:			// -y
								lim -= m_iArpeggio & 0x0F;
								break;
						}
						SetPeriod(TriggerNote(GetNote() + lim));
						break;
			}
			break;
		// Pitch
		case SEQ_PITCH:
			SetPeriod(GetPeriod() + Value);
			break;
		// Hi-pitch
		case SEQ_HIPITCH:
			SetPeriod(GetPeriod() + (Value << 4));
			break;
		// Duty cycling
		case SEQ_DUTYCYCLE:
			SetDutyPeriod(Value);
			break;
	}

	++m_iSeqPointer[Index];

	int Release = pSequence->GetReleasePoint();
	int Items = pSequence->GetItemCount();
	int Loop = pSequence->GetLoopPoint();

	if (m_iSeqPointer[Index] == (Release + 1) || m_iSeqPointer[Index] >= Items) {
		// End point reached
		if (Loop != -1 && !(IsReleasing() && Release != -1)) {
			m_iSeqPointer[Index] = Loop;
		}
		else {
			if (m_iSeqPointer[Index] >= Items) {
				// End of sequence 
				m_iSeqState[Index] = SEQ_STATE_END;
			}
			else if (!IsReleasing()) {
				// Waiting for release
				--m_iSeqPointer[Index];
			}
		}
	}

	theApp.GetSoundGenerator()->SetSequencePlayPos(pSequence, m_iSeqPointer[Index]);
}

void CSequenceHandler::UpdateSequenceEnd(int Index, const CSequence *pSequence)
{
	switch (Index) {
		case SEQ_ARPEGGIO:
			if (pSequence->GetSetting() == SETTING_ARP_FIXED) {
				SetPeriod(TriggerNote(GetNote()));
			}
			break;
	}

	m_iSeqState[Index] = SEQ_STATE_HALT;

	theApp.GetSoundGenerator()->SetSequencePlayPos(pSequence, -1);
}

void CSequenceHandler::RunSequence(int Index)
{
	const CSequence *pSequence = m_pSequence[Index];

	if (!pSequence || pSequence->GetItemCount() == 0 || !IsActive())
		return;

	switch (m_iSeqState[Index]) {
		case SEQ_STATE_RUNNING:
			UpdateSequenceRunning(Index, pSequence);
			break;
		case SEQ_STATE_END:
			UpdateSequenceEnd(Index, pSequence);
			break;
		case SEQ_STATE_DISABLED:
		case SEQ_STATE_HALT:
			// Do nothing
			break;
	}
}

void CSequenceHandler::ClearSequences()
{
	for (int i = 0; i < SEQ_COUNT; ++i) {
		m_iSeqState[i]	 = SEQ_STATE_DISABLED;
		m_iSeqPointer[i] = 0;
		m_pSequence[i]	 = NULL;
	}
}

void CSequenceHandler::ReleaseSequences()
{
	for (int i = 0; i < SEQ_COUNT; ++i) {
		if (m_iSeqState[i] == SEQ_STATE_RUNNING || m_iSeqState[i] == SEQ_STATE_END) {
			ReleaseSequence(i, m_pSequence[i]);
		}
	}
}

void CSequenceHandler::ReleaseSequence(int Index, const CSequence *pSeq)
{
	int ReleasePoint = pSeq->GetReleasePoint();

	if (ReleasePoint != -1) {
		m_iSeqPointer[Index] = ReleasePoint;
		m_iSeqState[Index] = SEQ_STATE_RUNNING;
	}
}

bool CSequenceHandler::IsSequenceEqual(int Index, const CSequence *pSequence) const
{
	return pSequence == m_pSequence[Index];
}

seq_state_t CSequenceHandler::GetSequenceState(int Index) const
{
	return m_iSeqState[Index];
}
