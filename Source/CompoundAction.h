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


#pragma once

#include "Action.h"
#include <vector>
#include <memory>
#include <initializer_list>

/*!
	\brief A compound action class allowing multiple undoable actions to be chained together.
	\details The compound uniquely manages the action objects it contains.
*/
class CCompoundAction final : public Action
{
public:
	CCompoundAction() = default;
	CCompoundAction(std::initializer_list<Action*> list);

	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;

	void SaveUndoState(const CMainFrame *pMainFrm);		// // //
	void SaveRedoState(const CMainFrame *pMainFrm);		// // //
	void RestoreUndoState(CMainFrame *pMainFrm) const;		// // //
	void RestoreRedoState(CMainFrame *pMainFrm) const;		// // //

	/*!	\brief Adds an action to the compound to be performed last (and undoed first).
		\param pAction Pointer to the action object. */
	void JoinAction(Action *const pAction);

private:
	std::vector<std::unique_ptr<Action>> m_pActionList { };
	mutable bool m_bFirst = true;
};
