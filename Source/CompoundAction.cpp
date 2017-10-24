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

CCompoundAction::CCompoundAction(std::initializer_list<CAction *> list)
{
	for (auto x : list)
		m_pActionList.emplace_back(x);
}

bool CCompoundAction::SaveState(const CMainFrame &MainFrm)
{
	if (m_pActionList.empty())
		return false;
	return (*m_pActionList.begin())->SaveState(MainFrm);
}

void CCompoundAction::Undo(CMainFrame &MainFrm) const
{
	for (auto it = m_pActionList.rbegin(); it != m_pActionList.rend(); ++it)
		(*it)->Undo(MainFrm);
}

void CCompoundAction::Redo(CMainFrame &MainFrm) const
{
	auto it = m_pActionList.begin();
	const auto end = m_pActionList.end();
	while (true) {
		(*it)->Redo(MainFrm);
		if (++it == end)
			break;
		if (!m_bFirst)
			continue;
		(*it)->SaveUndoState(MainFrm);
		if (!(*it)->SaveState(MainFrm))
			throw new std::runtime_error("Unable to create action.");
	}
	m_bFirst = false;
}

void CCompoundAction::SaveUndoState(const CMainFrame &MainFrm)
{
	if (!m_pActionList.empty())
		m_pActionList.front()->SaveUndoState(MainFrm);
}

void CCompoundAction::SaveRedoState(const CMainFrame &MainFrm)
{
	if (!m_pActionList.empty())
		m_pActionList.back()->SaveRedoState(MainFrm);
}

void CCompoundAction::RestoreUndoState(CMainFrame &MainFrm) const
{
	if (!m_pActionList.empty())
		m_pActionList.front()->RestoreUndoState(MainFrm);
}

void CCompoundAction::RestoreRedoState(CMainFrame &pMainFrm) const
{
	if (!m_pActionList.empty())
		m_pActionList.back()->RestoreRedoState(pMainFrm);
}

void CCompoundAction::JoinAction(std::unique_ptr<CAction> pAction)
{
	m_pActionList.push_back(std::move(pAction));
}
