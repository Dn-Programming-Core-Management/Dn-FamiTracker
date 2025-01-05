/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/


#pragma once

#include "WinUser.h"

// Custom window messages for CFamiTrackerView
enum {
	WM_USER_MIDI_EVENT = WM_USER,				// There is a new MIDI command
	WM_USER_GUI_COUNT,
};

/// Sent from audio to GUI thread. Only pass into CFamiTrackerView::PostQueueMessage()
/// to avoid blocking the audio thread!
enum AudioMessageId {
	AM_PLAYER = WM_USER_GUI_COUNT,  // Pattern play row has changed
	AM_NOTE_EVENT,  // There is a new note command (by player)
	AM_DUMP_INST,  // // // End of track, add instrument

	AM_ERROR,  // audio thread error, (nIDPrompt, nType)
	/*
	Previously the audio thread would call AfxMessageBox upon errors. The resulting
	message boxes would often appear under the main window.

	https://forums.codeguru.com/showthread.php?454091-AfxMessageBox-from-a-worker-thread
	says to never call AfxMessageBox from a non-GUI thread, but instead call PostMessage
	to an object on the main window. http://flounder.com/workerthreads.htm says it can
	be a view rather than a CWinApp.

	As a result, the audio thread now calls
	CFamiTrackerView::PostMessage(WM_USER_ERROR, nIDPrompt, nType), which messages the
	main thread to call CFamiTrackerView::OnAudioThreadError(), which calls
	AfxMessageBox(nIDPrompt, nType).
	*/
};

struct AudioMessage {
	AudioMessageId message;
	WPARAM wParam;
	LPARAM lParam;
};
