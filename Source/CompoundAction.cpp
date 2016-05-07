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

#include "stdafx.h"
#include "CompoundAction.h"

// // //
// Compound action class
// // //

CCompoundAction::CCompoundAction() :
	m_pActionList(new std::vector<std::unique_ptr<CAction>>()),
	m_bFirst(true)
{
}

bool CCompoundAction::SaveState(const CMainFrame *pMainFrm)
{
	return (*m_pActionList->begin())->SaveState(pMainFrm);;
}

void CCompoundAction::Undo(CMainFrame *pMainFrm) const
{
	for (auto it = m_pActionList->rbegin(); it != m_pActionList->rend(); ++it)
		(*it)->Undo(pMainFrm);
}

void CCompoundAction::Redo(CMainFrame *pMainFrm) const
{
	auto it = m_pActionList->begin();
	const auto end = m_pActionList->end();
	while (true) {
		(*it)->Redo(pMainFrm);
		if (++it == end) break;
		if (!m_bFirst) continue;
		(*it)->SaveUndoState(pMainFrm);
		if (!(*it)->SaveState(pMainFrm)) {
			m_pActionList->erase(it, end);
			throw new std::runtime_error(_T("Unable to create action."));
		}
	}
	m_bFirst = false;
}

void CCompoundAction::SaveUndoState(const CMainFrame *pMainFrm)		// // //
{
	(*m_pActionList->begin())->SaveUndoState(pMainFrm);
}

void CCompoundAction::SaveRedoState(const CMainFrame *pMainFrm)		// // //
{
	(*m_pActionList->rbegin())->SaveRedoState(pMainFrm);
}

void CCompoundAction::RestoreUndoState(CMainFrame *pMainFrm) const		// // //
{
	(*m_pActionList->begin())->RestoreUndoState(pMainFrm);
}

void CCompoundAction::RestoreRedoState(CMainFrame *pMainFrm) const		// // //
{
	(*m_pActionList->rbegin())->RestoreRedoState(pMainFrm);
}

void CCompoundAction::JoinAction(CAction *const pAction)
{
	m_pActionList->push_back(std::unique_ptr<CAction>(pAction));
}
