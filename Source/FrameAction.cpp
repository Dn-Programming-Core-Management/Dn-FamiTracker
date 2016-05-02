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

// CFrameAction ///////////////////////////////////////////////////////////////////
//
// Undo/redo commands for frame editor
//

CFrameAction::CFrameAction(int iAction) : 
	CAction(iAction),
	m_pAllPatterns(NULL),
	m_pClipData(NULL)
{
}

CFrameAction::~CFrameAction()
{
	SAFE_RELEASE_ARRAY(m_pAllPatterns);
	SAFE_RELEASE(m_pClipData);
}

void CFrameAction::SetFrameCount(unsigned int FrameCount)
{
	m_iNewFrameCount = FrameCount;
}

void CFrameAction::SetPattern(unsigned int Pattern)
{
	m_iNewPattern = Pattern;
}

void CFrameAction::SetPatternDelta(int Delta)		// // //
{
	m_iPatternDelta = Delta;
}

void CFrameAction::SetPasteData(CFrameClipData *pClipData)
{
	m_pClipData = pClipData;
}

void CFrameAction::SaveFrame(CFamiTrackerDoc *pDoc)
{
	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); ++i) {
		m_iPatterns[i] = pDoc->GetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, i);
	}
}

void CFrameAction::RestoreFrame(CFamiTrackerDoc *pDoc)
{
	for (unsigned int i = 0; i < pDoc->GetAvailableChannels(); ++i) {
		pDoc->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, i, m_iPatterns[i]);
	}
}

void CFrameAction::SetDragInfo(int DragTarget, CFrameClipData *pClipData, bool Remove)
{
	m_iDragTarget = DragTarget;
	m_pClipData = pClipData;
	m_bDragRemove = Remove;
}

void CFrameAction::SaveAllFrames(CFamiTrackerDoc *pDoc)
{
	int Frames = pDoc->GetFrameCount(m_iUndoTrack);
	int Channels = pDoc->GetChannelCount();

	m_pAllPatterns = new unsigned int[Frames * Channels];

	for (int i = 0; i < Frames; ++i) {
		for (int j = 0; j < Channels; ++j) {
			m_pAllPatterns[i * Channels + j] = pDoc->GetPatternAtFrame(m_iUndoTrack, i, j);
		}
	}

	m_iUndoFrameCount = Frames;
}

void CFrameAction::RestoreAllFrames(CFamiTrackerDoc *pDoc)
{
	pDoc->SetFrameCount(m_iUndoTrack, m_iUndoFrameCount);

	int Frames = pDoc->GetFrameCount(m_iUndoTrack);
	int Channels = pDoc->GetChannelCount();
	
	for (int i = 0; i < Frames; ++i) {
		for (int j = 0; j < Channels; ++j) {
			pDoc->SetPatternAtFrame(m_iUndoTrack, i, j, m_pAllPatterns[i * Channels + j]);
		}
	}
}

int CFrameAction::ClipPattern(int Pattern) const
{
	if (Pattern < 0)
		Pattern = 0;
	if (Pattern > MAX_PATTERN - 1)
		Pattern = MAX_PATTERN - 1;

	return Pattern;
}

void CFrameAction::ClearPatterns(CFamiTrackerDoc *pDoc, int Target)
{
	const int Rows = m_pClipData->ClipInfo.Rows;
	const int Channels = m_pClipData->ClipInfo.Channels;

	// Clean up the copy to new patterns command
	for (int i = 0; i < Rows; ++i) {
		for (int j = 0; j < Channels; ++j) {
			pDoc->ClearPattern(m_iUndoTrack, Target + i, j);
		}
	}

	pDoc->DeleteFrames(m_iUndoTrack, Target, Rows);
}

