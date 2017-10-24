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

#include "Action.h"
#include <vector>
#include <memory>
#include <initializer_list>

/*!
	\brief A compound action class allowing multiple undoable actions to be chained together.
	\details The compound uniquely manages the action objects it contains.
*/
class CCompoundAction final : public CAction		// // //
{
public:
	CCompoundAction() = default;
	CCompoundAction(std::initializer_list<CAction*> list);

	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) const override;
	void Redo(CMainFrame &MainFrm) const override;

	void SaveUndoState(const CMainFrame &MainFrm) override;
	void SaveRedoState(const CMainFrame &MainFrm) override;
	void RestoreUndoState(CMainFrame &MainFrm) const override;
	void RestoreRedoState(CMainFrame &MainFrm) const override;

	/*!	\brief Adds an action to the compound to be performed last (and undone first).
		\param pAction Pointer to the action object. */
	void JoinAction(std::unique_ptr<CAction> pAction);

private:
	std::vector<std::unique_ptr<CAction>> m_pActionList;
	mutable bool m_bFirst = true;
};
