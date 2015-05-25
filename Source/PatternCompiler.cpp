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

#include <vector>
#include "stdafx.h"
#include "FamiTrackerDoc.h"
#include "PatternCompiler.h"
#include "TrackerChannel.h"
#include "Compiler.h"

/**
 * CPatternCompiler - Compress patterns to strings for the NSF code
 *
 */

/*

 Pattern byte layout: 

 00h - 7Fh : Notes, where 00h = rest, 7Fh = Note cut
 80h - DFh : Commands, defined in the command table
 E0h - EFh : Quick instrument switches, E0h = instrument 0, EFh = instrument 15
 F0h - FFh : Volume changes, F0h = volume 0, FFh = volume 15

 Each row entry is ended by a note and the duration of the note,
 if fixed duration is enabled then duration is omitted.

*/

// Optimize note durations when possible (default on)
#define OPTIMIZE_DURATIONS

// Use single-byte instrument commands for instrument 0-15 (default on)
#define PACKED_INST_CHANGE

// Command table
enum command_t {
	CMD_INSTRUMENT,
	CMD_SET_DURATION,
	CMD_RESET_DURATION,
	CMD_EFF_SPEED,
	CMD_EFF_TEMPO,
	CMD_EFF_JUMP,
	CMD_EFF_SKIP,
	CMD_EFF_HALT,
	CMD_EFF_VOLUME,
	CMD_EFF_CLEAR,
	CMD_EFF_PORTAUP,
	CMD_EFF_PORTADOWN,
	CMD_EFF_PORTAMENTO,
	CMD_EFF_ARPEGGIO,
	CMD_EFF_VIBRATO,
	CMD_EFF_TREMOLO,
	CMD_EFF_PITCH,
	CMD_EFF_RESET_PITCH,
	CMD_EFF_DUTY,
	CMD_EFF_DELAY,
	CMD_EFF_SWEEP,
	CMD_EFF_DAC,
	CMD_EFF_OFFSET,
	CMD_EFF_SLIDE_UP,
	CMD_EFF_SLIDE_DOWN,
	CMD_EFF_VOL_SLIDE,
	CMD_EFF_NOTE_CUT,
	CMD_EFF_RETRIGGER,
	CMD_EFF_DPCM_PITCH,
	CMD_EFF_NOTE_RELEASE,		// // //
	CMD_EFF_LINEAR_COUNTER,		// // //
	CMD_EFF_GROOVE,				// // //
	CMD_EFF_DELAYED_VOLUME,		// // //
	CMD_EFF_TRANSPOSE,			// // //
	CMD_EFF_FDS_MOD_DEPTH,
	CMD_EFF_FDS_MOD_RATE_HI,
	CMD_EFF_FDS_MOD_RATE_LO,
	CME_EFF_FDS_VOLUME,			// // //
	CME_EFF_N163_FINE_PITCH,	// // //
	CMD_EFF_S5B_ENV_TYPE,		// // //
	CMD_EFF_S5B_ENV_RATE_HI,	// // //
	CMD_EFF_S5B_ENV_RATE_LO		// // //
};

/*
const unsigned char CMD_EFF_VRC7_PATCH = CMD_EFF_FDS_MOD_DEPTH;	// TODO: hack, fix this
*/

const unsigned char CMD_LOOP_POINT = 26;	// Currently unused

CPatternCompiler::CPatternCompiler(CFamiTrackerDoc *pDoc, unsigned int *pInstList, DPCM_List_t *pDPCMList, CCompilerLog *pLogger) :
	m_pDocument(pDoc),
	m_pInstrumentList(pInstList),
	m_pDPCMList(pDPCMList),
	m_pLogger(pLogger)
{
	memset(m_bDSamplesAccessed, 0, sizeof(bool) * MAX_DSAMPLES);
}

CPatternCompiler::~CPatternCompiler()
{
}

