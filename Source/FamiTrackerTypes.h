/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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

#include "APU/Types.h"		// // //
#include <array>

/*
 * Here are the constants that defines the limits in the tracker
 * change if needed (some might cause side effects)
 *
 */

// Maximum number of instruments to use
const int MAX_INSTRUMENTS = 64;

// Hold instrument index
const int HOLD_INSTRUMENT = 0xFF;		// // // 050B
// TODO: check if this conflicts with INVALID_INSTRUMENT

// Maximum number of sequence lists
const int MAX_SEQUENCES	= 128;

// Maximum number of items in each sequence
const int MAX_SEQUENCE_ITEMS = /*128*/ 252;		// TODO: need to check if this exports correctly

// Maximum number of patterns per channel
const int MAX_PATTERN = 256;		// // //

// Maximum number of frames
const int MAX_FRAMES = 256;		// // //

// Maximum length of patterns (in rows). 256 is max in NSF
const int MAX_PATTERN_LENGTH = 256;

// Maximum number of DPCM samples, cannot be increased unless the NSF driver is modified.
const int MAX_DSAMPLES = 64;

// Sample space available (from $C000-$FFFF), may now switch banks
const int MAX_SAMPLE_SPACE = 0x40000;	// 256kB

// Number of effect columns allowed
const int MAX_EFFECT_COLUMNS = 4;

// Maximum numbers of tracks allowed (NSF limit is 256, but dunno if the bankswitcher can handle that)
const unsigned int MAX_TRACKS = 64;

// Max tempo
const int MAX_TEMPO	= 255;

// Min tempo
//const int MIN_TEMPO	= 21;

// Max speed
//const int MAX_SPEED = 20;

// Min speed
const int MIN_SPEED = 1;

// // // Maximum number of grooves
const int MAX_GROOVE = 32;

// // // Maximum number of entries in the echo buffer
const int ECHO_BUFFER_LENGTH = 3;

// Number of available channels (max) TODO: should not be used anymore!
// instead, check the channelsavailable variable and allocate dynamically
const int MAX_CHANNELS	 = 5 + 3 + 2 + 6 + 1 + 8 + 3;

const int CHANNELS_DEFAULT = 5;
const int CHANNELS_VRC6	   = 3;
const int CHANNELS_VRC7	   = 6;

const int OCTAVE_RANGE = 8;
const int NOTE_RANGE   = 12;
const int NOTE_COUNT   = OCTAVE_RANGE * NOTE_RANGE;	// // // mvoed from SoundGen.h

const int INVALID_INSTRUMENT = -1;

// Max allowed value in volume column. The actual meaning is no specific volume information, rather than max volume.
const int MAX_VOLUME = 0x10;

// Sequence types (shared with VRC6)

enum sequence_t {
	SEQ_VOLUME,
	SEQ_ARPEGGIO,
	SEQ_PITCH,
	SEQ_HIPITCH,		// TODO: remove this eventually
	SEQ_DUTYCYCLE,

	SEQ_COUNT
};


// Channel effects
enum effect_t : unsigned char {
	EF_NONE = 0,
	EF_SPEED,           	// Speed
	EF_JUMP,            	// Jump
	EF_SKIP,            	// Skip
	EF_HALT,            	// Halt
	EF_VOLUME,          	// Volume
	EF_PORTAMENTO,      	// Porta on
	EF_PORTAOFF,        	// Porta off		// unused
	EF_SWEEPUP,         	// Sweep up
	EF_SWEEPDOWN,       	// Sweep down
	EF_ARPEGGIO,        	// Arpeggio
	EF_VIBRATO,         	// Vibrato
	EF_TREMOLO,         	// Tremolo
	EF_PITCH,           	// Pitch
	EF_DELAY,           	// Note delay
	EF_DAC,             	// DAC setting
	EF_PORTA_UP,        	// Portamento up
	EF_PORTA_DOWN,      	// Portamento down
	EF_DUTY_CYCLE,      	// Duty cycle
	EF_SAMPLE_OFFSET,   	// Sample offset
	EF_SLIDE_UP,        	// Slide up
	EF_SLIDE_DOWN,      	// Slide down
	EF_VOLUME_SLIDE,    	// Volume slide
	EF_NOTE_CUT,        	// Note cut
	EF_RETRIGGER,       	// DPCM retrigger
	EF_DELAYED_VOLUME,  	// // // Delayed channel volume
	EF_FDS_MOD_DEPTH,   	// FDS modulation depth
	EF_FDS_MOD_SPEED_HI,	// FDS modulation speed hi
	EF_FDS_MOD_SPEED_LO,	// FDS modulation speed lo
	EF_DPCM_PITCH,      	// DPCM Pitch
	EF_SUNSOFT_ENV_TYPE,	// Sunsoft envelope type
	EF_SUNSOFT_ENV_HI,  	// Sunsoft envelope high
	EF_SUNSOFT_ENV_LO,  	// Sunsoft envelope low
	EF_SUNSOFT_NOISE,   	// // // 050B Sunsoft noise period
	EF_VRC7_PORT,       	// // // 050B VRC7 custom patch port
	EF_VRC7_WRITE,      	// // // 050B VRC7 custom patch write
	EF_NOTE_RELEASE,    	// // // Delayed release
	EF_GROOVE,          	// // // Groove
	EF_TRANSPOSE,       	// // // Delayed transpose
	EF_N163_WAVE_BUFFER,	// // // N163 wave buffer
	EF_FDS_VOLUME,      	// // // FDS volume envelope
	EF_FDS_MOD_BIAS,    	// // // FDS auto-FM bias
	EF_PHASE_RESET,  // Reset waveform phase without retriggering note (VRC6-only so far)
	EF_HARMONIC,  // Multiply the note pitch by an integer

