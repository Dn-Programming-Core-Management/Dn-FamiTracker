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

#include <vector>
#include <memory>

class CAction;
class CMainFrame;
class CFamiTrackerDoc;

// // // Stores action objects (a dual-stack, kind of)
class CActionHandler
{
public:
	CActionHandler() = default;

	// Clear the undo list
	void Clear();

	// Add new action to undo list
	void Push(std::unique_ptr<CAction> pAction);		// // //
	
	void UndoLastAction(CMainFrame &cxt);		// // //
	void RedoLastAction(CMainFrame &cxt);		// // //

	// // // Returns true if actions are lost due to undo level exceeding limit
	bool ActionsLost() const;

	// Returns true if there are undo objects available
	bool CanUndo() const;

	// Returns true if there are redo objects available
	bool CanRedo() const;

public:
	// Levels of undo in the editor. Higher value requires more memory
	static const int MAX_LEVELS;

private:
	std::vector<std::unique_ptr<CAction>> m_UndoStack;
	std::vector<std::unique_ptr<CAction>> m_RedoStack;
	bool lost_ = false;
};
