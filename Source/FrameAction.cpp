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

CFrameAction::CFrameAction(int iAction) : 
	CAction(iAction),
	m_pUndoState(nullptr),		// // //
	m_pRedoState(nullptr),		// // //
	m_pAllPatterns(NULL),
	m_pClipData(NULL)
{
}

CFrameAction::~CFrameAction()
{
	SAFE_RELEASE(m_pUndoState);		// // //
	SAFE_RELEASE(m_pRedoState);		// // //

	SAFE_RELEASE_ARRAY(m_pAllPatterns);
	SAFE_RELEASE(m_pClipData);
}

void CFrameAction::SetDragInfo(int DragTarget, CFrameClipData *pClipData, bool Remove)
{
	m_iDragTarget = DragTarget;
	m_pClipData = pClipData;
	m_bDragRemove = Remove;
}

void CFrameAction::SaveAllFrames(CFamiTrackerDoc *pDoc)
{
	int Frames = pDoc->GetFrameCount(m_pUndoState->Track);
	int Channels = pDoc->GetChannelCount();

	m_pAllPatterns = new unsigned int[Frames * Channels];

	for (int i = 0; i < Frames; ++i) {
		for (int j = 0; j < Channels; ++j) {
			m_pAllPatterns[i * Channels + j] = pDoc->GetPatternAtFrame(m_pUndoState->Track, i, j);
		}
	}

	m_iUndoFrameCount = Frames;
}

void CFrameAction::RestoreAllFrames(CFamiTrackerDoc *pDoc) const
{
	pDoc->SetFrameCount(m_pUndoState->Track, m_iUndoFrameCount);

	int Frames = pDoc->GetFrameCount(m_pUndoState->Track);
	int Channels = pDoc->GetChannelCount();
	
	for (int i = 0; i < Frames; ++i) {
		for (int j = 0; j < Channels; ++j) {
			pDoc->SetPatternAtFrame(m_pUndoState->Track, i, j, m_pAllPatterns[i * Channels + j]);
		}
	}
}

int CFrameAction::ClipPattern(int Pattern)
{
	if (Pattern < 0)
		Pattern = 0;
	if (Pattern > MAX_PATTERN - 1)
		Pattern = MAX_PATTERN - 1;

	return Pattern;
}

void CFrameAction::ClearPatterns(CFamiTrackerDoc *pDoc, const CFrameClipData *pClipData, int Target) const		// // //
{
	const int Rows = pClipData->ClipInfo.Rows;
	const int Channels = pClipData->ClipInfo.Channels;

	// Clean up the copy to new patterns command
	CFrameIterator it {pDoc, m_pUndoState->Track, {m_pUndoState->Cursor.m_iFrame, pClipData->ClipInfo.FirstChannel}};
	for (int i = 0; i < Rows; ++i) {
		for (int j = 0; j < Channels; ++j)
			pDoc->ClearPattern(m_pUndoState->Track, it.m_iFrame, j + it.m_iChannel);
		++it;
	}

	pDoc->DeleteFrames(m_pUndoState->Track, Target, Rows);
}

bool CFrameAction::SaveState(const CMainFrame *pMainFrm)
{
	// Save action state for undo

	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();
	const int Channels = pDoc->GetAvailableChannels();

	switch (m_iAction) {
		case ACT_DRAG_AND_DROP_MOVE:
		case ACT_DRAG_AND_DROP_COPY:
		case ACT_DRAG_AND_DROP_COPY_NEW:
		case ACT_DELETE_SELECTION:
		case ACT_MERGE_DUPLICATED_PATTERNS:
			SaveAllFrames(pDoc);
			break;
	}

	return true;
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

void CFrameAction::Undo(CMainFrame *pMainFrm) const
{
	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();

	switch (m_iAction) {
		case ACT_DRAG_AND_DROP_MOVE:
		case ACT_DRAG_AND_DROP_COPY:
		case ACT_DELETE_SELECTION:
			RestoreAllFrames(pDoc);
			pView->SelectFrame(m_pUndoState->Selection.m_cpEnd.m_iFrame);
			pMainFrm->UpdateControls();
			break;
		case ACT_DRAG_AND_DROP_COPY_NEW:
			ClearPatterns(pDoc, m_pClipData, m_iDragTarget);
			pView->SelectFrame(m_pUndoState->Selection.m_cpEnd.m_iFrame);
			pMainFrm->UpdateControls();
			break;
		case ACT_MERGE_DUPLICATED_PATTERNS:
			RestoreAllFrames(pDoc);
			break;
	}
}

void CFrameAction::Redo(CMainFrame *pMainFrm) const
{
	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();
	
	switch (m_iAction) {
		case ACT_DRAG_AND_DROP_MOVE:
			pFrameEditor->PerformDragOperation(m_pUndoState->Track, m_pClipData, m_iDragTarget, m_bDragRemove, false);
			pMainFrm->UpdateControls();
			break;
		case ACT_DRAG_AND_DROP_COPY:
			pFrameEditor->PerformDragOperation(m_pUndoState->Track, m_pClipData, m_iDragTarget, false, false);
			pMainFrm->UpdateControls();
			break;
		case ACT_DRAG_AND_DROP_COPY_NEW:
			pFrameEditor->PerformDragOperation(m_pUndoState->Track, m_pClipData, m_iDragTarget, false, true);
			pMainFrm->UpdateControls();
			break;
		case ACT_DELETE_SELECTION:
			pDoc->DeleteFrames(
				m_pUndoState->Track,
				m_pUndoState->Selection.m_cpStart.m_iFrame,
				m_pUndoState->Selection.m_cpEnd.m_iFrame - m_pUndoState->Selection.m_cpStart.m_iFrame + 1);		// // //
			pView->SelectFrame(m_pUndoState->Selection.m_cpStart.m_iFrame);
			pFrameEditor->CancelSelection();
			pMainFrm->UpdateControls();
			break;
		case ACT_MERGE_DUPLICATED_PATTERNS:
			pDoc->MergeDuplicatedPatterns(m_pUndoState->Track);		// // //
			break;
	}
}



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
	pMainFrm->GetFrameEditor()->PasteAt(m_pUndoState->Track, m_pRowClipData, m_pUndoState->Cursor);
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
	pMainFrm->UpdateControls();
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
	pMainFrm->UpdateControls();
}



bool CFActionPaste::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	return m_pClipData->ClipInfo.Channels <= pDoc->GetChannelCount() &&
		m_pClipData->ClipInfo.Rows + pDoc->GetFrameCount(m_pUndoState->Track) <= MAX_FRAMES;

}

void CFActionPaste::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	if (m_bClone)
		ClearPatterns(pDoc, m_pClipData, m_pUndoState->Cursor.m_iFrame);
	else
		pDoc->DeleteFrames(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, m_pClipData->ClipInfo.Rows);
}

void CFActionPaste::Redo(CMainFrame *pMainFrm) const
{
	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	if (m_bClone)
		pFrameEditor->PasteNew(m_pUndoState->Track, m_pClipData);
	else
		pFrameEditor->Paste(m_pUndoState->Track, m_pClipData);
}


/*



bool CFActionAddFrame::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
}

void CFActionAddFrame::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
}

void CFActionAddFrame::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
}

bool CFActionAddFrame::Merge(const CAction *Other)		// // //
{
	auto pAction = dynamic_cast<const CFActionAddFrame*>(Other);
	if (!pAction) return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;

	return true;
}
*/
