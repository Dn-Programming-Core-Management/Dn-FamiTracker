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

#include "Arpeggiator.h"

void CArpeggiator::Tick(int CurrentChannel) {
	if (m_iAutoArpKeyCount == 1)
		return;

	// auto arpeggio
	int OldPtr = m_iAutoArpPtr;
	do {
		m_iAutoArpPtr = (m_iAutoArpPtr + 1) & 127;
		if (m_iAutoArpNotes[m_iAutoArpPtr] == 1) {
			m_iLastAutoArpPtr = m_iAutoArpPtr;
			m_iArpeggiate[CurrentChannel] = m_iAutoArpPtr;
			break;
		}
		else if (m_iAutoArpNotes[m_iAutoArpPtr] == 2) {
			m_iAutoArpNotes[m_iAutoArpPtr] = 0;
		}
	}
	while (m_iAutoArpPtr != OldPtr);
}

void CArpeggiator::TriggerNote(unsigned MidiNote) {
	m_iAutoArpNotes[MidiNote] = 1;
	m_iAutoArpPtr = MidiNote;
	m_iLastAutoArpPtr = m_iAutoArpPtr;

	m_iAutoArpKeyCount = 0;
	for (int i = 0; i < 128; ++i)
		if (m_iAutoArpNotes[i] == 1)
			++m_iAutoArpKeyCount;
}

void CArpeggiator::ReleaseNote(unsigned MidiNote) {
	m_iAutoArpNotes[MidiNote] = 2;
}

void CArpeggiator::CutNote(unsigned MidiNote) {
	m_iAutoArpNotes[MidiNote] = 2;
}

int CArpeggiator::GetNextNote(unsigned Channel) {
	return std::exchange(m_iArpeggiate[Channel], 0);
}

std::string CArpeggiator::GetStateString() const {
	int Base = -1;
	std::string str = "Auto-arpeggio: ";

	for (int i = 0; i < 128; ++i) {
		if (m_iAutoArpNotes[i] == 1) {
			if (Base == -1)
				Base = i;
			str += std::to_string(i - Base) + ' ';
		}
	}

	return Base != -1 ? str : "";
}
