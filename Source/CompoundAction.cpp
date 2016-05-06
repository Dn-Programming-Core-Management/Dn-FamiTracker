/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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

CCompoundAction::CCompoundAction() : m_pActionList()
{
}

bool CCompoundAction::SaveState(CMainFrame *pMainFrm)
{
	for (auto it = m_pActionList.begin(); it != m_pActionList.end(); ++it)
		if (!(*it)->SaveState(pMainFrm)) // TODO: check if all compound actions can be undone in any order
			return false;
	return true;
}

void CCompoundAction::Undo(CMainFrame *pMainFrm)
{
	for (auto it = m_pActionList.rbegin(); it != m_pActionList.rend(); ++it)
		(*it)->Undo(pMainFrm);
}

void CCompoundAction::Redo(CMainFrame *pMainFrm)
{
	for (auto it = m_pActionList.begin(); it != m_pActionList.end(); ++it)
		(*it)->Redo(pMainFrm);
}

void CCompoundAction::SaveUndoState(const CMainFrame *pMainFrm)		// // //
{
	(*m_pActionList.begin())->SaveUndoState(pMainFrm);
}

void CCompoundAction::SaveRedoState(const CMainFrame *pMainFrm)		// // //
{
	(*m_pActionList.rbegin())->SaveRedoState(pMainFrm);
}

void CCompoundAction::RestoreUndoState(CMainFrame *pMainFrm) const		// // //
{
	(*m_pActionList.begin())->RestoreUndoState(pMainFrm);
}

void CCompoundAction::RestoreRedoState(CMainFrame *pMainFrm) const		// // //
{
	(*m_pActionList.rbegin())->RestoreRedoState(pMainFrm);
}

void CCompoundAction::JoinAction(CAction *const pAction)
{
	m_pActionList.push_back(std::unique_ptr<CAction>(pAction));
}
