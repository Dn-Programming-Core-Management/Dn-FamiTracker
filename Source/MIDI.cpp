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

#include "stdafx.h"
#include "FamiTracker.h"
#include "PatternNote.h"		// // //
#include "FamiTrackerViewMessage.h"		// // //
#include "MIDI.h"
#include "Settings.h"

/*
 * CMIDI - Wrapper class for input and output MIDI devices
 *
 */

#define ASSEMBLE_STATUS(Message, Channel) (((Message) << 4) | (Channel))
#define ASSEMBLE_PARAM(Status, Byte1, Byte2) ((Status) | ((Byte1) << 8) | ((Byte2) << 16))

// Static stuff

CMIDI *CMIDI::m_pInstance = NULL;

void CALLBACK CMIDI::MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	// MIDI input callback function

	if (wMsg == MIM_DATA) {
		unsigned char Status = (char)(dwParam1 & 0xFF);
		unsigned char Data1	 = (char)(dwParam1 >> 8) & 0xFF;
		unsigned char Data2	 = (char)(dwParam1 >> 16) & 0xFF;
		m_pInstance->Event(Status, Data1, Data2);
	}
}

// Instance stuff

CMIDI::CMIDI() : 
	m_bInStarted(false), 
	m_iInDevice(0),
	m_iOutDevice(0),
	m_MidiQueue(MAX_QUEUE),
	m_hMIDIIn(NULL),
	m_hMIDIOut(NULL),
	m_iTimingCounter(0)
{
	// Allow only one single midi object
	ASSERT( m_pInstance == NULL );
	m_pInstance = this;
}

CMIDI::~CMIDI()
{
	CloseDevices();
}

// CMIDI member functions

bool CMIDI::Init(void)
{
	// Load from settings
	m_iInDevice = theApp.GetSettings()->Midi.iMidiDevice;
	m_iOutDevice = theApp.GetSettings()->Midi.iMidiOutDevice;

	m_bMasterSync = theApp.GetSettings()->Midi.bMidiMasterSync;

	// Open devices
	OpenDevices();

	return true;
}

void CMIDI::Shutdown(void)
{
	// Store settings
	theApp.GetSettings()->Midi.iMidiDevice = m_iInDevice;
	theApp.GetSettings()->Midi.iMidiOutDevice = m_iOutDevice;

	CloseDevices();
}

bool CMIDI::OpenDevices(void)
{
	MMRESULT Result;

	if (m_iInDevice == 0 && m_iOutDevice == 0)
		return true;

	// Input
	if (m_iInDevice != 0) {

		Result = midiInOpen(&m_hMIDIIn, m_iInDevice - 1, (DWORD_PTR)MidiInProc, 0, CALLBACK_FUNCTION);

		if (Result != MMSYSERR_NOERROR) {
			m_hMIDIIn = NULL;
			AfxMessageBox(IDS_MIDI_ERR_INPUT);
			return false;
		}

		// Auto-enable input device
		midiInStart(m_hMIDIIn);
		m_bInStarted = true;
	}

	// Output
	if (m_iOutDevice != 0) {

		Result = midiOutOpen(&m_hMIDIOut, m_iOutDevice - 1, NULL, 0, CALLBACK_NULL);

		if (Result != MMSYSERR_NOERROR) {
			m_hMIDIOut = NULL;
			AfxMessageBox(IDS_MIDI_ERR_OUTPUT);
			return false;
		}

		// Set patches
		midiOutShortMsg(m_hMIDIOut, (MIDI_MSG_PROGRAM_CHANGE << 4 | 0x00) | (1 << 8));
		midiOutShortMsg(m_hMIDIOut, (MIDI_MSG_PROGRAM_CHANGE << 4 | 0x01) | (1 << 8));
		midiOutShortMsg(m_hMIDIOut, (MIDI_MSG_PROGRAM_CHANGE << 4 | 0x02) | (74 << 8));
		midiOutShortMsg(m_hMIDIOut, (MIDI_MSG_PROGRAM_CHANGE << 4 | 0x03) | (115 << 8));
		midiOutShortMsg(m_hMIDIOut, (MIDI_MSG_PROGRAM_CHANGE << 4 | 0x04) | (118 << 8));

		midiOutReset(m_hMIDIOut);
	}

	while (m_MidiQueue.front()) {
		m_MidiQueue.pop();
	}

	return true;
}

bool CMIDI::CloseDevices(void)
{
	if (m_bInStarted) {
		midiInStop(m_hMIDIIn);
		m_bInStarted = false;
	}

	if (m_hMIDIIn != NULL) {
		midiInClose(m_hMIDIIn);
		m_hMIDIIn = NULL;
	}

	if (m_hMIDIOut != NULL) {
		midiOutClose(m_hMIDIOut);
		m_hMIDIOut = NULL;
	}

	return false;
}

int CMIDI::GetNumInputDevices() const
{
	return midiInGetNumDevs();
}

int CMIDI::GetNumOutputDevices() const
{
	return midiOutGetNumDevs();
}

void CMIDI::GetInputDeviceString(int Num, CString &Text) const
{
	MIDIINCAPS InCaps;
	midiInGetDevCaps(Num, &InCaps, sizeof(MIDIINCAPS));
	Text = InCaps.szPname;
}

