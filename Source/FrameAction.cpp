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

#include "FrameAction.h"
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "FrameEditor.h"
#include "FrameClipData.h"		// // //

// // // all dependencies on CMainFrame
#define GET_VIEW() static_cast<CFamiTrackerView *>(MainFrm.GetActiveView())
#define GET_DOCUMENT() GET_VIEW()->GetDocument()
#define GET_FRAME_EDITOR() MainFrm.GetFrameEditor()
#define GET_SELECTED_TRACK() MainFrm.GetSelectedTrack()
#define UPDATE_CONTROLS() MainFrm.UpdateControls()

// // // Frame editor state class

CFrameEditorState::CFrameEditorState(const CFamiTrackerView *pView, int Track) :		// // //
	Track(Track),
	Cursor {static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor()->GetEditFrame(), pView->GetSelectedChannel()},
	OriginalSelection(static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor()->GetSelection()),
	IsSelecting(static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor()->IsSelecting())
{
	Selection = OriginalSelection.GetNormalized();
}

void CFrameEditorState::ApplyState(CFamiTrackerView *pView) const
{
	auto pEditor = static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor();
	pEditor->SetEditFrame(Cursor.m_iFrame);
	pView->SelectChannel(Cursor.m_iChannel);
	IsSelecting ? pEditor->SetSelection(OriginalSelection) : pEditor->CancelSelection();
}

int CFrameEditorState::GetFrameStart() const
{
	return IsSelecting ? Selection.GetFrameStart() : Cursor.m_iFrame;
}

int CFrameEditorState::GetFrameEnd() const
{
	return IsSelecting ? Selection.GetFrameEnd() : Cursor.m_iFrame;
}

int CFrameEditorState::GetChanStart() const
{
	return IsSelecting ? Selection.GetChanStart() : Cursor.m_iChannel;
}

int CFrameEditorState::GetChanEnd() const
{
	return IsSelecting ? Selection.GetChanEnd() : Cursor.m_iChannel;
}

#define STATE_EXPAND(st) (st)->Track, (st)->Cursor.m_iFrame, (st)->Cursor.m_iChannel



// CFrameAction ///////////////////////////////////////////////////////////////////
//
// Undo/redo commands for frame editor
//

CFrameAction::~CFrameAction()
{
	SAFE_RELEASE(m_pUndoState);		// // //
	SAFE_RELEASE(m_pRedoState);		// // //
}

int CFrameAction::ClipPattern(int Pattern)
{
	if (Pattern < 0)
		Pattern = 0;
	if (Pattern > MAX_PATTERN - 1)
		Pattern = MAX_PATTERN - 1;

	return Pattern;
}

void CFrameAction::SaveUndoState(const CMainFrame &MainFrm)		// // //
{
	CFamiTrackerView *pView = GET_VIEW();
	m_pUndoState = new CFrameEditorState {pView, GET_SELECTED_TRACK()};		// // //
	m_itFrames = make_int_range(m_pUndoState->GetFrameStart(), m_pUndoState->GetFrameEnd());
	m_itChannels = make_int_range(m_pUndoState->GetChanStart(), m_pUndoState->GetChanEnd());
}

void CFrameAction::SaveRedoState(const CMainFrame &MainFrm)		// // //
{
	CFamiTrackerView *pView = GET_VIEW();
	m_pRedoState = new CFrameEditorState {pView, GET_SELECTED_TRACK()};		// // //
	pView->GetDocument()->UpdateAllViews(NULL, UPDATE_FRAME);
}

void CFrameAction::RestoreUndoState(CMainFrame &MainFrm) const		// // //
{
	CFamiTrackerView *pView = GET_VIEW();
	m_pUndoState->ApplyState(pView);
	pView->GetDocument()->UpdateAllViews(NULL, UPDATE_FRAME);
}

void CFrameAction::RestoreRedoState(CMainFrame &MainFrm) const		// // //
{
	CFamiTrackerView *pView = GET_VIEW();
	m_pRedoState->ApplyState(pView);
	pView->GetDocument()->UpdateAllViews(NULL, UPDATE_FRAME);
}



// // // built-in frame action subtypes



bool CFActionAddFrame::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	return pDoc->GetFrameCount(m_pUndoState->Track) < MAX_FRAMES;
}

void CFActionAddFrame::Undo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->RemoveFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1);
}

void CFActionAddFrame::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->InsertFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1);
}



CFActionRemoveFrame::~CFActionRemoveFrame() {
	SAFE_RELEASE(m_pRowClipData);
}

