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

// This file handles playing of VRC7 channels

#include "ChannelsVRC7.h"
#include "APU/Types.h"		// // //
#include "InstHandler.h"		// // //
#include "InstHandlerVRC7.h"		// // //

#define OPL_NOTE_ON 0x10
#define OPL_SUSTAIN_ON 0x20

const int VRC7_PITCH_RESOLUTION = 2;		// // // extra bits for internal pitch

// True if custom instrument registers needs to be updated, shared among all channels
bool CChannelHandlerVRC7::m_bRegsDirty = false;
// Each bit represents that the custom patch register on that index has been updated
char CChannelHandlerVRC7::m_cPatchFlag = 0;		// // // 050B
// Custom instrument patch
unsigned char CChannelHandlerVRC7::m_iPatchRegs[8] = { };		// // // 050B

CChannelHandlerVRC7::CChannelHandlerVRC7() : 
	CChannelHandlerInverted((1 << (VRC7_PITCH_RESOLUTION + 9)) - 1, 15),		// // //
	m_iCommand(CMD_NONE),
	m_iTriggeredNote(0)
{
	m_iVolume = VOL_COLUMN_MAX;
}

void CChannelHandlerVRC7::SetChannelID(int ID)
{
	CChannelHandler::SetChannelID(ID);
	m_iChannel = ID - CHANID_VRC7_CH1;
}

void CChannelHandlerVRC7::SetPatch(unsigned char Patch)		// // //
{
	m_iDutyPeriod = Patch;
}

void CChannelHandlerVRC7::SetCustomReg(size_t Index, unsigned char Val)		// // //
{
	if (!(m_cPatchFlag & (1 << Index)))		// // // 050B
		m_iPatchRegs[Index] = Val;
}

void CChannelHandlerVRC7::HandleNoteData(stChanNote &NoteData)		// // //
{
	CChannelHandlerInverted::HandleNoteData(NoteData);		// // //

	if (m_iCommand == CMD_NOTE_TRIGGER && NoteData.Instrument == HOLD_INSTRUMENT)		// // // 050B
		m_iCommand = CMD_NOTE_ON;
}

bool CChannelHandlerVRC7::HandleEffect(effect_t EffNum, unsigned char EffParam)
{
	switch (EffNum) {
	case EF_DUTY_CYCLE:
		m_iPatch = EffParam;		// // // 050B
		break;
	case EF_VRC7_PORT:		// // // 050B
		m_iCustomPort = EffParam & 0x07;
		break;
	case EF_VRC7_WRITE:		// // // 050B
		m_iPatchRegs[m_iCustomPort] = EffParam;
		m_cPatchFlag |= 1 << m_iCustomPort;
		m_bRegsDirty = true;
		break;
	default: return CChannelHandlerInverted::HandleEffect(EffNum, EffParam);
	}

	return true;
}

void CChannelHandlerVRC7::HandleEmptyNote()
{
}

void CChannelHandlerVRC7::HandleCut()
{
	RegisterKeyState(-1);
	m_bGate = false;
//	m_iPeriod = 0;
//	m_iPortaTo = 0;
	m_iCommand = CMD_NOTE_HALT;
//	m_iOldOctave = -1;		// // //
}

void CChannelHandlerVRC7::UpdateNoteRelease()		// // //
{
	// Note release (Lxx)
	if (m_iNoteRelease > 0) {
		m_iNoteRelease--;
		if (m_iNoteRelease == 0) {
			HandleRelease();
		}
	}
}

void CChannelHandlerVRC7::HandleRelease()
{
	if (!m_bRelease) {
		m_iCommand = CMD_NOTE_RELEASE;
		RegisterKeyState(-1);
	}
}

void CChannelHandlerVRC7::HandleNote(int Note, int Octave)
{
	CChannelHandler::HandleNote(Note, Octave);		// // //

	m_bHold	= true;

/*
	if ((m_iEffect != EF_PORTAMENTO || m_iPortaSpeed == 0) ||
		m_iCommand == CMD_NOTE_HALT || m_iCommand == CMD_NOTE_RELEASE)		// // // 050B
		m_iCommand = CMD_NOTE_TRIGGER;
*/
	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO &&
		m_iCommand != CMD_NOTE_HALT && m_iCommand != CMD_NOTE_RELEASE)		// // // 050B
		CorrectOctave();
	else
		m_iCommand = CMD_NOTE_TRIGGER;
}

int CChannelHandlerVRC7::RunNote(int Octave, int Note)		// // //
{
	// Run the note and handle portamento
	int NewNote = MIDI_NOTE(Octave, Note);

	int NesFreq = TriggerNote(NewNote);

	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO && m_bGate) {		// // //
		if (m_iPeriod == 0) {
			m_iPeriod = NesFreq;
			m_iOldOctave = m_iOctave = Octave;
		}
		m_iPortaTo = NesFreq;
		
	}
	else {
		m_iPeriod = NesFreq;
		m_iPortaTo = 0;
		m_iOldOctave = m_iOctave = Octave;
	}

	m_bGate = true;

	CorrectOctave();		// // //

	return NewNote;
}

bool CChannelHandlerVRC7::CreateInstHandler(inst_type_t Type)
{
	switch (Type) {
	case INST_VRC7:
		if (m_iInstTypeCurrent != INST_VRC7)
			m_pInstHandler.reset(new CInstHandlerVRC7(this, 0x0F));
		return true;
	}
	return false;
}

void CChannelHandlerVRC7::SetupSlide()		// // //
{
	CChannelHandler::SetupSlide();		// // //
	
	CorrectOctave();
}

