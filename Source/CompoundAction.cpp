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

#include "stdafx.h"
#include "CompoundAction.h"
#include <stdexcept>

// // //
// Compound action class
// // //

CCompoundAction::CCompoundAction(std::initializer_list<Action*> list)
{
	for (auto x : list)
		m_pActionList.emplace_back(x);
}

bool CCompoundAction::SaveState(const CMainFrame *pMainFrm)
{
	if (m_pActionList.empty())
		return false;
	return (*m_pActionList.begin())->SaveState(pMainFrm);
}

void CCompoundAction::Undo(CMainFrame *pMainFrm) const
{
	for (auto it = m_pActionList.rbegin(); it != m_pActionList.rend(); ++it)
		(*it)->Undo(pMainFrm);
}

void CCompoundAction::Redo(CMainFrame *pMainFrm) const
{
	auto it = m_pActionList.begin();
	const auto end = m_pActionList.end();
	while (true) {
		(*it)->Redo(pMainFrm);
		if (++it == end) break;
		if (!m_bFirst) continue;
		(*it)->SaveUndoState(pMainFrm);
		if (!(*it)->SaveState(pMainFrm))
			throw new std::runtime_error(_T("Unable to create action."));
	}
	m_bFirst = false;
}

void CCompoundAction::SaveUndoState(const CMainFrame *pMainFrm)		// // //
{
	if (!m_pActionList.empty())
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

void CCompoundAction::JoinAction(Action *const pAction)
{
	m_pActionList.emplace_back(pAction);
}