bool CFActionRemoveFrame::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	if (pDoc->GetFrameCount(m_pUndoState->Track) <= 1)
		return false;
	m_pRowClipData = GET_FRAME_EDITOR()->CopyFrame(m_pUndoState->Cursor.m_iFrame);
	return true;
}

void CFActionRemoveFrame::Undo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->InsertFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
	GET_FRAME_EDITOR()->PasteInsert(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, m_pRowClipData);
}

void CFActionRemoveFrame::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->RemoveFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}



bool CFActionDuplicateFrame::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	return pDoc->GetFrameCount(m_pUndoState->Track) < MAX_FRAMES;
}

void CFActionDuplicateFrame::Undo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->RemoveFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}

void CFActionDuplicateFrame::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->DuplicateFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}



bool CFActionCloneFrame::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	return pDoc->GetFrameCount(m_pUndoState->Track) < MAX_FRAMES;
}

void CFActionCloneFrame::Undo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	int Count = pDoc->GetChannelCount();
	for (int i = 0; i < Count; ++i)
		pDoc->ClearPattern(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1, i);
	pDoc->RemoveFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1);
}

void CFActionCloneFrame::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->CloneFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1);
}



bool CFActionFrameCount::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	m_iOldFrameCount = pDoc->GetFrameCount(m_pUndoState->Track);
	return m_iNewFrameCount != m_iOldFrameCount;
}

void CFActionFrameCount::Undo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->SetFrameCount(m_pUndoState->Track, m_iOldFrameCount);
	UPDATE_CONTROLS();
}

void CFActionFrameCount::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->SetFrameCount(m_pUndoState->Track, m_iNewFrameCount);
	UPDATE_CONTROLS();
}

bool CFActionFrameCount::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionFrameCount *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewFrameCount = pAction->m_iNewFrameCount;
	return true;
}



CFActionSetPattern::~CFActionSetPattern() {
	SAFE_RELEASE(m_pClipData);
}

bool CFActionSetPattern::SaveState(const CMainFrame &MainFrm)
{
	m_pClipData = GET_FRAME_EDITOR()->Copy();
	return true;
}

void CFActionSetPattern::Undo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->PasteAt(m_pUndoState->Track, m_pClipData, m_pUndoState->Selection.m_cpStart);
}

void CFActionSetPattern::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	for (int f : m_itFrames) for (int c : m_itChannels)
		pDoc->SetPatternAtFrame(m_pUndoState->Track, f, c, m_iNewPattern);
}

bool CFActionSetPattern::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionSetPattern *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track ||
		m_itFrames != pAction->m_itFrames || m_itChannels != pAction->m_itChannels)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewPattern = pAction->m_iNewPattern;
	return true;
}



CFActionSetPatternAll::~CFActionSetPatternAll() {
	SAFE_RELEASE(m_pRowClipData);
}

bool CFActionSetPatternAll::SaveState(const CMainFrame &MainFrm)
{
	m_pRowClipData = GET_FRAME_EDITOR()->CopyFrame(m_pUndoState->Cursor.m_iFrame);
	return true;
}

void CFActionSetPatternAll::Undo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->PasteAt(m_pUndoState->Track, m_pRowClipData, m_pUndoState->Cursor);
}

void CFActionSetPatternAll::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	for (int i = 0; i < m_pRowClipData->ClipInfo.Channels; ++i)
		pDoc->SetPatternAtFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, i, m_iNewPattern);
}

bool CFActionSetPatternAll::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionSetPatternAll *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track ||
		m_pUndoState->Cursor.m_iFrame != pAction->m_pUndoState->Cursor.m_iFrame)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewPattern = pAction->m_iNewPattern;
	return true;
}



CFActionChangePattern::~CFActionChangePattern() {
	SAFE_RELEASE(m_pClipData);
}

bool CFActionChangePattern::SaveState(const CMainFrame &MainFrm)
{
	if (!m_iPatternOffset)
		return false;
	m_pClipData = GET_FRAME_EDITOR()->Copy();
	return true;
}

void CFActionChangePattern::Undo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->PasteAt(m_pUndoState->Track, m_pClipData, m_pUndoState->Selection.m_cpStart);
}