void CChannelHandlerVRC7::CorrectOctave()		// // //
{
	// Set current frequency to the one with highest octave
	if (m_bLinearPitch)
		return;

	if (m_iOldOctave == -1) {
		m_iOldOctave = m_iOctave;
		return;
	}

	int Offset = m_iOctave - m_iOldOctave;
	if (Offset > 0) {
		m_iPeriod >>= Offset;
		m_iOldOctave = m_iOctave;
	}
	else if (Offset < 0) {
		// Do nothing
		m_iPortaTo >>= -Offset;
		m_iOctave = m_iOldOctave;
	}
}

int CChannelHandlerVRC7::TriggerNote(int Note)
{
	m_iTriggeredNote = Note;
	RegisterKeyState(Note);
	if (m_iCommand != CMD_NOTE_TRIGGER && m_iCommand != CMD_NOTE_HALT)
		m_iCommand = CMD_NOTE_ON;
	m_iOctave = Note / NOTE_RANGE;

	return m_bLinearPitch ? (Note << LINEAR_PITCH_AMOUNT) : GetFnum(Note);		// // //
}

unsigned int CChannelHandlerVRC7::GetFnum(int Note) const
{
	return m_pNoteLookupTable[Note % NOTE_RANGE] << VRC7_PITCH_RESOLUTION;		// // //
}

int CChannelHandlerVRC7::CalculateVolume() const
{
	int Volume = (m_iVolume >> VOL_COLUMN_SHIFT) - GetTremolo();
	if (Volume > 15)
		Volume = 15;
	if (Volume < 0)
		Volume = 0;
	return Volume;		// // //
}

int CChannelHandlerVRC7::CalculatePeriod() const
{
	int Detune = GetVibrato() - GetFinePitch() - GetPitch();
	int Period = LimitPeriod(GetPeriod() + (Detune << VRC7_PITCH_RESOLUTION));		// // //
	if (m_bLinearPitch && m_pNoteLookupTable != nullptr) {
		Period = LimitPeriod(GetPeriod() + Detune);		// // //
		int Note = (Period >> LINEAR_PITCH_AMOUNT) % NOTE_RANGE;
		int Sub = Period % (1 << LINEAR_PITCH_AMOUNT);
		int Offset = (GetFnum(Note + 1) << ((Note < NOTE_RANGE - 1) ? 0 : 1)) - GetFnum(Note);
		Offset = Offset * Sub >> LINEAR_PITCH_AMOUNT;
		if (Sub && Offset < (1 << VRC7_PITCH_RESOLUTION)) Offset = 1 << VRC7_PITCH_RESOLUTION;
		Period = GetFnum(Note) + Offset;
	}
	return LimitRawPeriod(Period) >> VRC7_PITCH_RESOLUTION;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC7 Channels
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC7Channel::RefreshChannel()
{	
//	int Note = m_iTriggeredNote;
	int Volume = CalculateVolume();
	int Fnum = CalculatePeriod();		// // //
	int Bnum = !m_bLinearPitch ? m_iOctave :
		((GetPeriod() + GetVibrato() - GetFinePitch() - GetPitch()) >> LINEAR_PITCH_AMOUNT) / NOTE_RANGE;

	if (m_iPatch != -1) {		// // //
		m_iDutyPeriod = m_iPatch;
		m_iPatch = -1;
	}

	// Write custom instrument
	if (m_iDutyPeriod == 0 && (m_iCommand == CMD_NOTE_TRIGGER || m_bRegsDirty)) {
		for (int i = 0; i < 8; ++i)
			RegWrite(i, m_iPatchRegs[i]);
	}

	m_bRegsDirty = false;

	if (!m_bGate)
		m_iCommand = CMD_NOTE_HALT;

	int Cmd = 0;

	switch (m_iCommand) {
		case CMD_NOTE_TRIGGER:
			RegWrite(0x20 + m_iChannel, 0);
			m_iCommand = CMD_NOTE_ON;
			Cmd = OPL_NOTE_ON | OPL_SUSTAIN_ON;
			break;
		case CMD_NOTE_ON:
			Cmd = m_bHold ? OPL_NOTE_ON : OPL_SUSTAIN_ON;
			break;
		case CMD_NOTE_HALT:
			Cmd = 0;
			break;
		case CMD_NOTE_RELEASE:
			Cmd = OPL_SUSTAIN_ON;
			break;
	}
	
	// Write frequency
	RegWrite(0x10 + m_iChannel, Fnum & 0xFF);
	
	if (m_iCommand != CMD_NOTE_HALT) {
		// Select volume & patch
		RegWrite(0x30 + m_iChannel, (m_iDutyPeriod << 4) | (Volume ^ 0x0F));		// // //
	}

	RegWrite(0x20 + m_iChannel, ((Fnum >> 8) & 1) | (Bnum << 1) | Cmd);

	if (m_iChannelID == CHANID_VRC7_CH6)		// // // 050B
		m_cPatchFlag = 0;
}

void CVRC7Channel::ClearRegisters()
{
	for (int i = 0x10; i < 0x30; i += 0x10)
		RegWrite(i + m_iChannel, 0);
	RegWrite(0x30 + m_iChannel, 0x0F);		// // //

	m_iNote = 0;
	m_iOctave = m_iOldOctave = -1;		// // //
	m_iPatch = -1;
	m_iEffect = EF_NONE;

	m_iCommand = CMD_NOTE_HALT;
	m_iCustomPort = 0;		// // // 050B
}

void CVRC7Channel::RegWrite(unsigned char Reg, unsigned char Value)
{
	WriteRegister(0x9010, Reg);
	WriteRegister(0x9030, Value);
}
