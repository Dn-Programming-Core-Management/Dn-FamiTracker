/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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


#pragma once

#include <vector>
#include <memory>

// Undo / redo helper class

//
// Change MAX_LEVELS in the class History if you want more undo levels
//

class CMainFrame;		// // //

// Base class for action commands
class Action
{
public:
	virtual ~Action();

	// // // Save the action-specific state information. This method may reject the action by returning false
	virtual bool SaveState(const CMainFrame *pMainFrm) = 0;

	// Undo the operation
	virtual void Undo(CMainFrame *pMainFrm) const = 0;

	// Redo the operation
	virtual void Redo(CMainFrame *pMainFrm) const = 0;

	// // // Save the undo state before performing the action
	virtual void SaveUndoState(const CMainFrame *pMainFrm) = 0;

	// // // Save the redo state after performing the action
	virtual void SaveRedoState(const CMainFrame *pMainFrm) = 0;

	// // // Restore the state just before the action
	virtual void RestoreUndoState(CMainFrame *pMainFrm) const = 0;

	// // // Restore the state just after the action
	virtual void RestoreRedoState(CMainFrame *pMainFrm) const = 0;

	// // // Combine current action with another one, return true if permissible
	virtual bool Merge(const Action *Other);

	// Get the action type
	int GetAction() const;

protected:
	Action(int iAction = -1);		// // //

	int m_iAction;
};

// Stores action objects (a dual-stack, kind of)
class History
{
public:
	History();

	// Clear the undo list
	void Clear();

	// Add new action to undo list
	void Push(Action *pAction);

	// Get first undo action object in queue
	Action *PopUndo();

	// Get first redo action object in queue
	Action *PopRedo();

	// Return last action in queue without changing the queue
	Action *GetLastAction() const;

	// Get number of undo levels available
	int GetUndoLevel() const;

	// Get number of redo levels available
	int GetRedoLevel() const;

	// Returns true if there are undo objects available
	bool CanUndo() const;

	// Returns true if there are redo objects available
	bool CanRedo() const;

public:
	// Levels of undo in the editor. Higher value requires more memory
	static const int MAX_LEVELS;

private:
	std::vector<std::unique_ptr<Action>> m_UndoStack, m_RedoStack;
};