void CFActionChangePattern::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	const int f0 = m_pUndoState->GetFrameStart();
	const int c0 = m_pUndoState->IsSelecting ? m_pUndoState->GetChanStart() : 0; // copies entire row by default
	for (int f : m_itFrames) for (int c : m_itChannels) {
		int Frame = m_pClipData->GetFrame(f - f0, c - c0);
		int NewFrame = ClipPattern(Frame + m_iPatternOffset);
		if (NewFrame == Frame)
			m_bOverflow = true;
		pDoc->SetPatternAtFrame(m_pUndoState->Track, f, c, NewFrame);
	}
}

bool CFActionChangePattern::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionChangePattern *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track ||
		m_itFrames != pAction->m_itFrames || m_itChannels != pAction->m_itChannels)
		return false;
	if (m_bOverflow && m_iPatternOffset * pAction->m_iPatternOffset < 0) // different directions
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iPatternOffset += pAction->m_iPatternOffset;
	if (pAction->m_bOverflow)
		m_bOverflow = true;
	return true;
}



CFActionChangePatternAll::~CFActionChangePatternAll() {
	SAFE_RELEASE(m_pRowClipData);
}

bool CFActionChangePatternAll::SaveState(const CMainFrame &MainFrm)
{
	if (!m_iPatternOffset)
		return false;
	m_pRowClipData = GET_FRAME_EDITOR()->CopyFrame(m_pUndoState->Cursor.m_iFrame);
	return true;
}

void CFActionChangePatternAll::Undo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->PasteAt(m_pUndoState->Track, m_pRowClipData, m_pUndoState->Cursor);
}

void CFActionChangePatternAll::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	for (int i = 0; i < m_pRowClipData->ClipInfo.Channels; ++i) {
		int Frame = m_pRowClipData->GetFrame(0, i);
		int NewFrame = ClipPattern(m_pRowClipData->GetFrame(0, i) + m_iPatternOffset);
		if (NewFrame == Frame)
			m_bOverflow = true;
		pDoc->SetPatternAtFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, i, NewFrame);
	}
}

bool CFActionChangePatternAll::Merge(const CAction &Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionChangePatternAll *>(&Other);
	if (!pAction)
		return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track ||
		m_pUndoState->Cursor.m_iFrame != pAction->m_pUndoState->Cursor.m_iFrame)
		return false;
	if (m_bOverflow && m_iPatternOffset * pAction->m_iPatternOffset < 0) // different directions
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iPatternOffset += pAction->m_iPatternOffset;
	if (pAction->m_bOverflow)
		m_bOverflow = true;
	return true;
}



bool CFActionMoveDown::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	return m_pUndoState->Cursor.m_iFrame < static_cast<int>(pDoc->GetFrameCount(m_pUndoState->Track)) - 1;
}

void CFActionMoveDown::Undo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->MoveFrameUp(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1);
}

void CFActionMoveDown::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->MoveFrameDown(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}



bool CFActionMoveUp::SaveState(const CMainFrame &MainFrm)
{
	return m_pUndoState->Cursor.m_iFrame > 0;
}

void CFActionMoveUp::Undo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->MoveFrameDown(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame - 1);
}

void CFActionMoveUp::Redo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	pDoc->MoveFrameUp(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}



CFActionPaste::~CFActionPaste() {
	SAFE_RELEASE(m_pClipData);
}

bool CFActionPaste::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	return m_pClipData->ClipInfo.Channels <= pDoc->GetChannelCount() &&
		m_pClipData->ClipInfo.Frames + pDoc->GetFrameCount(m_pUndoState->Track) <= MAX_FRAMES;

}

void CFActionPaste::Undo(CMainFrame &MainFrm)
{
	CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	if (m_bClone)
		GET_FRAME_EDITOR()->ClearPatterns(m_pUndoState->Track, m_pRedoState->Selection);		// // //
	pDoc->DeleteFrames(m_pUndoState->Track, m_iTargetFrame, m_pClipData->ClipInfo.Frames);
	UPDATE_CONTROLS();
}

void CFActionPaste::Redo(CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	if (m_bClone)
		pFrameEditor->PasteNew(m_pUndoState->Track, m_iTargetFrame, m_pClipData);
	else
		pFrameEditor->PasteInsert(m_pUndoState->Track, m_iTargetFrame, m_pClipData);
	UPDATE_CONTROLS();
}



CFActionPasteOverwrite::~CFActionPasteOverwrite() {
	SAFE_RELEASE(m_pClipData);
}

