/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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
** Any permitted reproduction of these routin, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "FrameAction.h"
#include "FrameEditor.h"

// // // Frame editor state class

CFrameEditorState::CFrameEditorState(const CFamiTrackerView *pView, int Track) :		// // //
	Track(Track),
	Cursor {pView->GetSelectedFrame(), pView->GetSelectedChannel()},
	OriginalSelection(static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor()->GetSelection()),
	IsSelecting(static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor()->IsSelecting())
{
	Selection = OriginalSelection.GetNormalized();
}

void CFrameEditorState::ApplyState(CFamiTrackerView *pView) const
{
	pView->SelectFrame(Cursor.m_iFrame);
	pView->SelectChannel(Cursor.m_iChannel);
	auto pEditor = static_cast<CMainFrame*>(pView->GetParentFrame())->GetFrameEditor();
	IsSelecting ? pEditor->SetSelection(OriginalSelection) : pEditor->CancelSelection();
}

#define STATE_EXPAND(st) (st)->Track, (st)->Cursor.m_iFrame, (st)->Cursor.m_iChannel

// CFrameAction ///////////////////////////////////////////////////////////////////
//
// Undo/redo commands for frame editor
//

CFrameAction::CFrameAction() : CAction(0)		// // // dummy
{
}

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

void CFrameAction::SaveUndoState(const CMainFrame *pMainFrm)		// // //
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	m_pUndoState = new CFrameEditorState {pView, pMainFrm->GetSelectedTrack()};		// // //
}

void CFrameAction::SaveRedoState(const CMainFrame *pMainFrm)		// // //
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	m_pRedoState = new CFrameEditorState {pView, pMainFrm->GetSelectedTrack()};		// // //
	pView->GetDocument()->UpdateAllViews(NULL, UPDATE_FRAME);
}

void CFrameAction::RestoreUndoState(CMainFrame *pMainFrm) const		// // //
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	m_pUndoState->ApplyState(pView);
	pView->GetDocument()->UpdateAllViews(NULL, UPDATE_FRAME);
}

void CFrameAction::RestoreRedoState(CMainFrame *pMainFrm) const		// // //
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	m_pRedoState->ApplyState(pView);
	pView->GetDocument()->UpdateAllViews(NULL, UPDATE_FRAME);
}



// // // built-in frame action subtypes



bool CFActionAddFrame::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	return pDoc->GetFrameCount(m_pUndoState->Track) < MAX_FRAMES;
}

void CFActionAddFrame::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->RemoveFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1);
}

void CFActionAddFrame::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->InsertFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1);
}



bool CFActionRemoveFrame::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	if (pDoc->GetFrameCount(m_pUndoState->Track) <= 1)
		return false;
	m_pRowClipData = pMainFrm->GetFrameEditor()->CopyFrame(m_pUndoState->Cursor.m_iFrame);
	return true;
}

void CFActionRemoveFrame::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->InsertFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
	pMainFrm->GetFrameEditor()->PasteInsert(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, m_pRowClipData);
}

void CFActionRemoveFrame::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->RemoveFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}



bool CFActionDuplicateFrame::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	return pDoc->GetFrameCount(m_pUndoState->Track) < MAX_FRAMES;
}

void CFActionDuplicateFrame::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->RemoveFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}

void CFActionDuplicateFrame::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->DuplicateFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}



bool CFActionCloneFrame::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	return pDoc->GetFrameCount(m_pUndoState->Track) < MAX_FRAMES;
}

void CFActionCloneFrame::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	int Count = pDoc->GetChannelCount();
	for (int i = 0; i < Count; ++i)
		pDoc->ClearPattern(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1, i);
	pDoc->RemoveFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1);
}

void CFActionCloneFrame::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->CloneFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame + 1);
}



bool CFActionFrameCount::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	m_iOldFrameCount = pDoc->GetFrameCount(m_pUndoState->Track);
	return m_iNewFrameCount != m_iOldFrameCount;
}

void CFActionFrameCount::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetFrameCount(m_pUndoState->Track, m_iOldFrameCount);
	pMainFrm->UpdateControls();
}

void CFActionFrameCount::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetFrameCount(m_pUndoState->Track, m_iNewFrameCount);
	pMainFrm->UpdateControls();
}

bool CFActionFrameCount::Merge(const CAction *Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionFrameCount*>(Other);
	if (!pAction) return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewFrameCount = pAction->m_iNewFrameCount;
	return true;
}



bool CFActionSetPattern::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	m_iOldPattern = pDoc->GetPatternAtFrame(STATE_EXPAND(m_pUndoState));
	return true;
}

void CFActionSetPattern::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetPatternAtFrame(STATE_EXPAND(m_pUndoState), m_iOldPattern);
}

