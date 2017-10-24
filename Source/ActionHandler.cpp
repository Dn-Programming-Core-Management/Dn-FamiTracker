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
#include "FamiTrackerDoc.h"

const int CActionHandler::MAX_LEVELS = 64;		// // //

void CActionHandler::Clear()
{
	m_UndoStack.clear();
	m_RedoStack.clear();
	lost_ = false;
}

void CActionHandler::Push(std::unique_ptr<CAction> pAction)		// // //
{
	if (!pAction)
		return;
	if (m_UndoStack.empty() || !m_UndoStack.back()->Merge(*pAction)) {
		if (m_UndoStack.size() == MAX_LEVELS) {
			m_UndoStack.erase(m_UndoStack.begin());
			lost_ = true;
		}
		m_UndoStack.push_back(std::move(pAction));
	}
	m_RedoStack.clear();
}

void CActionHandler::UndoLastAction(CMainFrame &cxt) {		// // //
	if (CanUndo()) {
		auto &pAction = m_RedoStack.emplace_back(std::move(m_UndoStack.back()));
		pAction->RestoreRedoState(cxt);
		pAction->Undo(cxt);
		pAction->RestoreUndoState(cxt);
		m_UndoStack.pop_back();
	}
}

void CActionHandler::RedoLastAction(CMainFrame &cxt) {		// // //
	if (CanRedo()) {
		auto &pAction = m_UndoStack.emplace_back(std::move(m_RedoStack.back()));
		pAction->RestoreUndoState(cxt);		// // //
		pAction->Redo(cxt);
		pAction->RestoreRedoState(cxt);		// // //
		m_RedoStack.pop_back();
	}
}

bool CActionHandler::ActionsLost() const {		// // //
	return lost_;
}

bool CActionHandler::CanUndo() const
{
	return !m_UndoStack.empty();
}

bool CActionHandler::CanRedo() const
{
	return !m_RedoStack.empty();
}