bool CFActionPasteOverwrite::SaveState(const CMainFrame &MainFrm)		// // //
{
	m_TargetSelection.m_cpStart.m_iFrame = m_pUndoState->Cursor.m_iFrame;
	m_TargetSelection.m_cpEnd.m_iFrame = m_TargetSelection.m_cpStart.m_iFrame + m_pClipData->ClipInfo.Frames - 1;
	m_TargetSelection.m_cpStart.m_iChannel = m_pClipData->ClipInfo.FirstChannel;
	m_TargetSelection.m_cpEnd.m_iChannel = m_TargetSelection.m_cpStart.m_iChannel + m_pClipData->ClipInfo.Channels - 1;

	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	int Frames = pDoc->GetFrameCount(GET_SELECTED_TRACK());
	if (m_TargetSelection.m_cpEnd.m_iFrame >= Frames)
		m_TargetSelection.m_cpEnd.m_iFrame = Frames - 1;
	if (m_TargetSelection.m_cpEnd.m_iFrame < m_TargetSelection.m_cpStart.m_iFrame)
		return false;

	m_pOldClipData = GET_FRAME_EDITOR()->Copy(m_TargetSelection);
	return true;
}

void CFActionPasteOverwrite::Undo(CMainFrame &MainFrm)		// // //
{
	GET_FRAME_EDITOR()->PasteAt(m_pUndoState->Track, m_pOldClipData, m_pUndoState->Cursor);
}

void CFActionPasteOverwrite::Redo(CMainFrame &MainFrm)		// // //
{
	auto pEditor = GET_FRAME_EDITOR();
	pEditor->PasteAt(m_pUndoState->Track, m_pClipData, m_pUndoState->Cursor);
	pEditor->SetSelection(m_TargetSelection);
}



CFActionDropMove::~CFActionDropMove() {
	SAFE_RELEASE(m_pClipData);
}

bool CFActionDropMove::SaveState(const CMainFrame &MainFrm)
{
	return true;
}

void CFActionDropMove::Undo(CMainFrame &MainFrm)
{
	CFrameCursorPos Orig(m_pUndoState->Selection.m_cpStart);
	if (m_pRedoState->Selection.m_cpStart.m_iFrame < Orig.m_iFrame)
		Orig.m_iFrame += m_pRedoState->Selection.m_cpEnd.m_iFrame - m_pRedoState->Selection.m_cpStart.m_iFrame + 1;
	GET_FRAME_EDITOR()->MoveSelection(m_pUndoState->Track, m_pRedoState->Selection, Orig);
}

void CFActionDropMove::Redo(CMainFrame &MainFrm)
{
	GET_FRAME_EDITOR()->MoveSelection(m_pUndoState->Track, m_pUndoState->Selection,
											  {m_iTargetFrame, m_pUndoState->Cursor.m_iChannel});
}



CFActionClonePatterns::~CFActionClonePatterns() {
	SAFE_RELEASE(m_pClipData);
}

bool CFActionClonePatterns::SaveState(const CMainFrame &MainFrm)		// // //
{
	if (m_pUndoState->IsSelecting) {
		m_pClipData = GET_FRAME_EDITOR()->Copy();
		return true; // TODO: check this when all patterns are used up
	}
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	m_iOldPattern = pDoc->GetPatternAtFrame(STATE_EXPAND(m_pUndoState));
	if (pDoc->IsPatternEmpty(m_pUndoState->Track, m_pUndoState->Cursor.m_iChannel, m_iOldPattern))
		return false;
	m_iNewPattern = pDoc->GetFirstFreePattern(m_pUndoState->Track, m_pUndoState->Cursor.m_iChannel);
	return m_iNewPattern != -1;
}

void CFActionClonePatterns::Undo(CMainFrame &MainFrm)		// // //
{
	if (m_pUndoState->IsSelecting) {
		auto pEditor = GET_FRAME_EDITOR();
		pEditor->ClearPatterns(m_pUndoState->Track, m_pUndoState->Selection);
		ASSERT(m_pClipData != nullptr);
		pEditor->PasteAt(m_pUndoState->Track, m_pClipData, m_pUndoState->Selection.m_cpStart);
	}
	else {
		CFamiTrackerDoc *pDoc = GET_DOCUMENT();
		pDoc->ClearPattern(STATE_EXPAND(m_pUndoState));
		pDoc->SetPatternAtFrame(STATE_EXPAND(m_pUndoState), m_iOldPattern);
	}
}

