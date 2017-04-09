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

CAction::CAction(int iAction) : m_iAction(iAction)
{
}

CAction::~CAction()
{
}

bool CAction::Merge(const CAction *Other)		// // //
{
	return false;
}

int CAction::GetAction() const
{
	return m_iAction;
}


// CActionHandler /////////////////////////////////////////////////////////////////

const int CActionHandler::MAX_LEVELS = 64;		// // //

CActionHandler::CActionHandler() : m_UndoStack(), m_RedoStack()
{
}

void CActionHandler::Clear()
{
	m_UndoStack.clear();
	m_RedoStack.clear();
}

void CActionHandler::Push(CAction *pAction)
{
	auto ptr = std::unique_ptr<CAction>(pAction);		// // //
	if (m_UndoStack.empty() || !(*m_UndoStack.rbegin())->Merge(pAction)) {
		if (m_UndoStack.size() == MAX_LEVELS)
			m_UndoStack.erase(m_UndoStack.begin(), m_UndoStack.begin() + 1);
		m_UndoStack.push_back(std::move(ptr));
	}
	m_RedoStack.clear();
}

CAction *CActionHandler::PopUndo()
{
	if (m_UndoStack.empty())
		return nullptr;

	CAction *pAction = m_UndoStack.rbegin()->release();
	m_UndoStack.erase(m_UndoStack.end() - 1, m_UndoStack.end());
	m_RedoStack.push_back(std::unique_ptr<CAction>(pAction));
	return pAction;
}

CAction *CActionHandler::PopRedo()
{
	if (m_RedoStack.empty())
		return nullptr;

	CAction *pAction = m_RedoStack.rbegin()->release();
	m_RedoStack.erase(m_RedoStack.end() - 1, m_RedoStack.end());
	m_UndoStack.push_back(std::unique_ptr<CAction>(pAction));
	return pAction;
}

CAction *CActionHandler::GetLastAction() const
{
	return m_UndoStack.empty() ? nullptr : m_UndoStack.rbegin()->get();
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
