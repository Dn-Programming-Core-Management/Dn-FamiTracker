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

// These classes implements a new more flexible undo system

#include "Action.h"
#include <utility> // std::exchange

bool CAction::Merge(const CAction &Other) {		// // //
	return false;
}

bool CAction::Commit(CMainFrame &cxt) {
	if (done_)
		return false;
	SaveUndoState(cxt);
	if (!SaveState(cxt)) // TODO: remove
		return false; // Operation cancelled
	Redo(cxt);
	SaveRedoState(cxt);
	return done_ = true;
}


void CAction::PerformUndo(CMainFrame &cxt) {
	if (std::exchange(done_, false)) {
		RestoreRedoState(cxt);
		Undo(cxt);
		RestoreUndoState(cxt);
	}
}
void CAction::PerformRedo(CMainFrame &cxt) {
	if (!std::exchange(done_, true)) {
		RestoreUndoState(cxt);
		Redo(cxt);
		RestoreRedoState(cxt);
	}
}
