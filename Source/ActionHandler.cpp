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

#include "ActionHandler.h"
#include "Action.h"
#include "stdafx.h" // ???

CActionHandler::CActionHandler(unsigned capacity) :
	redoPtr_(undoList_.cbegin()), capacity_(capacity)
{
}

bool CActionHandler::AddAction(CMainFrame &cxt, std::unique_ptr<CAction> pAction) {		// // //
	if (!pAction || !pAction->Commit(cxt))
		return false;

	redoPtr_ = undoList_.erase(redoPtr_, undoList_.cend());
	if (!(CanUndo() && (*std::prev(redoPtr_))->Merge(*pAction))) {
		undoList_.push_back(std::move(pAction));
		if (undoList_.size() > capacity_) {
			undoList_.pop_front();
			lost_ = true;
		}
	}

	return true;
}

void CActionHandler::UndoLastAction(CMainFrame &cxt) {		// // //
	if (CanUndo())
		(*--redoPtr_)->PerformUndo(cxt);
}

void CActionHandler::RedoLastAction(CMainFrame &cxt) {		// // //
	if (CanRedo())
		(*redoPtr_++)->PerformRedo(cxt);
}

bool CActionHandler::ActionsLost() const {		// // //
	return lost_;
}

bool CActionHandler::CanUndo() const
{
	return redoPtr_ != undoList_.cbegin();
}

bool CActionHandler::CanRedo() const
{
	return redoPtr_ != undoList_.cend();
}