void CFActionSetPattern::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetPatternAtFrame(STATE_EXPAND(m_pUndoState), m_iNewPattern);
}

bool CFActionSetPattern::Merge(const CAction *Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionSetPattern*>(Other);
	if (!pAction) return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewPattern = pAction->m_iNewPattern;
	return true;
}



bool CFActionSetPatternAll::SaveState(const CMainFrame *pMainFrm)
{
	m_pRowClipData = pMainFrm->GetFrameEditor()->CopyFrame(m_pUndoState->Cursor.m_iFrame);
	return true;
}

void CFActionSetPatternAll::Undo(CMainFrame *pMainFrm) const
{
	pMainFrm->GetFrameEditor()->PasteAt(m_pUndoState->Track, m_pRowClipData, m_pUndoState->Cursor);
}

void CFActionSetPatternAll::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	for (int i = 0; i < m_pRowClipData->ClipInfo.Channels; ++i)
		pDoc->SetPatternAtFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, i, m_iNewPattern);
}



bool CFActionChangePattern::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	m_iOldPattern = pDoc->GetPatternAtFrame(STATE_EXPAND(m_pUndoState));
	return ClipPattern(m_iOldPattern + m_iPatternOffset) != m_iOldPattern;
}

void CFActionChangePattern::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetPatternAtFrame(STATE_EXPAND(m_pUndoState), m_iOldPattern);
}

void CFActionChangePattern::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetPatternAtFrame(STATE_EXPAND(m_pUndoState), ClipPattern(m_iOldPattern + m_iPatternOffset));
}

bool CFActionChangePattern::Merge(const CAction *Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionChangePattern*>(Other);
	if (!pAction) return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iPatternOffset = ClipPattern(ClipPattern(m_iOldPattern + m_iPatternOffset) + pAction->m_iPatternOffset) - m_iOldPattern;
	return true;
}



bool CFActionChangePatternAll::SaveState(const CMainFrame *pMainFrm)
{
	m_pRowClipData = pMainFrm->GetFrameEditor()->CopyFrame(m_pUndoState->Cursor.m_iFrame);
	return true;
}

void CFActionChangePatternAll::Undo(CMainFrame *pMainFrm) const
{
	pMainFrm->GetFrameEditor()->PasteAt(m_pUndoState->Track, m_pRowClipData, m_pUndoState->Cursor);
}

void CFActionChangePatternAll::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	for (int i = 0; i < m_pRowClipData->ClipInfo.Channels; ++i)
		pDoc->SetPatternAtFrame(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, i,
								ClipPattern(m_pRowClipData->GetFrame(0, i) + m_iPatternOffset));
}



bool CFActionMoveDown::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	return m_pUndoState->Cursor.m_iFrame < static_cast<int>(pDoc->GetFrameCount(m_pUndoState->Track)) - 1;
}

void CFActionMoveDown::Undo(CMainFrame *pMainFrm) const
{
	Redo(pMainFrm); // inflection
}

void CFActionMoveDown::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->MoveFrameDown(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}



bool CFActionMoveUp::SaveState(const CMainFrame *pMainFrm)
{
	return m_pUndoState->Cursor.m_iFrame > 0;
}

void CFActionMoveUp::Undo(CMainFrame *pMainFrm) const
{
	Redo(pMainFrm); // inflection
}

void CFActionMoveUp::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->MoveFrameUp(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame);
}



bool CFActionPaste::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	return m_pClipData->ClipInfo.Channels <= pDoc->GetChannelCount() &&
		m_pClipData->ClipInfo.Frames + pDoc->GetFrameCount(m_pUndoState->Track) <= MAX_FRAMES;

}

void CFActionPaste::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	if (m_bClone)
		pMainFrm->GetFrameEditor()->ClearPatterns(m_pUndoState->Track, m_pRedoState->Selection);		// // //
	pDoc->DeleteFrames(m_pUndoState->Track, m_iTargetFrame, m_pClipData->ClipInfo.Frames);
	pMainFrm->UpdateControls();
}

void CFActionPaste::Redo(CMainFrame *pMainFrm) const
{
	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	if (m_bClone)
		pFrameEditor->PasteNew(m_pUndoState->Track, m_iTargetFrame, m_pClipData);
	else
		pFrameEditor->PasteInsert(m_pUndoState->Track, m_iTargetFrame, m_pClipData);
	pMainFrm->UpdateControls();
}



bool CFActionDropMove::SaveState(const CMainFrame *pMainFrm)
{
	return true;
}