void CMIDI::GetOutputDeviceString(int Num, CString &Text) const
{
	MIDIOUTCAPS OutCaps;
	midiOutGetDevCaps(Num, &OutCaps, sizeof(MIDIOUTCAPS));
	Text = OutCaps.szPname;
}

void CMIDI::SetInputDevice(int Device, bool MasterSync)
{
	m_iInDevice = Device;
	m_bMasterSync = MasterSync;

	CloseDevices();
	OpenDevices();
}

void CMIDI::SetOutputDevice(int Device)
{
	m_iOutDevice = Device;

	CloseDevices();
	OpenDevices();
}

void CMIDI::Enqueue(unsigned char MsgType, unsigned char MsgChannel, unsigned char Data1, unsigned char Data2)
{
	// Ehh, dropped events are fine I guess...
	(void) m_MidiQueue.try_push(MidiMessage{
		(char)MsgType,
		(char)MsgChannel,
		(char)Data1,
		(char)Data2,
		(char)m_iTimingCounter,
	});
}

void CMIDI::Event(unsigned char Status, unsigned char Data1, unsigned char Data2)
{
	const unsigned char MsgType	   = Status >> 4;
	const unsigned char MsgChannel = Status & 0x0F;

	CFrameWnd *pFrame = static_cast<CFrameWnd*>(AfxGetApp()->m_pMainWnd);
	CView *pView = pFrame->GetActiveView();

	TRACE("%i: MIDI message %02X %02X %02X\n", GetTickCount(), Status, Data1, Data2);

	// Timing
	switch (Status) {
	case 0xF8:	// Timing tick
		if (m_bMasterSync) {
			if (++m_iTimingCounter == 6) {
				m_iTimingCounter = 0;
				Enqueue(MsgType, MsgChannel, Data1, Data2);
				pView->PostMessage(WM_USER_MIDI_EVENT);
			}
		}
		break;
	case 0xFA:	// Start
		m_iTimingCounter = 0;
		break;
	case 0xFC:	// Stop
		m_iTimingCounter = 0;
		break;
	default:
		switch (MsgType) {
			case MIDI_MSG_NOTE_OFF:
			case MIDI_MSG_NOTE_ON: 
			case MIDI_MSG_PITCH_WHEEL:
				Enqueue(MsgType, MsgChannel, Data1, Data2);
				pView->PostMessage(WM_USER_MIDI_EVENT);
				break;
		}
	}
}

bool CMIDI::ReadMessage(unsigned char & Message, unsigned char & Channel, unsigned char & Data1, unsigned char & Data2)
{
	bool Result = false;
	
	if (auto MidiMessage = m_MidiQueue.front()) {
		m_MidiQueue.pop();
		Result = true;

		Message = MidiMessage->MsgType;
		Channel = MidiMessage->MsgChan;
		Data1	= MidiMessage->Data1;
		Data2	= MidiMessage->Data2;
		m_iQuant = MidiMessage->Quantization;
	}

	return Result;
}

int CMIDI::GetQuantization() const
{
	return m_iQuant;
}

void CMIDI::ToggleInput()
{
	if (m_bInStarted)
		midiInStop(m_hMIDIIn);
	else
		midiInStart(m_hMIDIIn);

	m_bInStarted = !m_bInStarted;
}

void CMIDI::WriteNote(unsigned char Channel, unsigned char Note, unsigned char Octave, unsigned char Velocity)
{
	static unsigned int LastNote[MAX_CHANNELS];	// Quick hack
//	static unsigned int LastVolume[MAX_CHANNELS];

	if (/*!m_bOpened ||*/ m_iOutDevice == 0 || m_hMIDIOut == NULL)
		return;

	if (Note == 0)
		return;

	Octave++;

	if ((Channel == 4 || Channel == 3) && Octave < 3)
		Octave += 3;

	if (Velocity == 0x10)
		Velocity--;
		/*
		Velocity = LastVolume[Channel];
	else
		LastVolume[Channel] = Velocity;*/

	unsigned int MsgChannel = Channel;
	unsigned int MsgType;

	unsigned int Data1 = Note - 1 + Octave * 12;		// note
	unsigned int Data2 = Velocity * 8;					// velocity

	if (Note == HALT || Note == RELEASE) {
		MsgType = MIDI_MSG_NOTE_OFF;
		Data2 = 0;
		Data1 = LastNote[Channel];
		LastNote[Channel] = 0;
	}
	else {
		if (LastNote[Channel] != 0 && Note != LastNote[Channel])
			WriteNote(Channel, HALT, 0, 0);

		MsgType = MIDI_MSG_NOTE_ON;
		LastNote[Channel] = Data1;
	}

	unsigned int Status = (MsgType << 4) | MsgChannel;
	unsigned int dwParam1 = Status | (Data1 << 8) | (Data2 << 16);

	midiOutShortMsg(m_hMIDIOut, dwParam1);
}

void CMIDI::ResetOutput()
{
	midiOutReset(m_hMIDIOut);
}

bool CMIDI::IsOpened() const
{
	return m_bInStarted;
}

bool CMIDI::IsAvailable() const
{
	return m_hMIDIIn != NULL;
}

int CMIDI::GetInputDevice() const 
{ 
	return m_iInDevice; 
}

int CMIDI::GetOutputDevice() const 
{ 
	return m_iOutDevice; 
}
