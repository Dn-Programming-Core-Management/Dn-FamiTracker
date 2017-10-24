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

#include "ModuleAction.h"
#include "MainFrm.h"
#include "FamiTrackerView.h"
#include "FamiTrackerDoc.h"

void CModuleAction::SaveUndoState(const CMainFrame &MainFrm) {
}

void CModuleAction::SaveRedoState(const CMainFrame &MainFrm) {
}

void CModuleAction::RestoreUndoState(CMainFrame &MainFrm) const {
}

void CModuleAction::RestoreRedoState(CMainFrame &MainFrm) const {
}



bool ModuleAction::CComment::SaveState(const CMainFrame &MainFrm) {
	auto pDoc = static_cast<CFamiTrackerView*>(MainFrm.GetActiveView())->GetDocument();
	oldComment_ = pDoc->GetComment();
	oldShow_ = pDoc->ShowCommentOnOpen();
	return true; // no merge because the comment dialog is modal
}

void ModuleAction::CComment::Undo(CMainFrame &MainFrm) const {
	auto pDoc = static_cast<CFamiTrackerView*>(MainFrm.GetActiveView())->GetDocument();
	pDoc->SetComment(oldComment_, oldShow_);
	MainFrm.SetMessageText(_T("Comment settings changed"));
}

void ModuleAction::CComment::Redo(CMainFrame &MainFrm) const {
	auto pDoc = static_cast<CFamiTrackerView*>(MainFrm.GetActiveView())->GetDocument();
	pDoc->SetComment(newComment_, newShow_);
	MainFrm.SetMessageText(_T("Comment settings changed"));
}
