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

#define GET_VIEW() static_cast<CFamiTrackerView *>(MainFrm.GetActiveView())
#define GET_DOCUMENT() (*GET_VIEW()->GetDocument())

void CModuleAction::SaveUndoState(const CMainFrame &MainFrm) {
}

void CModuleAction::SaveRedoState(const CMainFrame &MainFrm) {
}

void CModuleAction::RestoreUndoState(CMainFrame &MainFrm) const {
}

void CModuleAction::RestoreRedoState(CMainFrame &MainFrm) const {
}



bool ModuleAction::CComment::SaveState(const CMainFrame &MainFrm) {
	auto &doc = GET_DOCUMENT();
	oldComment_ = doc.GetComment();
	oldShow_ = doc.ShowCommentOnOpen();
	return true; // no merge because the comment dialog is modal
}

void ModuleAction::CComment::Undo(CMainFrame &MainFrm) {
	auto &doc = GET_DOCUMENT();
	doc.SetComment(oldComment_, oldShow_);
	MainFrm.SetMessageText(_T("Comment settings changed"));
}

void ModuleAction::CComment::Redo(CMainFrame &MainFrm) {
	auto &doc = GET_DOCUMENT();
	doc.SetComment(newComment_, newShow_);
	MainFrm.SetMessageText(_T("Comment settings changed"));
}



ModuleAction::CTitle::CTitle(std::string_view str) :
	newStr_(str.substr(0, METADATA_FIELD_LENGTH - 1))
{
}

bool ModuleAction::CTitle::SaveState(const CMainFrame &MainFrm) {
	oldStr_ = GET_DOCUMENT().GetModuleName();
	return newStr_ != oldStr_;
}

void ModuleAction::CTitle::Undo(CMainFrame &MainFrm) {
	auto &doc = GET_DOCUMENT();
	doc.SetModuleName(oldStr_);
	MainFrm.SetSongInfo(doc);
}

void ModuleAction::CTitle::Redo(CMainFrame &MainFrm) {
	auto &doc = GET_DOCUMENT();
	doc.SetModuleName(newStr_);
	MainFrm.SetSongInfo(doc);
}

bool ModuleAction::CTitle::Merge(const CAction &other) {
	if (auto pAction = dynamic_cast<const CTitle *>(&other)) {
		newStr_ = pAction->newStr_;
		return true;
	}
	return false;
}



ModuleAction::CArtist::CArtist(std::string_view str) :
	newStr_(str.substr(0, METADATA_FIELD_LENGTH - 1))
{
}

bool ModuleAction::CArtist::SaveState(const CMainFrame &MainFrm) {
	oldStr_ = GET_DOCUMENT().GetModuleArtist();
	return newStr_ != oldStr_;
}

void ModuleAction::CArtist::Undo(CMainFrame &MainFrm) {
	auto &doc = GET_DOCUMENT();
	doc.SetModuleArtist(oldStr_);
	MainFrm.SetSongInfo(doc);
}

void ModuleAction::CArtist::Redo(CMainFrame &MainFrm) {
	auto &doc = GET_DOCUMENT();
	doc.SetModuleArtist(newStr_);
	MainFrm.SetSongInfo(doc);
}

bool ModuleAction::CArtist::Merge(const CAction &other) {
	if (auto pAction = dynamic_cast<const CArtist *>(&other)) {
		newStr_ = pAction->newStr_;
		return true;
	}
	return false;
}



ModuleAction::CCopyright::CCopyright(std::string_view str) :
	newStr_(str.substr(0, METADATA_FIELD_LENGTH - 1))
{
}

bool ModuleAction::CCopyright::SaveState(const CMainFrame &MainFrm) {
	oldStr_ = GET_DOCUMENT().GetModuleCopyright();
	return newStr_ != oldStr_;
}

void ModuleAction::CCopyright::Undo(CMainFrame &MainFrm) {
	auto &doc = GET_DOCUMENT();
	doc.SetModuleCopyright(oldStr_);
	MainFrm.SetSongInfo(doc);
}

void ModuleAction::CCopyright::Redo(CMainFrame &MainFrm) {
	auto &doc = GET_DOCUMENT();
	doc.SetModuleCopyright(newStr_);
	MainFrm.SetSongInfo(doc);
}

bool ModuleAction::CCopyright::Merge(const CAction &other) {
	if (auto pAction = dynamic_cast<const CCopyright *>(&other)) {
		newStr_ = pAction->newStr_;
		return true;
	}
	return false;
}
