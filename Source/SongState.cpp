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

#include "SongState.h"
#include "FamiTrackerDoc.h"
#include "TrackerChannel.h"
#include "Groove.h"
#include <algorithm>



namespace {

void UpdateEchoTranspose(const stChanNote &Note, int &Value, unsigned int EffColumns) {
	for (int j = EffColumns; j >= 0; --j) {
		const int Param = Note.EffParam[j] & 0x0F;
		switch (Note.EffNumber[j]) {
		case EF_SLIDE_UP:
			Value += Param; return;
		case EF_SLIDE_DOWN:
			Value -= Param; return;
		case EF_TRANSPOSE: // Sometimes there are not enough ticks for the transpose to take place
			if (Note.EffParam[j] & 0x80)
				Value -= Param;
			else
				Value += Param;
			return;
		}
	}
}

} // namespace



std::string MakeCommandString(effect_t Effect, unsigned char Param) {		// // //
	return {' ', EFF_CHAR[Effect - 1], hex(Param >> 4), hex(Param)};
}



stChannelState::stChannelState()
{
	memset(Effect, -1, EF_COUNT * sizeof(int));
	memset(Echo, -1, ECHO_BUFFER_LENGTH * sizeof(int));
}

std::string stChannelState::GetStateString() const {
	std::string log("Inst.: ");
	if (Instrument == MAX_INSTRUMENTS)
		log += "None";
	else
		log += {hex(Instrument >> 4), hex(Instrument)};
	log += "        Vol.: ";
	log += hex(Volume >= MAX_VOLUME ? 0xF : Volume);
	log += "        Active effects:";

	std::string effStr;

	const effect_t SLIDE_EFFECT = Effect[EF_ARPEGGIO] >= 0 ? EF_ARPEGGIO :
		Effect[EF_PORTA_UP] >= 0 ? EF_PORTA_UP :
		Effect[EF_PORTA_DOWN] >= 0 ? EF_PORTA_DOWN :
		EF_PORTAMENTO;
	for (const auto &x : {SLIDE_EFFECT, EF_VIBRATO, EF_TREMOLO, EF_VOLUME_SLIDE, EF_PITCH, EF_DUTY_CYCLE}) {
		int p = Effect[x];
		if (p < 0) continue;
		if (p == 0 && x != EF_PITCH) continue;
		if (p == 0x80 && x == EF_PITCH) continue;
		effStr += MakeCommandString(x, p);
	}

	if ((ChannelIndex >= CHANID_SQUARE1 && ChannelIndex <= CHANID_SQUARE2) ||
		ChannelIndex == CHANID_NOISE ||
		(ChannelIndex >= CHANID_MMC5_SQUARE1 && ChannelIndex <= CHANID_MMC5_SQUARE2))
		for (const auto &x : {EF_VOLUME}) {
			int p = Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelIndex == CHANID_TRIANGLE)
		for (const auto &x : {EF_VOLUME, EF_NOTE_CUT}) {
			int p = Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelIndex == CHANID_DPCM)
		for (const auto &x : {EF_SAMPLE_OFFSET, /*EF_DPCM_PITCH*/}) {
			int p = Effect[x];
			if (p <= 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelIndex >= CHANID_VRC7_CH1 && ChannelIndex <= CHANID_VRC7_CH6)
		for (const auto &x : VRC7_EFFECTS) {
			int p = Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelIndex == CHANID_FDS)
		for (const auto &x : FDS_EFFECTS) {
			int p = Effect[x];
			if (p < 0 || (x == EF_FDS_MOD_BIAS && p == 0x80)) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelIndex >= CHANID_S5B_CH1 && ChannelIndex <= CHANID_S5B_CH3)
		for (const auto &x : S5B_EFFECTS) {
			int p = Effect[x];
			if (p < 0) continue;
			effStr += MakeCommandString(x, p);
		}
	else if (ChannelIndex >= CHANID_N163_CH1 && ChannelIndex <= CHANID_N163_CH8)
		for (const auto &x : N163_EFFECTS) {
			int p = Effect[x];
			if (p < 0 || (x == EF_N163_WAVE_BUFFER && p == 0x7F)) continue;
			effStr += MakeCommandString(x, p);
		}
	if (Effect_LengthCounter >= 0)
		effStr += MakeCommandString(EF_VOLUME, Effect_LengthCounter);
	if (Effect_AutoFMMult >= 0)
		effStr += MakeCommandString(EF_FDS_MOD_DEPTH, Effect_AutoFMMult);

	if (!effStr.size()) effStr = " None";
	log += effStr;
	return log;
}



CSongState::CSongState(int Count) :
	State(std::make_unique<stChannelState[]>(Count))
{
}

void CSongState::Retrieve(const CFamiTrackerDoc &doc, unsigned Track, unsigned Frame, unsigned Row) {
	const int Chans = doc.GetChannelCount();
	
	int totalRows = 0;
	auto BufferPos = std::make_unique<int[]>(Chans);
	auto Transpose = std::make_unique<int[][ECHO_BUFFER_LENGTH + 1]>(Chans);

	bool maskFDS = false; // no need to create per-channel array since only one FDS channel exists
						  // may not be the case in future additions

	for (int c = 0; c < Chans; ++c) {
		for (int i = 0; i <= ECHO_BUFFER_LENGTH; ++i)
			Transpose[c][i] = -1;
		State[c].ChannelIndex = doc.GetChannelType(c);
		// State[c].Mute = CFamiTrackerView::GetView()->IsChannelMuted(i);
	}

	while (true) {
		for (int c = Chans - 1; c >= 0; c--) {
			// if (Channel != -1) c = GetChannelIndex(Channel);
			stChannelState &chState = State[c];
			int EffColumns = doc.GetEffColumns(Track, c);
			const auto &Note = doc.GetNoteData(Track, Frame, c, Row);		// // //
		
			if (Note.Note != NONE && Note.Note != RELEASE) {
				for (int i = 0; i < std::min(BufferPos[c], ECHO_BUFFER_LENGTH + 1); i++) {
					if (chState.Echo[i] == ECHO_BUFFER_ECHO) {
						UpdateEchoTranspose(Note, Transpose[c][i], EffColumns);
						switch (Note.Note) {
						case HALT: chState.Echo[i] = ECHO_BUFFER_HALT; break;
						case ECHO: chState.Echo[i] = ECHO_BUFFER_ECHO + Note.Octave; break;
						default:
							int NewNote = MIDI_NOTE(Note.Octave, Note.Note) + Transpose[c][i];
							NewNote = std::max(std::min(NewNote, NOTE_COUNT - 1), 0);
							chState.Echo[i] = NewNote;
						}
					}
					else if (chState.Echo[i] > ECHO_BUFFER_ECHO && chState.Echo[i] <= ECHO_BUFFER_ECHO + ECHO_BUFFER_LENGTH)
						chState.Echo[i]--;
				}
				if (BufferPos[c] >= 0 && BufferPos[c] <= ECHO_BUFFER_LENGTH) {
					// WriteEchoBuffer(&Note, BufferPos, EffColumns);
					int Value;
					switch (Note.Note) {
					case HALT: Value = ECHO_BUFFER_HALT; break;
					case ECHO: Value = ECHO_BUFFER_ECHO + Note.Octave; break;
					default:
						Value = MIDI_NOTE(Note.Octave, Note.Note);
						UpdateEchoTranspose(Note, Value, EffColumns);
						Value = std::max(std::min(Value, NOTE_COUNT - 1), 0);
					}
					chState.Echo[BufferPos[c]] = Value;
					UpdateEchoTranspose(Note, Transpose[c][BufferPos[c]], EffColumns);
				}
				BufferPos[c]++;
			}
			if (BufferPos[c] < 0)
				BufferPos[c] = 0;

			if (chState.Instrument == MAX_INSTRUMENTS)
				if (Note.Instrument != MAX_INSTRUMENTS && Note.Instrument != HOLD_INSTRUMENT)		// // // 050B
					chState.Instrument = Note.Instrument;

			if (chState.Volume == MAX_VOLUME)
				if (Note.Vol != MAX_VOLUME)
					chState.Volume = Note.Vol;
		
			CTrackerChannel &ch = doc.GetChannel(c);
			for (int k = EffColumns; k >= 0; k--) {
				unsigned char fx = Note.EffNumber[k], xy = Note.EffParam[k];
				switch (fx) {
				// ignore effects that cannot have memory
				case EF_NONE: case EF_PORTAOFF:
				case EF_DAC: case EF_DPCM_PITCH: case EF_RETRIGGER:
				case EF_DELAY: case EF_DELAYED_VOLUME: case EF_NOTE_RELEASE: case EF_TRANSPOSE:
				case EF_JUMP: case EF_SKIP: // no true backward iterator
					continue;
				case EF_HALT:
					Row = Frame = 0; goto outer;
				case EF_SPEED:
					if (Speed == -1 && (xy < doc.GetSpeedSplitPoint() || doc.GetSongTempo(Track) == 0)) {
						Speed = xy; if (Speed < 1) Speed = 1;
						GroovePos = -2;
					}
					else if (Tempo == -1 && xy >= doc.GetSpeedSplitPoint()) Tempo = xy;
					continue;
				case EF_GROOVE:
					if (GroovePos == -1 && xy < MAX_GROOVE && doc.GetGroove(xy)) {
						GroovePos = totalRows;
						Speed = xy;
					}
					continue;
				case EF_VOLUME:
					if (!ch.IsEffectCompatible(fx, xy)) continue;
					if (chState.Effect_LengthCounter == -1 && xy >= 0xE0 && xy <= 0xE3)
						chState.Effect_LengthCounter = xy;
					else if (chState.Effect[fx] == -1 && xy <= 0x1F) {
						chState.Effect[fx] = xy;
						if (chState.Effect_LengthCounter == -1)
							chState.Effect_LengthCounter = ch.GetID() == CHANID_TRIANGLE ? 0xE1 : 0xE2;
					}
					continue;
				case EF_NOTE_CUT:
					if (!ch.IsEffectCompatible(fx, xy)) continue;
					if (ch.GetID() != CHANID_TRIANGLE) continue;
					if (chState.Effect[fx] == -1) {
						if (xy <= 0x7F) {
							if (chState.Effect_LengthCounter == -1)
								chState.Effect_LengthCounter = 0xE0;
							continue;
						}
						if (chState.Effect_LengthCounter != 0xE0) {
							chState.Effect[fx] = xy;
							if (chState.Effect_LengthCounter == -1) chState.Effect_LengthCounter = 0xE1;
						}
					}
					continue;
				case EF_FDS_MOD_DEPTH:
					if (!ch.IsEffectCompatible(fx, xy)) continue;
					if (chState.Effect_AutoFMMult == -1 && xy >= 0x80)
						chState.Effect_AutoFMMult = xy;
					continue;
				case EF_FDS_MOD_SPEED_HI:
					if (!ch.IsEffectCompatible(fx, xy)) continue;
					if (xy <= 0x0F)
						maskFDS = true;
					else if (!maskFDS && chState.Effect[fx] == -1) {
						chState.Effect[fx] = xy;
						if (chState.Effect_AutoFMMult == -1) chState.Effect_AutoFMMult = -2;
					}
					continue;
				case EF_FDS_MOD_SPEED_LO:
					if (!ch.IsEffectCompatible(fx, xy)) continue;
					maskFDS = true;
					continue;
				case EF_SAMPLE_OFFSET:
				case EF_FDS_VOLUME: case EF_FDS_MOD_BIAS:
				case EF_SUNSOFT_ENV_LO: case EF_SUNSOFT_ENV_HI: case EF_SUNSOFT_ENV_TYPE:
				case EF_N163_WAVE_BUFFER:
				case EF_VRC7_PORT:
					if (!ch.IsEffectCompatible(fx, xy)) continue;
				case EF_DUTY_CYCLE:
					if (ch.GetChip() == SNDCHIP_VRC7) continue;		// // // 050B
				case EF_VIBRATO: case EF_TREMOLO: case EF_PITCH: case EF_VOLUME_SLIDE:
					if (chState.Effect[fx] == -1)
						chState.Effect[fx] = xy;
					continue;

				case EF_SWEEPUP: case EF_SWEEPDOWN: case EF_SLIDE_UP: case EF_SLIDE_DOWN:
				case EF_PORTAMENTO: case EF_ARPEGGIO: case EF_PORTA_UP: case EF_PORTA_DOWN:
					if (chState.Effect[EF_PORTAMENTO] == -1) { // anything else within can be used here
						chState.Effect[EF_PORTAMENTO] = fx == EF_PORTAMENTO ? xy : -2;
						chState.Effect[EF_ARPEGGIO] = fx == EF_ARPEGGIO ? xy : -2;
						chState.Effect[EF_PORTA_UP] = fx == EF_PORTA_UP ? xy : -2;
						chState.Effect[EF_PORTA_DOWN] = fx == EF_PORTA_DOWN ? xy : -2;
					}
					continue;
				}
			}
			// if (Channel != -1) break;
		}
	outer:
		if (Row) Row--;
		else if (Frame) Row = doc.GetFrameLength(Track, --Frame) - 1;
		else break;
		totalRows++;
	}
	if (GroovePos == -1 && doc.GetSongGroove(Track)) {
		unsigned Index = doc.GetSongSpeed(Track);
		if (Index < MAX_GROOVE && doc.GetGroove(Index)) {
			GroovePos = totalRows;
			Speed = Index;
		}
	}
}

std::string CSongState::GetChannelStateString(const CFamiTrackerDoc &doc, int chan) const {
	std::string str = State[doc.GetChannelIndex(chan)].GetStateString();
	if (Tempo >= 0)
		str += "        Tempo: " + std::to_string(Tempo);
	if (Speed >= 0) {
		if (const CGroove *Groove = doc.GetGroove(Speed); Groove && GroovePos >= 0) {
			str += "        Groove: ";
			str += {hex(Speed >> 4), hex(Speed)};
			const unsigned char Size = Groove->GetSize();
			for (unsigned char i = 0; i < Size; i++)
				str += ' ' + std::to_string(Groove->GetEntry((i + GroovePos) % Size));
		}
		else
			str += "        Speed: " + std::to_string(Speed);
	}

	return str;
}
