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


#pragma once

class CMainFrame;		// // //

// Base class for action commands
class CAction
{
public:
	virtual ~CAction() = default;

	// // // Save the action-specific state information. This method may reject the action by returning false
	virtual bool SaveState(const CMainFrame &MainFrm) = 0;

	// Undo the operation
	virtual void Undo(CMainFrame &MainFrm) const = 0;

	// Redo the operation
	virtual void Redo(CMainFrame &MainFrm) const = 0;

	// // // Save the undo state before performing the action
	virtual void SaveUndoState(const CMainFrame &MainFrm) = 0;

	// // // Save the redo state after performing the action
	virtual void SaveRedoState(const CMainFrame &MainFrm) = 0;

	// // // Restore the state just before the action
	virtual void RestoreUndoState(CMainFrame &MainFrm) const = 0;

	// // // Restore the state just after the action
	virtual void RestoreRedoState(CMainFrame &MainFrm) const = 0;

	// // // Combine current action with another one, return true if permissible
	virtual bool Merge(const CAction &Other);
};