void CFActionClonePatterns::Redo(CMainFrame &MainFrm)		// // //
{
	if (m_pUndoState->IsSelecting)
		GET_FRAME_EDITOR()->ClonePatterns(m_pUndoState->Track, m_pUndoState->Selection);
	else {
		CFamiTrackerDoc *pDoc = GET_DOCUMENT();
		pDoc->SetPatternAtFrame(STATE_EXPAND(m_pUndoState), m_iNewPattern);
		pDoc->CopyPattern(m_pUndoState->Track, m_iNewPattern, m_iOldPattern, m_pUndoState->Cursor.m_iChannel);
	}
}



CFActionDeleteSel::~CFActionDeleteSel() {
	SAFE_RELEASE(m_pClipData);
}

bool CFActionDeleteSel::SaveState(const CMainFrame &MainFrm)
{
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	CFrameSelection Sel(m_pUndoState->Selection);
	Sel.m_cpStart.m_iChannel = 0;
	Sel.m_cpEnd.m_iChannel = pDoc->GetChannelCount() - 1;
	int Frames = Sel.m_cpEnd.m_iFrame - Sel.m_cpStart.m_iFrame + 1;
	if (Frames == pDoc->GetFrameCount(m_pUndoState->Track))
		if (!Sel.m_cpEnd.m_iFrame--)
			return false;
	m_pClipData = GET_FRAME_EDITOR()->Copy(Sel);
	return true;
}

void CFActionDeleteSel::Undo(CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	pFrameEditor->PasteInsert(m_pUndoState->Track, m_pUndoState->Selection.m_cpStart.m_iFrame, m_pClipData);
	UPDATE_CONTROLS();
}

void CFActionDeleteSel::Redo(CMainFrame &MainFrm)
{
	auto pView = GET_VIEW();
	pView->GetDocument()->DeleteFrames(
		m_pUndoState->Track,
		m_pUndoState->Selection.m_cpStart.m_iFrame,
		m_pUndoState->Selection.m_cpEnd.m_iFrame - m_pUndoState->Selection.m_cpStart.m_iFrame + 1);		// // //
	pView->SelectFrame(m_pUndoState->Selection.m_cpStart.m_iFrame);
	GET_FRAME_EDITOR()->CancelSelection();
	UPDATE_CONTROLS();
}



CFActionMergeDuplicated::~CFActionMergeDuplicated() {
	SAFE_RELEASE(m_pOldClipData);
	SAFE_RELEASE(m_pClipData);
}

bool CFActionMergeDuplicated::SaveState(const CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	const CFamiTrackerDoc *pDoc = GET_DOCUMENT();
	m_pOldClipData = pFrameEditor->CopyEntire(m_pUndoState->Track);

	const int Channels = pDoc->GetChannelCount();
	const int Frames = pDoc->GetFrameCount(m_pUndoState->Track);
	m_pClipData = new CFrameClipData {Channels, Frames};

	unsigned int uiPatternUsed[MAX_PATTERN];
	for (int c = 0; c < Channels; ++c) {
		// mark all as unused
		for (unsigned int ui = 0; ui < MAX_PATTERN; ++ui)
			uiPatternUsed[ui] = MAX_PATTERN;

		// map used patterns to themselves
		for (int f = 0; f < Frames; ++f) {
			unsigned int uiPattern = pDoc->GetPatternAtFrame(m_pUndoState->Track, f, c);
			uiPatternUsed[uiPattern] = uiPattern;
		}

		// remap duplicates
		for (unsigned int ui = 0; ui < MAX_PATTERN; ++ui) {
			if (uiPatternUsed[ui] == MAX_PATTERN) continue;
			for (unsigned int uj = 0; uj < ui; ++uj)
				if (pDoc->ArePatternsSame(m_pUndoState->Track, c, ui, uj)) {		// // //
					uiPatternUsed[ui] = uj;
					TRACE("Duplicate: %d = %d\n", ui, uj);
					break;
				}
		}

		// apply mapping
		for (int f = 0; f < Frames; ++f)
			m_pClipData->SetFrame(f, c, uiPatternUsed[pDoc->GetPatternAtFrame(m_pUndoState->Track, f, c)]);
	}

	return true;
}

void CFActionMergeDuplicated::Undo(CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	pFrameEditor->PasteAt(m_pUndoState->Track, m_pOldClipData, {0, 0});
}

void CFActionMergeDuplicated::Redo(CMainFrame &MainFrm)
{
	CFrameEditor *pFrameEditor = GET_FRAME_EDITOR();
	pFrameEditor->PasteAt(m_pUndoState->Track, m_pClipData, {0, 0});
}