bool CFrameAction::SaveState(CMainFrame *pMainFrm)
{
	// Save action state for undo

	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDocument = pView->GetDocument();
	const int Channels = pDocument->GetAvailableChannels();

	m_iUndoTrack = pMainFrm->GetSelectedTrack();
	m_iUndoFramePos = pView->GetSelectedFrame();
	m_iUndoChannelPos = pView->GetSelectedChannel();

	pFrameEditor->GetSelectInfo(m_oSelInfo);

	switch (m_iAction) {
		case ACT_ADD:
			if (pDocument->GetFrameCount(m_iUndoTrack) == MAX_FRAMES)
				return false;
			break;
		case ACT_REMOVE:
			if (pDocument->GetFrameCount(m_iUndoTrack) == 1)
				return false;
			SaveFrame(pDocument);
			break;
		case ACT_DUPLICATE:
			if (pDocument->GetFrameCount(m_iUndoTrack) == MAX_FRAMES)
				return false;
			break;
		case ACT_DUPLICATE_PATTERNS:
			if (pDocument->GetFrameCount(m_iUndoTrack) == MAX_FRAMES)
				return false;
			break;
		case ACT_DUPLICATE_CURRENT:		// // //
			m_iOldPattern = pDocument->GetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos);
			if (pDocument->IsPatternEmpty(m_iUndoTrack, m_iUndoChannelPos, m_iOldPattern))
				return false;
			break;
		case ACT_CHANGE_COUNT:
			m_iUndoFrameCount = pDocument->GetFrameCount(m_iUndoTrack);
			break;
		case ACT_SET_PATTERN:
			m_iOldPattern = pDocument->GetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos);
			break;
		case ACT_SET_PATTERN_ALL:
			SaveFrame(pDocument);
			break;
		case ACT_CHANGE_PATTERN:
			m_iOldPattern = pDocument->GetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos);
			if (ClipPattern(m_iOldPattern + m_iPatternDelta) == m_iOldPattern)
				return false;
			break;
		case ACT_CHANGE_PATTERN_ALL:
			SaveFrame(pDocument);
			for (int i = 0; i < Channels; ++i) {
				if (m_iPatterns[i] + m_iPatternDelta < 0 || m_iPatterns[i] + m_iPatternDelta >= MAX_FRAMES)
					return false;
			}
			break;
		case ACT_MOVE_DOWN:
			if (pDocument->GetFrameCount(m_iUndoTrack) == m_iUndoFramePos + 1)
				return false;
			break;
		case ACT_MOVE_UP:
			if (m_iUndoFramePos == 0)
				return false;
			break;
		case ACT_DRAG_AND_DROP_MOVE:
		case ACT_DRAG_AND_DROP_COPY:
		case ACT_DRAG_AND_DROP_COPY_NEW:
		case ACT_DELETE_SELECTION:
		case ACT_MERGE_DUPLICATED_PATTERNS:
			SaveAllFrames(pDocument);
			break;
	}

	return true;
}

void CFrameAction::SaveRedoState(CMainFrame *pMainFrm)		// // //
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	m_iRedoFramePos = pView->GetSelectedFrame();
	m_iRedoChannelPos = pView->GetSelectedChannel();
}