void CPatternCompiler::CompileData(int Track, int Pattern, int Channel)
{
	int EffColumns = m_pDocument->GetEffColumns(Track, Channel) + 1;

	stSpacingInfo SpaceInfo;
	stChanNote ChanNote;

	// Global init
	m_iHash = 0;
	m_iDuration = 0;
	m_iCurrentDefaultDuration = 0xFF;

	m_vData.clear();
	m_vCompressedData.clear();

	// Local init
	unsigned int iPatternLen = m_pDocument->GetPatternLength(Track);
	unsigned char LastInstrument = MAX_INSTRUMENTS + 1;
	unsigned char DPCMInst = 0;
	unsigned char NESNote = 0;

	for (unsigned int i = 0; i < iPatternLen; ++i) {

		m_pDocument->GetDataAtPattern(Track, Pattern, Channel, i, &ChanNote);

		unsigned char Note = ChanNote.Note;
		unsigned char Octave = ChanNote.Octave;
		unsigned char Instrument = FindInstrument(ChanNote.Instrument);
		unsigned char Volume = ChanNote.Vol;
		
		bool Action = false;

		CTrackerChannel *pTrackerChannel = m_pDocument->GetChannel(Channel);
		int ChanID = pTrackerChannel->GetID();
		int ChipID = pTrackerChannel->GetChip();

		if (ChanNote.Instrument != MAX_INSTRUMENTS && Note != HALT && Note != NONE && Note != RELEASE) {
			if (!pTrackerChannel->IsInstrumentCompatible(ChanNote.Instrument, m_pDocument)) {
				CString str;
				str.Format(_T("Error: Missing or incompatible instrument (on row %i, channel %i, pattern %i)\n"), i, Channel, Pattern);
				Print(str);
			}
		}

		// Check for delays, must come first
		for (int j = 0; j < EffColumns; ++j) {
			unsigned char Effect   = ChanNote.EffNumber[j];
			unsigned char EffParam = ChanNote.EffParam[j];
			if (Effect == EF_DELAY && EffParam > 0) {
				WriteDuration();
				for (int k = 0; k < EffColumns; ++k) {
					// Clear skip and jump commands on delayed rows
					if (ChanNote.EffNumber[k] == EF_SKIP) {
						WriteData(Command(CMD_EFF_SKIP));
						WriteData(ChanNote.EffParam[k] + 1);
						ChanNote.EffNumber[k] = 0;
					}
					else if (ChanNote.EffNumber[k] == EF_JUMP) {
						WriteData(Command(CMD_EFF_JUMP));
						WriteData(ChanNote.EffParam[k] + 1);
						ChanNote.EffNumber[k] = 0;
					}
				}
				Action = true;
				WriteData(Command(CMD_EFF_DELAY));
				WriteData(EffParam);
			}
		}

#ifdef OPTIMIZE_DURATIONS

		// Determine length of space between notes
		SpaceInfo = ScanNoteLengths(Track, i, Pattern, Channel);

		if (SpaceInfo.SpaceCount > 2) {
			if (SpaceInfo.SpaceSize != m_iCurrentDefaultDuration && SpaceInfo.SpaceCount != 0xFF) {
				// Enable compressed durations
				WriteDuration();
				m_iCurrentDefaultDuration = SpaceInfo.SpaceSize;
				WriteData(Command(CMD_SET_DURATION));
				WriteData(m_iCurrentDefaultDuration);
			}
		}
		else {
			if (m_iCurrentDefaultDuration != 0xFF && m_iCurrentDefaultDuration != SpaceInfo.SpaceSize) {
				// Disable compressed durations
				WriteDuration();
				m_iCurrentDefaultDuration = 0xFF;
				WriteData(Command(CMD_RESET_DURATION));
			}
		}
		
#endif /* OPTIMIZE_DURATIONS */
/*
		if (SpaceInfo.SpaceCount > 2 && SpaceInfo.SpaceSize != CurrentDefaultDuration) {
			CurrentDefaultDuration = SpaceInfo.SpaceSize;
			WriteData(CMD_SET_DURATION);
			WriteData(CurrentDefaultDuration);
		}
		else if (SpaceInfo.SpaceCount < 2 && SpaceInfo.SpaceSize == CurrentDefaultDuration) {
		}
		else 
*/
		if (Instrument != LastInstrument && Instrument < MAX_INSTRUMENTS && Note != HALT && Note != RELEASE) {		// // //

			LastInstrument = Instrument;
			// Write instrument change command
			//if (Channel < InstrChannels) {
			if (ChanID != CHANID_DPCM) {		// Skip DPCM
				WriteDuration();
#ifdef PACKED_INST_CHANGE
				if (Instrument < 0x10) {
					WriteData(0xE0 | Instrument);
				}
				else {
					WriteData(Command(CMD_INSTRUMENT));
					WriteData(Instrument << 1);
				}
#else
				WriteData(Command(CMD_INSTRUMENT));
				WriteData(Instrument << 1);
#endif /* PACKED_INST_CHANGE */
				Action = true;
			}
			else {
				DPCMInst = ChanNote.Instrument;
			}
		}
#ifdef OPTIMIZE_DURATIONS
		else if (Instrument == LastInstrument && Instrument < MAX_INSTRUMENTS) {		// // //
			if (ChanID != CHANID_DPCM) {
				WriteDuration();
				Action = true;
			}
		}
#endif /* OPTIMIZE_DURATIONS */

		if (Note == 0) {
			NESNote = 0xFF;
		}
		else if (Note == HALT) {
			NESNote = 0x7F - 1;
		}
		else if (Note == RELEASE) {
			NESNote = 0x7F - 2;
		}
		else if (Note == ECHO) {		// // //
			NESNote = 0x6F + Octave;
		}
		else {
			if (ChanID == CHANID_DPCM) {
				// 2A03 DPCM
				int LookUp = FindSample(DPCMInst, Octave, Note);
				if (LookUp > 0) {
					NESNote = LookUp - 1;
					CInstrument2A03 *pInstrument = static_cast<CInstrument2A03*>(m_pDocument->GetInstrument(DPCMInst));
					if (pInstrument != NULL) {
						if (pInstrument->GetType() == INST_2A03) {
							int Sample = pInstrument->GetSample(Octave, Note - 1) - 1;
							m_bDSamplesAccessed[Sample] = true;
						}
						pInstrument->Release();
					}
					// TODO: Print errors if incompatible or non-existing instrument is found
				}
				else {
					NESNote = 0xFF;		// Invalid sample, skip
					CString str;
					str.Format(_T("Error: Missing DPCM sample (on row %i, channel %i, pattern %i)\n"), i, Channel, Pattern);
					Print(str);
				}
			}
			else if (ChanID == CHANID_NOISE) {
				// 2A03 Noise
				NESNote = (Note - 1) + (Octave * NOTE_RANGE);
				NESNote = (NESNote & 0x0F) | 0x10;
			}
			else
				// All other channels
				NESNote = (Note - 1) + (Octave * NOTE_RANGE);
		}

		for (int j = 0; j < EffColumns; ++j) {

			unsigned char Effect   = ChanNote.EffNumber[j];
			unsigned char EffParam = ChanNote.EffParam[j];
			
			if (Effect > 0) {
				WriteDuration();
				Action = true;
			}

			switch (Effect) {
				case EF_SPEED:
					if (EffParam >= m_pDocument->GetSpeedSplitPoint() && m_pDocument->GetSongTempo(Track))		// // //
						WriteData(Command(CMD_EFF_TEMPO));
					else
						WriteData(Command(CMD_EFF_SPEED));
					WriteData(EffParam ? EffParam : 1); // NSF halts if 0 is exported
					break;
				case EF_JUMP:
					WriteData(Command(CMD_EFF_JUMP));
					WriteData(EffParam + 1);
					break;
				case EF_SKIP:
					WriteData(Command(CMD_EFF_SKIP));
					WriteData(EffParam + 1);
					break;
				case EF_HALT:
					WriteData(Command(CMD_EFF_HALT));
					WriteData(EffParam);
					break;
				case EF_VOLUME:		// // //
					switch (ChanID) {
					case CHANID_SQUARE1: case CHANID_SQUARE2: case CHANID_TRIANGLE: case CHANID_NOISE:
					case CHANID_MMC5_SQUARE1: case CHANID_MMC5_SQUARE2:
						WriteData(Command(CMD_EFF_VOLUME));
						if ((EffParam <= 0x1F && ChanID != CHANID_TRIANGLE) || (EffParam >= 0xE0 && EffParam <= 0xE3))
							WriteData(EffParam & 0x9F);
					}
					break;
				case EF_PORTAMENTO:
					if (ChanID != CHANID_DPCM) {
						if (EffParam == 0)
							WriteData(Command(CMD_EFF_CLEAR));
						else {
							WriteData(Command(CMD_EFF_PORTAMENTO));
							WriteData(EffParam);
						}
					}
					break;
				case EF_PORTA_UP:
					if (ChanID != CHANID_DPCM) {
						if (EffParam == 0)
							WriteData(Command(CMD_EFF_CLEAR));
						else {
							if (ChipID == SNDCHIP_FDS || ChipID == SNDCHIP_VRC7 || ChipID == SNDCHIP_N163)
								WriteData(Command(CMD_EFF_PORTADOWN));	// Pitch is inverted for these chips
							else
								WriteData(Command(CMD_EFF_PORTAUP));
							WriteData(EffParam);
						}
					}
					break;
				case EF_PORTA_DOWN:
					if (ChanID != CHANID_DPCM) {
						if (EffParam == 0)
							WriteData(Command(CMD_EFF_CLEAR));
						else {
							if (ChipID == SNDCHIP_FDS || ChipID == SNDCHIP_VRC7 || ChipID == SNDCHIP_N163)
								WriteData(Command(CMD_EFF_PORTAUP));
							else
								WriteData(Command(CMD_EFF_PORTADOWN));
							WriteData(EffParam);
						}
					}
					break;
					/*
				case EF_PORTAOFF:
					if (Channel < 5) {
						WriteData(CMD_EFF_PORTAOFF);
						//WriteData(EffParam);
					}
					break;*/
				case EF_SWEEPUP:
					if (ChanID < CHANID_TRIANGLE) {
						WriteData(Command(CMD_EFF_SWEEP));
						WriteData(0x88 | (EffParam & 0x77));	// Calculate sweep
					}
					break;
				case EF_SWEEPDOWN:
					if (ChanID < CHANID_TRIANGLE) {
						WriteData(Command(CMD_EFF_SWEEP));
						WriteData(0x80 | (EffParam & 0x77));	// Calculate sweep
					}
					break;
				case EF_ARPEGGIO:
					if (ChanID != CHANID_DPCM) {
						if (EffParam == 0)
							WriteData(Command(CMD_EFF_CLEAR));
						else {
							WriteData(Command(CMD_EFF_ARPEGGIO));
							WriteData(EffParam);
						}
					}
					break;
				case EF_VIBRATO:
					if (ChanID != CHANID_DPCM) {
						WriteData(Command(CMD_EFF_VIBRATO));
						//WriteData(EffParam);
						WriteData((EffParam & 0xF) << 4 | (EffParam >> 4));
					}
					break;
				case EF_TREMOLO:
					if (ChanID != CHANID_DPCM) {
						WriteData(Command(CMD_EFF_TREMOLO));
//						WriteData(EffParam & 0xF7);
						WriteData((EffParam & 0xF) << 4 | (EffParam >> 4));
					}
					break;
				case EF_PITCH:
					if (ChanID != CHANID_DPCM) {
						if (EffParam == 0x80)
							WriteData(Command(CMD_EFF_RESET_PITCH));
						else {
							switch (ChipID) {
								case SNDCHIP_VRC7:
								case SNDCHIP_FDS:
								case SNDCHIP_N163:
									EffParam = (char)(256 - (int)EffParam);
									if (EffParam == 0)
										EffParam = 0xFF;
									break;
							}
							WriteData(Command(CMD_EFF_PITCH));
							WriteData(EffParam);
						}
					}
					break;
				case EF_DAC:
					if (ChanID == CHANID_DPCM) {
						WriteData(Command(CMD_EFF_DAC));
						WriteData(EffParam & 0x7F);
					}
					break;
				case EF_DUTY_CYCLE:
					if (ChipID == SNDCHIP_VRC7) {
//						WriteData(CMD_EFF_VRC7_PATCH);
//						WriteData(EffParam << 4);
					}
					else if (ChanID != CHANID_TRIANGLE && ChanID != CHANID_DPCM) {	// Not triangle and dpcm
						WriteData(Command(CMD_EFF_DUTY));
						WriteData(EffParam);
					}
					break;
				case EF_SAMPLE_OFFSET:
					if (ChanID == CHANID_DPCM) {	// DPCM
						WriteData(Command(CMD_EFF_OFFSET));
						WriteData(EffParam);
					}
					break;
				case EF_SLIDE_UP:
					if (ChanID != CHANID_DPCM) {
						WriteData(Command(CMD_EFF_SLIDE_UP));
						WriteData(EffParam);
					}
					break;
				case EF_SLIDE_DOWN:
					if (ChanID != CHANID_DPCM) {
						WriteData(Command(CMD_EFF_SLIDE_DOWN));
						WriteData(EffParam);
					}
					break;
				case EF_VOLUME_SLIDE:
					if (ChanID != CHANID_DPCM) {
						WriteData(Command(CMD_EFF_VOL_SLIDE));
						WriteData(EffParam);
					}
					break;
				case EF_NOTE_CUT:
					if (EffParam >= 0x80 && ChanID == CHANID_TRIANGLE) {		// // //
						WriteData(Command(CMD_EFF_LINEAR_COUNTER));
						WriteData(EffParam - 0x80);
					}
					else {
						WriteData(Command(CMD_EFF_NOTE_CUT));
						WriteData(EffParam);
					}
					break;
				case EF_RETRIGGER:
					if (ChanID == CHANID_DPCM) {
						WriteData(Command(CMD_EFF_RETRIGGER));
						WriteData(EffParam + 1);
					}
					break;
				case EF_DPCM_PITCH:
					if (ChanID == CHANID_DPCM) {
						WriteData(Command(CMD_EFF_DPCM_PITCH));
						WriteData(EffParam);
					}
					break;
				case EF_NOTE_RELEASE:		// // //
					if (EffParam < 0x80) {
						WriteData(Command(CMD_EFF_NOTE_RELEASE));
						WriteData(EffParam);
					}
					break;
				case EF_GROOVE:		// // //
					if (EffParam < MAX_GROOVE) {
						WriteData(Command(CMD_EFF_GROOVE));

						int Pos = 1;
						for (int i = 0; i < EffParam; i++)
							if (m_pDocument->GetGroove(i) != NULL)
								Pos += m_pDocument->GetGroove(i)->GetSize() + 2;
						WriteData(Pos);
					}
					break;
				case EF_DELAYED_VOLUME:		// // //
					if (ChanID != CHANID_DPCM && (EffParam >> 4) && (EffParam & 0x0F)) {
						WriteData(Command(CMD_EFF_DELAYED_VOLUME));
						WriteData(EffParam);
					}
					break;
				case EF_TRANSPOSE:			// // //
					if (ChanID != CHANID_DPCM) {
						WriteData(Command(CMD_EFF_TRANSPOSE));
						WriteData(EffParam);
					}
					break;
				// FDS
				case EF_FDS_MOD_DEPTH:
					if (ChanID == CHANID_FDS) {
						WriteData(Command(CMD_EFF_FDS_MOD_DEPTH));
						WriteData(EffParam);
					}
					break;
				case EF_FDS_MOD_SPEED_HI:
					if (ChanID == CHANID_FDS) {
						WriteData(Command(CMD_EFF_FDS_MOD_RATE_HI));
						WriteData(EffParam & 0x0F);
					}
					break;
				case EF_FDS_MOD_SPEED_LO:
					if (ChanID == CHANID_FDS) {
						WriteData(Command(CMD_EFF_FDS_MOD_RATE_LO));
						WriteData(EffParam);
					}
					break;
				case EF_FDS_VOLUME:		// // //
					if (ChanID == CHANID_FDS) {
						WriteData(Command(CME_EFF_FDS_VOLUME));
						WriteData(EffParam == 0xE0 ? 0x80 : (EffParam ^ 0x40));
					}
					break;
				// // // Sunsoft 5B
				case EF_SUNSOFT_ENV_TYPE:
					if (ChipID == SNDCHIP_S5B) {
						WriteData(Command(CMD_EFF_S5B_ENV_TYPE));
						WriteData(EffParam & 0x0F);
					}
					break;
				case EF_SUNSOFT_ENV_HI:
					if (ChipID == SNDCHIP_S5B) {
						WriteData(Command(CMD_EFF_S5B_ENV_RATE_HI));
						WriteData(EffParam);
					}
					break;
				case EF_SUNSOFT_ENV_LO:
					if (ChipID == SNDCHIP_S5B) {
						WriteData(Command(CMD_EFF_S5B_ENV_RATE_LO));
						WriteData(EffParam);
					}
					break;
				// // // N163
				case EF_N163_WAVE_BUFFER:
					if (ChipID == SNDCHIP_N163 && EffParam <= 0x7F) {
						WriteData(Command(CMD_EFF_OFFSET));
						WriteData((EffParam + 1) & 0x7F);
					}
					break;
			}
		}

		// Volume command
		if (Volume < 0x10) {
			WriteDuration();
			WriteData(0xF0 | Volume);
			Action = true;			// Terminate command
		} 

		if (NESNote == 0xFF) {
			if (Action) {
				// A instrument/effect command was issued but no new note, write rest command
				WriteData(0);
			}
			AccumulateDuration();
		}
		else {
			// Write note command
			WriteDuration();
			WriteData(NESNote + 1);
			AccumulateDuration();
		}
	}

	WriteDuration();

//	OptimizeString();
}

