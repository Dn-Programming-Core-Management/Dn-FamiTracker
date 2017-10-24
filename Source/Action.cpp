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

// CAction ////////////////////////////////////////////////////////////////////////

bool CAction::Merge(const CAction *Other)		// // //
{
	return false;
}


// CActionHandler /////////////////////////////////////////////////////////////////

const int CActionHandler::MAX_LEVELS = 64;		// // //

void CActionHandler::Clear()
{
	m_UndoStack.clear();
	m_RedoStack.clear();
}

void CActionHandler::Push(CAction *pAction)
{
	auto ptr = std::unique_ptr<CAction>(pAction);		// // //
	if (m_UndoStack.empty() || !m_UndoStack.back()->Merge(pAction)) {
		if (m_UndoStack.size() == MAX_LEVELS)
			m_UndoStack.erase(m_UndoStack.begin());
		m_UndoStack.push_back(std::move(ptr));
	}
	m_RedoStack.clear();
}

CAction *CActionHandler::PopUndo()
{
	if (m_UndoStack.empty())
		return nullptr;

	auto &pAction = m_RedoStack.emplace_back(std::move(m_UndoStack.back()));
	m_UndoStack.pop_back();
	return pAction.get();
}

CAction *CActionHandler::PopRedo()
{
	if (m_RedoStack.empty())
		return nullptr;

	auto &pAction = m_UndoStack.emplace_back(std::move(m_RedoStack.back()));
	m_RedoStack.pop_back();
	return pAction.get();
}

CAction *CActionHandler::GetLastAction() const
{
	return m_UndoStack.empty() ? nullptr : m_UndoStack.back().get();
}

int CActionHandler::GetUndoLevel() const
{
	return m_UndoStack.size();
}

int CActionHandler::GetRedoLevel() const
{
	return m_RedoStack.size();
}

bool CActionHandler::CanUndo() const
{
	return !m_UndoStack.empty();
}

bool CActionHandler::CanRedo() const
{
	return !m_RedoStack.empty();
}
