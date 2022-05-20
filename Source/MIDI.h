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

#include <mmsystem.h>
#include "rigtorp/SPSCQueue.h"

const int MIDI_MSG_NOTE_OFF			= 0x08;
const int MIDI_MSG_NOTE_ON			= 0x09;
const int MIDI_MSG_AFTER_TOUCH		= 0x0A;
const int MIDI_MSG_CONTROL_CHANGE	= 0x0B;
const int MIDI_MSG_PROGRAM_CHANGE	= 0x0C;
const int MIDI_MSG_CHANNEL_PRESSURE = 0x0D;
const int MIDI_MSG_PITCH_WHEEL		= 0x0E;

// CMIDI command target

struct MidiMessage {
	char MsgType;
	char MsgChan;
	char Data1;
	char Data2;
	char Quantization;
};

class CMIDI : public CObject
{
public:
	CMIDI();
	virtual ~CMIDI();

	bool	Init(void);
	void	Shutdown(void);

	bool	OpenDevices(void);
	bool	CloseDevices(void);

	bool	ReadMessage(unsigned char & Message, unsigned char & Channel, unsigned char & Data1, unsigned char & Data2);
	void	WriteNote(unsigned char Channel, unsigned char Note, unsigned char Octave, unsigned char Velocity);
	void	ResetOutput();
	void	ToggleInput();

	int		GetQuantization() const;

	bool	IsOpened() const;
	bool	IsAvailable() const;

	void	SetInputDevice(int Device, bool MasterSync);
	void	SetOutputDevice(int Device);

	int		GetInputDevice() const;
	int		GetOutputDevice() const;

	// Device enumeration
	int		GetNumInputDevices() const;
	int		GetNumOutputDevices() const;

	void	GetInputDeviceString(int Num, CString &Text) const;
	void	GetOutputDeviceString(int Num, CString &Text) const;

	// Private methods
private:
	void	Event(unsigned char Status, unsigned char Data1, unsigned char Data2);
	void	Enqueue(unsigned char MsgType, unsigned char MsgChannel, unsigned char Data1, unsigned char Data2);

	// Constants
private:
	static const int MAX_QUEUE = 100;

	// Static functions & variables
private:
	static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
	static CMIDI *m_pInstance;

	// Private variables
private:
	// Devices
	int		m_iInDevice;
	int		m_iOutDevice;

	bool	m_bMasterSync;
	bool	m_bInStarted;

	// MIDI queue
	rigtorp::SPSCQueue<MidiMessage> m_MidiQueue;

	int		m_iQuant;
	int		m_iTimingCounter;

	// Device handles
	HMIDIIN	 m_hMIDIIn;
	HMIDIOUT m_hMIDIOut;
};