unsigned char CPatternCompiler::Command(int cmd) const
{
	// // // truncate values if some chips do not exist
	if (!m_pDocument->ExpansionEnabled(SNDCHIP_N163) && cmd > CME_EFF_N163_FINE_PITCH) cmd -= 1;
	// MMC5
	if (!m_pDocument->ExpansionEnabled(SNDCHIP_FDS) && cmd > CMD_EFF_FDS_MOD_RATE_LO) cmd -= 4;
	// VRC7, VRC6
	return (cmd << 1) | 0x80;
}

unsigned int CPatternCompiler::FindInstrument(int Instrument) const
{
	if (Instrument == MAX_INSTRUMENTS)
		return MAX_INSTRUMENTS;

	for (int i = 0; i < MAX_INSTRUMENTS; i++) {
		if (m_pInstrumentList[i] == Instrument)
			return i;
	}

	return 0;	// Could not find the instrument
}

unsigned int CPatternCompiler::FindSample(int Instrument, int Octave, int Key) const
{
	return (*m_pDPCMList)[Instrument][Octave][Key - 1];
}

CPatternCompiler::stSpacingInfo CPatternCompiler::ScanNoteLengths(int Track, unsigned int StartRow, int Pattern, int Channel)
{
	stChanNote NoteData;
	int StartSpace = -1, Space = 0, SpaceCount = 0;
	stSpacingInfo Info;

	Info.SpaceCount = 0;
	Info.SpaceSize = 0;

	for (unsigned i = StartRow; i < m_pDocument->GetPatternLength(Track); ++i) {
		m_pDocument->GetDataAtPattern(Track, Pattern, Channel, i, &NoteData);
		bool NoteUsed = false;

		if (NoteData.Note > 0)
			NoteUsed = true;
		if (NoteData.Instrument < MAX_INSTRUMENTS)
			NoteUsed = true;
		if (NoteData.Vol < 0x10)
			NoteUsed = true;
		for (unsigned j = 0; j < (m_pDocument->GetEffColumns(Track, Channel) + 1); ++j) {
			if (NoteData.EffNumber[j] != EF_NONE)
				NoteUsed = true;
		}

		if (i == StartRow && NoteUsed == false) {
			Info.SpaceCount = 0xFF;
			Info.SpaceSize = StartSpace;
			return Info;
		}

		if (i > StartRow) {
			if (NoteUsed) {
				if (StartSpace == -1)
					StartSpace = Space;
				else {
					if (StartSpace == Space)
						SpaceCount++;
					else {
						Info.SpaceCount = SpaceCount;
						Info.SpaceSize = StartSpace;
						return Info;
					}
				}
				Space = 0;
			}
			else
				Space++;
		}
	}

	if (StartSpace == Space) {
		SpaceCount++;
	}

	Info.SpaceCount = SpaceCount;
	Info.SpaceSize = StartSpace;

	return Info;
}