void CFActionDropMove::Undo(CMainFrame *pMainFrm) const
{
	CFrameCursorPos Orig(m_pUndoState->Selection.m_cpStart);
	if (m_pRedoState->Selection.m_cpStart.m_iFrame < Orig.m_iFrame)
		Orig.m_iFrame += m_pRedoState->Selection.m_cpEnd.m_iFrame - m_pRedoState->Selection.m_cpStart.m_iFrame + 1;
	pMainFrm->GetFrameEditor()->MoveSelection(m_pUndoState->Track, m_pRedoState->Selection, Orig);
}

void CFActionDropMove::Redo(CMainFrame *pMainFrm) const
{
	pMainFrm->GetFrameEditor()->MoveSelection(m_pUndoState->Track, m_pUndoState->Selection,
											  {m_iTargetFrame, m_pUndoState->Cursor.m_iChannel});
}



bool CFActionClonePatterns::SaveState(const CMainFrame *pMainFrm)		// // //
{
	if (m_pUndoState->IsSelecting) {
		m_pClipData = pMainFrm->GetFrameEditor()->Copy();
		return true; // TODO: check this when all patterns are used up
	}
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	m_iOldPattern = pDoc->GetPatternAtFrame(STATE_EXPAND(m_pUndoState));
	if (pDoc->IsPatternEmpty(m_pUndoState->Track, m_pUndoState->Cursor.m_iChannel, m_iOldPattern))
		return false;
	m_iNewPattern = pDoc->GetFirstFreePattern(m_pUndoState->Track, m_pUndoState->Cursor.m_iChannel);
	return m_iNewPattern != -1;
}

void CFActionClonePatterns::Undo(CMainFrame *pMainFrm) const		// // //
{
	if (m_pUndoState->IsSelecting) {
		auto pEditor = pMainFrm->GetFrameEditor();
		pEditor->ClearPatterns(m_pUndoState->Track, m_pUndoState->Selection);
		ASSERT(m_pClipData != nullptr);
		pEditor->PasteAt(m_pUndoState->Track, m_pClipData, m_pUndoState->Selection.m_cpStart);
	}
	else {
		CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
		pDoc->ClearPattern(STATE_EXPAND(m_pUndoState));
		pDoc->SetPatternAtFrame(STATE_EXPAND(m_pUndoState), m_iOldPattern);
	}
}

void CFActionClonePatterns::Redo(CMainFrame *pMainFrm) const		// // //
{
	if (m_pUndoState->IsSelecting)
		pMainFrm->GetFrameEditor()->ClonePatterns(m_pUndoState->Track, m_pUndoState->Selection);
	else {
		CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
		pDoc->SetPatternAtFrame(STATE_EXPAND(m_pUndoState), m_iNewPattern);
		pDoc->CopyPattern(m_pUndoState->Track, m_iNewPattern, m_iOldPattern, m_pUndoState->Cursor.m_iChannel);
	}
}



bool CFActionDeleteSel::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	CFrameSelection Sel(m_pUndoState->Selection);
	Sel.m_cpStart.m_iChannel = 0;
	Sel.m_cpEnd.m_iChannel = pDoc->GetChannelCount() - 1;
	int Frames = Sel.m_cpEnd.m_iFrame - Sel.m_cpStart.m_iFrame + 1;
	if (Frames == pDoc->GetFrameCount(m_pUndoState->Track))
		if (!Sel.m_cpEnd.m_iFrame--)
			return false;
	m_pClipData = pMainFrm->GetFrameEditor()->Copy(Sel);
	return true;
}

void CFActionDeleteSel::Undo(CMainFrame *pMainFrm) const
{
	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	pFrameEditor->PasteInsert(m_pUndoState->Track, m_pUndoState->Selection.m_cpStart.m_iFrame, m_pClipData);
	pMainFrm->UpdateControls();
}

void CFActionDeleteSel::Redo(CMainFrame *pMainFrm) const
{
	auto pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	pView->GetDocument()->DeleteFrames(
		m_pUndoState->Track,
		m_pUndoState->Selection.m_cpStart.m_iFrame,
		m_pUndoState->Selection.m_cpEnd.m_iFrame - m_pUndoState->Selection.m_cpStart.m_iFrame + 1);		// // //
	pView->SelectFrame(m_pUndoState->Selection.m_cpStart.m_iFrame);
	pMainFrm->GetFrameEditor()->CancelSelection();
	pMainFrm->UpdateControls();
}



bool CFActionMergeDuplicated::SaveState(const CMainFrame *pMainFrm)
{
	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
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

void CFActionMergeDuplicated::Undo(CMainFrame *pMainFrm) const
{
	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	pFrameEditor->PasteAt(m_pUndoState->Track, m_pOldClipData, {0, 0});
}

void CFActionMergeDuplicated::Redo(CMainFrame *pMainFrm) const
{
	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	pFrameEditor->PasteAt(m_pUndoState->Track, m_pClipData, {0, 0});
}