void CFrameAction::Undo(CMainFrame *pMainFrm)
{
	// Undo action

	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDocument = pView->GetDocument();
	const int Channels = pDocument->GetAvailableChannels();
	int UpdateHint = UPDATE_FRAME;

	pView->SelectFrameChannel(m_iUndoFramePos, m_iUndoChannelPos);

	pFrameEditor->SetSelectInfo(m_oSelInfo);

	switch (m_iAction) {
		case ACT_ADD:
			pDocument->RemoveFrame(m_iUndoTrack, m_iUndoFramePos + 1);
			break;
		case ACT_REMOVE:
			pDocument->InsertFrame(m_iUndoTrack, m_iUndoFramePos);
			RestoreFrame(pDocument);
			break;
		case ACT_DUPLICATE:
			pDocument->RemoveFrame(m_iUndoTrack, m_iUndoFramePos);
			break;
		case ACT_DUPLICATE_PATTERNS:
			for (int i = 0; i < Channels; ++i) {
				pDocument->ClearPattern(m_iUndoTrack, m_iUndoFramePos + 1, i);
			}
			pDocument->RemoveFrame(m_iUndoTrack, m_iUndoFramePos + 1);
			break;
		case ACT_DUPLICATE_CURRENT:		// // //
			pDocument->ClearPattern(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos);
			pDocument->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos, m_iOldPattern);
			pMainFrm->UpdateControls();
			break;
		case ACT_CHANGE_COUNT:
			pDocument->SetFrameCount(m_iUndoTrack, m_iUndoFrameCount);
			pMainFrm->UpdateControls();
			break;
		case ACT_SET_PATTERN:
			pDocument->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos, m_iOldPattern);
			break;
		case ACT_SET_PATTERN_ALL:
			RestoreFrame(pDocument);
			break;
		case ACT_CHANGE_PATTERN:
			pDocument->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos, m_iOldPattern);
			break;
		case ACT_CHANGE_PATTERN_ALL:
			RestoreFrame(pDocument);
			break;
		case ACT_MOVE_DOWN:
			pDocument->MoveFrameUp(m_iUndoTrack, m_iUndoFramePos + 1);
			break;
		case ACT_MOVE_UP:
			pDocument->MoveFrameDown(m_iUndoTrack, m_iUndoFramePos - 1);
			break;
		case ACT_PASTE:
			pDocument->DeleteFrames(m_iUndoTrack, m_iUndoFramePos, m_pClipData->ClipInfo.Rows);
			break;
		case ACT_PASTE_NEW:
			ClearPatterns(pDocument, m_iUndoFramePos);
			break;
		case ACT_DRAG_AND_DROP_MOVE:
		case ACT_DRAG_AND_DROP_COPY:
		case ACT_DELETE_SELECTION:
			RestoreAllFrames(pDocument);
			pView->SelectFrame(m_oSelInfo.iRowEnd);
			pMainFrm->UpdateControls();
			break;
		case ACT_DRAG_AND_DROP_COPY_NEW:
			ClearPatterns(pDocument, m_iDragTarget);
			pView->SelectFrame(m_oSelInfo.iRowEnd);
			pMainFrm->UpdateControls();
			break;
		case ACT_MERGE_DUPLICATED_PATTERNS:
			RestoreAllFrames(pDocument);
			break;
	}

	pDocument->UpdateAllViews(NULL, UpdateHint);
}