void CPatternCompiler::WriteData(unsigned char Value)
{
	m_vData.push_back(Value);
	m_iHash += Value;				// Simple CRC-hash
	m_iHash += (m_iHash << 10);
	m_iHash ^= (m_iHash >> 6);
}

void CPatternCompiler::AccumulateDuration()
{
	++m_iDuration;
}

void CPatternCompiler::WriteDuration()
{
	if (m_iCurrentDefaultDuration == 0xFF) {
		if (!m_vData.size() && m_iDuration > 0)
			WriteData(0x00);
		if (m_iDuration > 0)
			WriteData(m_iDuration - 1);
	}

	m_iDuration = 0;
}

// Returns the size of the block at 'position' in the data array. A block is terminated by a note
int CPatternCompiler::GetBlockSize(int Position)
{
	unsigned int Pos = Position;

	int iDuration = 1;

	// Find if note duration optimization is on
	for (int i = 0; i < Position; ++i) {
		if (m_vData[i] == Command(CMD_SET_DURATION))
			iDuration = 0;
		else if (m_vData[i] == Command(CMD_RESET_DURATION))
			iDuration = 1;
	}

	for (; Pos < m_vData.size(); Pos++) {
		unsigned char data = m_vData[Pos];
		if (data < 0x80) {		// Note
			//int size = (Pos + 1 + iDuration) - Position;
			int size = (Pos - Position);
//			if (size > 1)
//				return size - 1;

			return size + 1 + iDuration;// (Pos + 1 + iDuration) - Position;
		}
		else if (data == Command(CMD_SET_DURATION))
			iDuration = 0;
		else if (data == Command(CMD_RESET_DURATION))
			iDuration = 1;
		else {
			if (data < 0xE0 || data > 0xEF)
				Pos++;				// Command, skip parameter
		}
	//	Pos++;
	}

	// Error
	return 1;
}

