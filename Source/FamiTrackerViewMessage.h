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
