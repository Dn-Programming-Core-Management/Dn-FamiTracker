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
#include <string>

// // // global document actions

class CModuleAction : public CAction {
	void SaveUndoState(const CMainFrame &MainFrm) override;
	void SaveRedoState(const CMainFrame &MainFrm) override;
	void RestoreUndoState(CMainFrame &MainFrm) const override;
	void RestoreRedoState(CMainFrame &MainFrm) const override;
};

namespace ModuleAction {

class CComment : public ::CModuleAction {
public:
	CComment(const std::string &comment, bool show) :
		newComment_(comment), newShow_(show) {
	}
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) const override;
	void Redo(CMainFrame &MainFrm) const override;

	std::string oldComment_;
	std::string newComment_;
	bool oldShow_ = false;
	bool newShow_ = false;
};

} // namespace ModuleAction
