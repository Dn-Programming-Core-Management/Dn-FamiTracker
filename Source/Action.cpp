/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#include "stdafx.h"
#include "resource.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "PatternEditor.h"
#include "Action.h"

// CAction ////////////////////////////////////////////////////////////////////////

CAction::CAction(int iAction)
{
	m_iAction = iAction;
}

CAction::~CAction()
{
}

int CAction::GetAction() const
{
	return m_iAction;
}


// CActionHandler /////////////////////////////////////////////////////////////////

CActionHandler::CActionHandler()
{
	m_iUndoLevel = 0;
	m_iRedoLevel = 0;

	for (int i = 0; i < MAX_LEVELS; ++i) {
		m_pActionStack[i] = NULL;
	}
}

CActionHandler::~CActionHandler()
{
	Clear();
}

void CActionHandler::Clear()
{
	m_iUndoLevel = 0;
	m_iRedoLevel = 0;

	for (int i = 0; i < MAX_LEVELS; ++i) {
		SAFE_RELEASE(m_pActionStack[i]);
	}
}

void CActionHandler::Push(CAction *pAction)
{
	if (m_iUndoLevel < MAX_LEVELS) {
		SAFE_RELEASE(m_pActionStack[m_iUndoLevel]);
		m_pActionStack[m_iUndoLevel++] = pAction;
	}
	else {
		SAFE_RELEASE(m_pActionStack[0]);
		for (int i = 1; i < MAX_LEVELS; ++i)
			m_pActionStack[i - 1] = m_pActionStack[i];
		m_pActionStack[MAX_LEVELS - 1] = pAction;
	}

	m_iRedoLevel = 0;
}

CAction *CActionHandler::PopUndo()
{
	if (!m_iUndoLevel)
		return NULL;

	m_iRedoLevel++;
	m_iUndoLevel--;

	return m_pActionStack[m_iUndoLevel];
}

CAction *CActionHandler::PopRedo()
{
	if (!m_iRedoLevel)
		return NULL;

	m_iUndoLevel++;
	m_iRedoLevel--;

	return m_pActionStack[m_iUndoLevel - 1];
}

CAction *CActionHandler::GetLastAction() const
{
	return (m_iUndoLevel == 0) ? NULL : m_pActionStack[m_iUndoLevel - 1];
}

int CActionHandler::GetUndoLevel() const
{
	return m_iUndoLevel;
}

int CActionHandler::GetRedoLevel() const
{
	return m_iRedoLevel;
}

bool CActionHandler::CanUndo() const
{
	return m_iUndoLevel > 0;
}

bool CActionHandler::CanRedo() const
{
	return m_iRedoLevel > 0;
}
