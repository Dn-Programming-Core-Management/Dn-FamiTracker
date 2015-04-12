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

// This file handles playing of VRC7 channels

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ChannelHandler.h"
#include "ChannelsVRC7.h"
#include "SoundGen.h"

#define OPL_NOTE_ON 0x10
#define OPL_SUSTAIN_ON 0x20

// True if custom instrument registers needs to be updated, shared among all channels
bool CChannelHandlerVRC7::m_bRegsDirty = false;

CChannelHandlerVRC7::CChannelHandlerVRC7() : 
	CChannelHandlerInverted(2047, 15),		// // //
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

void CChannelHandlerVRC7::HandleCustomEffects(int EffNum, int EffParam)
{
	if (EffNum == EF_PORTA_DOWN) {
		m_iEffect = EF_PORTA_UP;
		m_iEffectParam = EffParam;		// // //
		m_iPortaSpeed = EffParam;
	}
	else if (EffNum == EF_PORTA_UP) {
		m_iEffect = EF_PORTA_DOWN;
		m_iEffectParam = EffParam;		// // //
		m_iPortaSpeed = EffParam;
	}
	else {
		if (!CheckCommonEffects(EffNum, EffParam)) {
			switch (EffNum) {
				// // //
				case EF_DUTY_CYCLE:
//					Patch = EffParam;		// TODO add this
					break;
/*
				case EF_VRC7_MODULATOR:
					switch (EffParam & 0xF0) {
						case 0x00:	// Amplitude modulation on/off
							break;
						case 0x10:	// Vibrato on/off
							break;
						case 0x20:	// Sustain on/off
							break;
						case 0x30:	// Wave rectification on/off
							break;
						case 0x40:	// Key rate scaling on/off
							break;
						case 0x50:	// Key rate level
							break;
						case 0x60:	// Mult factor
							break;
						case 0x70:	// Attack
							break;
						case 0x80:	// Decay
							break;
						case 0x90:	// Sustain
							break;
						case 0xA0:	// Release
							break;
					}
					break;
				case EF_VRC7_CARRIER:
					break;
				case EF_VRC7_LEVELS:
					if (EffParam & 0x80)	// Feedback
						m_iRegs[0x03] = (m_iRegs[0x03] & 0xF8) | (EffParam & 0x07);
					else
						m_iRegs[0x02] = (m_iRegs[0x02] & 0xC0) | (EffParam & 0x3F);
					m_bRegsDirty = true;
					break;
					*/
			}
		}
	}
}

bool CChannelHandlerVRC7::HandleInstrument(int Instrument, bool Trigger, bool NewInstrument)
{
	CFamiTrackerDoc *pDocument = m_pSoundGen->GetDocument();
	CInstrumentContainer<CInstrumentVRC7> instContainer(pDocument, Instrument);
	CInstrumentVRC7 *pInstrument = instContainer();

	if (pInstrument == NULL)
		return false;

	if (Trigger || NewInstrument) {
		// Patch number
		m_iPatch = pInstrument->GetPatch();

		// Load custom parameters
		if (m_iPatch == 0) {
			for (int i = 0; i < 8; ++i)
				m_iRegs[i] = pInstrument->GetCustomReg(i);
		}
	}

	return true;
}

void CChannelHandlerVRC7::HandleEmptyNote()
{
}

void CChannelHandlerVRC7::HandleCut()
{
	m_iCommand = CMD_NOTE_HALT;
	m_iPortaTo = 0;
	RegisterKeyState(-1);
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
	int OldNote = m_iNote;
	int OldOctave = m_iOctave;

	// Portamento fix
	if (m_iCommand == CMD_NOTE_HALT)
		m_iPeriod = 0;

	// Trigger note
	m_iNote	= CChannelHandler::RunNote(Octave, Note);
	m_bHold	= true;

	if ((m_iEffect != EF_PORTAMENTO || m_iPortaSpeed == 0) || m_iCommand == CMD_NOTE_HALT)
		m_iCommand = CMD_NOTE_TRIGGER;

	if (m_iPortaSpeed > 0 && m_iEffect == EF_PORTAMENTO && m_iCommand != CMD_NOTE_HALT) {
		// Set current frequency to the one with highest octave
		if (m_iOctave > OldOctave) {
			m_iPeriod >>= (m_iOctave - OldOctave);
		}
		else if (OldOctave > m_iOctave) {
			// Do nothing
			m_iPortaTo >>= (OldOctave - m_iOctave);
			m_iOctave = OldOctave;
		}
	}
}

void CChannelHandlerVRC7::SetupSlide()		// // //
{
	int OldOctave = m_iOctave;

	CChannelHandler::SetupSlide();		// // //

	if (m_iOctave > OldOctave) {
		m_iPeriod >>= (m_iOctave - OldOctave);
	}
	else if (m_iOctave < OldOctave) {
		m_iPortaTo >>= (OldOctave - m_iOctave);
		m_iOctave = OldOctave;
	}
}

void CChannelHandlerVRC7::ProcessChannel()
{
	// Default effects
	CChannelHandler::ProcessChannel();
}

void CChannelHandlerVRC7::ResetChannel()
{
	CChannelHandler::ResetChannel();
}

int CChannelHandlerVRC7::TriggerNote(int Note)
{
	m_iTriggeredNote = Note;
	RegisterKeyState(Note);
	if (m_iCommand != CMD_NOTE_TRIGGER && m_iCommand != CMD_NOTE_HALT)
		m_iCommand = CMD_NOTE_ON;
	m_iOctave = Note / NOTE_RANGE;
	return GetFnum(Note);
}

unsigned int CChannelHandlerVRC7::GetFnum(int Note) const
{
	return m_pNoteLookupTable[Note % NOTE_RANGE] << 2;		// // //
}

int CChannelHandlerVRC7::CalculateVolume() const
{
	int Volume = (m_iVolume >> VOL_COLUMN_SHIFT) - GetTremolo();
	if (Volume > 15)
		Volume = 15;
	if (Volume < 0)
		Volume = 0;
	return 15 - Volume;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// VRC7 Channels
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void CVRC7Channel::RefreshChannel()
{	
	int Note = m_iTriggeredNote;
	int Patch = m_iPatch;
	int Volume = CalculateVolume();
	int Bnum = m_iOctave;
	int Fnum = (m_iPeriod >> 2) - GetVibrato() - GetFinePitch();// (m_iFinePitch - 0x80);

	// Write custom instrument
	if (Patch == 0 && (m_iCommand == CMD_NOTE_TRIGGER || m_bRegsDirty)) {
		for (int i = 0; i < 8; ++i)
			RegWrite(i, m_iRegs[i]);
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
		RegWrite(0x30 + m_iChannel, (Patch << 4) | Volume);
	}

	RegWrite(0x20 + m_iChannel, ((Fnum >> 8) & 1) | (Bnum << 1) | Cmd);
}

void CVRC7Channel::ClearRegisters()
{
	for (int i = 0x10; i < 0x30; i += 0x10)
		RegWrite(i + m_iChannel, 0);
	RegWrite(0x30 + m_iChannel, 0x0F);		// // //

	m_iNote = 0;
	m_iEffect = EF_NONE;

	m_iCommand = CMD_NOTE_HALT;

}

void CVRC7Channel::RegWrite(unsigned char Reg, unsigned char Value)
{
	WriteExternalRegister(0x9010, Reg);
	WriteExternalRegister(0x9030, Value);
}