	EF_COUNT
};

// Note: Order must be preserved.
// Global/2A03 effects should be listed before expansion-specific effects
// sharing the same character.

// const effect_t VRC6_EFFECTS[] = {};
const effect_t VRC7_EFFECTS[] = {EF_VRC7_PORT, EF_VRC7_WRITE};
const effect_t FDS_EFFECTS[] = {EF_FDS_MOD_DEPTH, EF_FDS_MOD_SPEED_HI, EF_FDS_MOD_SPEED_LO, EF_FDS_VOLUME, EF_FDS_MOD_BIAS};
// const effect_t MMC5_EFFECTS[] = {};
const effect_t N163_EFFECTS[] = {EF_N163_WAVE_BUFFER};
const effect_t S5B_EFFECTS[] = {EF_SUNSOFT_ENV_TYPE, EF_SUNSOFT_ENV_HI, EF_SUNSOFT_ENV_LO, EF_SUNSOFT_NOISE};

// Effect checking = bool CTrackerChannel::IsEffectCompatible

// Channel effect letters
const char EFF_CHAR[] = {
	'\xff',	// EF_NONE,
	'F',   	// EF_SPEED,
	'B',   	// EF_JUMP,
	'D',   	// EF_SKIP,
	'C',   	// EF_HALT,
	'E',   	// EF_VOLUME,
	'3',   	// EF_PORTAMENTO,
	'\xff',	// EF_PORTAOFF,
	'H',   	// EF_SWEEPUP,
	'I',   	// EF_SWEEPDOWN,
	'0',   	// EF_ARPEGGIO,
	'4',   	// EF_VIBRATO,
	'7',   	// EF_TREMOLO,
	'P',   	// EF_PITCH,
	'G',   	// EF_DELAY,
	'Z',   	// EF_DAC,
	'1',   	// EF_PORTA_UP,
	'2',   	// EF_PORTA_DOWN,
	'V',   	// EF_DUTY_CYCLE,
	'Y',   	// EF_SAMPLE_OFFSET,
	'Q',   	// EF_SLIDE_UP,
	'R',   	// EF_SLIDE_DOWN,
	'A',   	// EF_VOLUME_SLIDE,
	'S',   	// EF_NOTE_CUT,
	'X',   	// EF_RETRIGGER,
	'M',   	// EF_DELAYED_VOLUME,
	'H',   	// EF_FDS_MOD_DEPTH,
	'I',   	// EF_FDS_MOD_SPEED_HI,
	'J',   	// EF_FDS_MOD_SPEED_LO,
	'W',   	// EF_DPCM_PITCH,
	'H',   	// EF_SUNSOFT_ENV_TYPE,
	'I',   	// EF_SUNSOFT_ENV_HI,
	'J',   	// EF_SUNSOFT_ENV_LO,
	'W',   	// EF_SUNSOFT_NOISE,
	'H',   	// EF_VRC7_PORT,
	'I',   	// EF_VRC7_WRITE,
	'L',   	// EF_NOTE_RELEASE,
	'O',   	// EF_GROOVE,
	'T',   	// EF_TRANSPOSE,
	'Z',   	// EF_N163_WAVE_BUFFER,
	'E',   	// EF_FDS_VOLUME,
	'Z',   	// EF_FDS_MOD_BIAS,
	'=',	// EF_PHASE_RESET
	'K',	// EF_HARMONIC
};

struct Effect {
	char eff_char;
	int initial = 0x00;
	int uiDefault = 0x00;
};

const extern std::array<Effect, EF_COUNT> effects;

effect_t GetEffectFromChar(char ch, int Chip, bool *bValid = nullptr);		// // //

enum note_t : unsigned char {
	NONE = 0,	// No note
	NOTE_C,  NOTE_Cs, NOTE_D,  NOTE_Ds, NOTE_E,  NOTE_F,		// // // renamed
	NOTE_Fs, NOTE_G,  NOTE_Gs, NOTE_A,  NOTE_As, NOTE_B,
	RELEASE,	// Release, begin note release sequence
	HALT,		// Halt, stops note
	ECHO,		// // // Echo buffer access, octave determines position
};

// // // special echo buffer constants
const char ECHO_BUFFER_NONE = '\xFF';
const char ECHO_BUFFER_HALT = '\x7F';
const char ECHO_BUFFER_ECHO = '\x80';

enum machine_t {
	NTSC,
	PAL
};

enum vibrato_t : unsigned char {
	VIBRATO_OLD = 0,
	VIBRATO_NEW,
};

inline int MIDI_NOTE(int octave, int note)		// // //
{
	return octave * NOTE_RANGE + note - 1;
}

inline int GET_OCTAVE(int midi_note)
{
	int x = midi_note / NOTE_RANGE;
	if (midi_note < 0 && (midi_note % NOTE_RANGE)) --x;
	return x;
}

inline int GET_NOTE(int midi_note)
{
	int x = midi_note % NOTE_RANGE;
	if (x < 0) x += NOTE_RANGE;
	return ++x;
}