void CPatternCompiler::OptimizeString()
{
	// Try to optimize by finding repeating patterns and compress them into a loop (simple RLE)
	//

	//
	// Ok, just figured this won't work without using loads of NES RAM so I'll
	// probably put this on hold for a while
	//

	unsigned int i, j, k, l;
	int matches, best_length, last_inst;
	bool matched;

	/*

	80 00 2E 00 2E 00 2E 00 2E 00 2E 00 2E 00 ->
	80 00 2E 00 FF 06 02

	*/

	// Always copy first 2 bytes
//	memcpy(m_pCompressedData, m_pData, 2);
//	m_iCompressedDataPointer += 2;

	if (m_vData[0] == 0x80)
		last_inst = m_vData[1];
	else
		last_inst = 0;

	// Loop from start
	for (i = 0; i < m_vData.size(); /*i += 2*/) {

		int best_matches = 0;

		// Instrument
		if (m_vData[i] == 0x80)
			last_inst = m_vData[i + 1];
		else if (m_vData[i] >= 0xE0 && m_vData[i] <= 0xEF)
			last_inst = m_vData[i & 0xF];

		// Start checking from the first tuple
		for (l = GetBlockSize(i); l < (m_vData.size() - i); /*l += 2*/) {
			matches = 0;
			// See how many following matches there are from this combination in a row
			for (j = i + l; j <= m_vData.size(); j += l) {
				matched = true;
				// Compare one word
				for (k = 0; k < l; k++) {
					if (m_vData[i + k] != m_vData[j + k])
						matched = false;
				}
				if (!matched)
					break;
				matches++;
				/*
				if ((j + l) <= m_iDataPointer) {
					if (memcmp(m_pData + i, m_pData + j, l) == 0)
						matches++;
					else
						break;
				}
				*/
			}
			// Save
			if (matches > best_matches) {
				best_matches = matches;
				best_length = l;
			}

			l += GetBlockSize(i + l);
		}
		// Compress
		if ((best_matches > 1 && best_length > 4) || best_matches > 2 /*&& (best_length > 2 && best_matches > 1)*/) {
			// Include the first one
			best_matches++;
			int size = best_length * best_matches;
			//
			// Last known instrument must also be added
			//
			std::copy(m_vData.begin() + i, m_vData.begin() + i + best_length, m_vCompressedData.end());
			// Define a loop point: 0xFF (number of loops) (number of bytes)
			m_vCompressedData.push_back(Command(CMD_LOOP_POINT));
			m_vCompressedData.push_back(best_matches - 1);	// the nsf code sees one less
			m_vCompressedData.push_back(best_length);
			i += size;
		}
		else {
			// No loop
			int size = GetBlockSize(i);
			std::copy(m_vData.begin() + i, m_vData.begin() + i + size, m_vCompressedData.end());
			i += size;
		}
	}
}	

unsigned int CPatternCompiler::GetHash() const
{
	return m_iHash;
}

void CPatternCompiler::Print(LPCTSTR text) const
{
	if (m_pLogger != NULL)
		m_pLogger->WriteLog(text);
}

bool CPatternCompiler::CompareData(const std::vector<char> &data) const
{
	return m_vData == data;
}

const std::vector<char> &CPatternCompiler::GetData() const
{
	return m_vData;
}

const std::vector<char> &CPatternCompiler::GetCompressedData() const
{
	return m_vCompressedData;
}

unsigned int CPatternCompiler::GetDataSize() const
{
	return m_vData.size();
}

unsigned int CPatternCompiler::GetCompressedDataSize() const
{
	return m_vCompressedData.size();
}
