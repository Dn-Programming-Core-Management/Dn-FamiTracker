/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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

/*!
	\brief A compound action class allowing multiple undoable actions to be chained together.
	\details The compound uniquely manages the action objects it contains.
*/
class CCompoundAction final : public CAction
{
public:
	CCompoundAction();

	virtual bool SaveState(CMainFrame *pMainFrm);
	virtual void Undo(CMainFrame *pMainFrm);
	virtual void Redo(CMainFrame *pMainFrm);

	/*!	\brief Adds an action to the compound to be performed last (and undoed first).
		\param pAction Pointer to the action object. */
	void JoinAction(CAction *const pAction);

private:
	std::vector<std::unique_ptr<CAction>> m_pActionList;
};
