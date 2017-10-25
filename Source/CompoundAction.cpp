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

#include "CompoundAction.h"

// // //
// Compound action class
// // //

bool CCompoundAction::Commit(CMainFrame &MainFrm) {
	if (done_ || m_pActionList.empty())
		return false;

	auto it = m_pActionList.begin();
	const auto end = m_pActionList.end();
	while (it != end) {
		(*it)->SaveUndoState(MainFrm);
		if (!(*it)->SaveState(MainFrm)) { // TODO: remove
			while (it != m_pActionList.begin())
				(*--it)->Undo(MainFrm);
			return false; // Operation cancelled
		}
		(*it++)->Redo(MainFrm);
	}

	SaveRedoState(MainFrm);
	return done_ = true;
}

bool CCompoundAction::SaveState(const CMainFrame &MainFrm)
{
	return !m_pActionList.empty() && m_pActionList.front()->SaveState(MainFrm);
}

void CCompoundAction::Undo(CMainFrame &MainFrm)
{
	for (auto it = m_pActionList.rbegin(); it != m_pActionList.rend(); ++it)
		(*it)->Undo(MainFrm);
}

void CCompoundAction::Redo(CMainFrame &MainFrm)
{
	for (auto &x : m_pActionList)
		x->Redo(MainFrm);
}

void CCompoundAction::SaveUndoState(const CMainFrame &MainFrm)
{
	m_pActionList.front()->SaveUndoState(MainFrm);
}

void CCompoundAction::SaveRedoState(const CMainFrame &MainFrm)
{
	m_pActionList.back()->SaveRedoState(MainFrm);
}

void CCompoundAction::RestoreUndoState(CMainFrame &MainFrm) const
{
	m_pActionList.front()->RestoreUndoState(MainFrm);
}

void CCompoundAction::RestoreRedoState(CMainFrame &pMainFrm) const
{
	m_pActionList.back()->RestoreRedoState(pMainFrm);
}

void CCompoundAction::JoinAction(std::unique_ptr<CAction> pAction)
{
	m_pActionList.push_back(std::move(pAction));
}