void CFrameAction::Redo(CMainFrame *pMainFrm)
{
	// Redo action

	CFrameEditor *pFrameEditor = pMainFrm->GetFrameEditor();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDocument = pView->GetDocument();
	const int Channels = pDocument->GetAvailableChannels();
	int UpdateHint = UPDATE_FRAME;
	
	pView->SelectFrameChannel(m_iUndoFramePos, m_iUndoChannelPos);		// // //
	
	switch (m_iAction) {
		case ACT_ADD:
			pDocument->InsertFrame(m_iUndoTrack, m_iUndoFramePos + 1);
			break;
		case ACT_REMOVE:
			pDocument->RemoveFrame(m_iUndoTrack, m_iUndoFramePos);
			break;
		case ACT_DUPLICATE:
			pDocument->DuplicateFrame(m_iUndoTrack, m_iUndoFramePos);
			break;
		case ACT_DUPLICATE_PATTERNS:
			pDocument->DuplicatePatterns(m_iUndoTrack, m_iUndoFramePos + 1);
			break;
		case ACT_DUPLICATE_CURRENT:		// // //
			pDocument->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos,
				pDocument->GetFirstFreePattern(m_iUndoTrack, m_iUndoChannelPos));
			pDocument->CopyPattern(m_iUndoTrack, pDocument->GetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos),
				m_iOldPattern, m_iUndoChannelPos);
			break;
		case ACT_CHANGE_COUNT:
			pDocument->SetFrameCount(m_iUndoTrack, m_iNewFrameCount);
			pMainFrm->UpdateControls();
			break;
		case ACT_SET_PATTERN:
			pDocument->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos, m_iNewPattern);
			break;
		case ACT_SET_PATTERN_ALL:
			for (int i = 0; i < Channels; ++i) {
				pDocument->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, i, m_iNewPattern);
			}
			break;
		case ACT_CHANGE_PATTERN:
			pDocument->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos, ClipPattern(m_iOldPattern + m_iPatternDelta));
			break;
		case ACT_CHANGE_PATTERN_ALL:
			for (int i = 0; i < Channels; ++i) {
				pDocument->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, i, ClipPattern(m_iPatterns[i] + m_iPatternDelta));
			}
			break;
		case ACT_MOVE_DOWN:
			pDocument->MoveFrameDown(m_iUndoTrack, m_iUndoFramePos);
			pView->SelectFrame(m_iUndoFramePos + 1);		// // //
			pMainFrm->UpdateControls();
			break;
		case ACT_MOVE_UP:
			pDocument->MoveFrameUp(m_iUndoTrack, m_iUndoFramePos);
			pView->SelectFrame(m_iUndoFramePos - 1);		// // //
			pMainFrm->UpdateControls();
			break;
		case ACT_PASTE:
			pFrameEditor->Paste(m_iUndoTrack, m_pClipData);
			break;
		case ACT_PASTE_NEW:
			pFrameEditor->PasteNew(m_iUndoTrack, m_pClipData);
			break;
		case ACT_DRAG_AND_DROP_MOVE:
			pFrameEditor->PerformDragOperation(m_iUndoTrack, m_pClipData, m_iDragTarget, m_bDragRemove, false);
			pMainFrm->UpdateControls();
			break;
		case ACT_DRAG_AND_DROP_COPY:
			pFrameEditor->PerformDragOperation(m_iUndoTrack, m_pClipData, m_iDragTarget, false, false);
			pMainFrm->UpdateControls();
			break;
		case ACT_DRAG_AND_DROP_COPY_NEW:
			pFrameEditor->PerformDragOperation(m_iUndoTrack, m_pClipData, m_iDragTarget, false, true);
			pMainFrm->UpdateControls();
			break;
		case ACT_DELETE_SELECTION:
			pDocument->DeleteFrames(m_iUndoTrack, m_oSelInfo.iRowStart, m_oSelInfo.iRowEnd - m_oSelInfo.iRowStart + 1);
			pView->SelectFrame(m_oSelInfo.iRowStart);
			pFrameEditor->CancelSelection();
			pMainFrm->UpdateControls();
			break;
		case ACT_MERGE_DUPLICATED_PATTERNS:
			pDocument->MergeDuplicatedPatterns(m_iUndoTrack);		// // //
			break;
	}

	pDocument->UpdateAllViews(NULL, UpdateHint);
}

void CFrameAction::Update(CMainFrame *pMainFrm)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDocument = pView->GetDocument();

	switch (m_iAction) {
		case ACT_CHANGE_COUNT:
			pDocument->SetFrameCount(m_iUndoTrack, m_iNewFrameCount);
			pDocument->UpdateAllViews(NULL, UPDATE_FRAME);
			break;
		// TODO add change pattern 
		case ACT_CHANGE_PATTERN:
			{
				int OldPattern = pDocument->GetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos);
				int NewPattern = ClipPattern(OldPattern + m_iPatternDelta);
				if (NewPattern == OldPattern)
					return;
				pDocument->SetPatternAtFrame(m_iUndoTrack, m_iUndoFramePos, m_iUndoChannelPos, NewPattern);
				pDocument->SetModifiedFlag();
				pDocument->UpdateAllViews(NULL, UPDATE_PATTERN);
			}
			break;
	}
}
