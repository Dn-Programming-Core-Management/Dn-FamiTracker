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
#include "Settings.h"		// // //
#include "MainFrm.h"
#include "PatternEditor.h"
#include "PatternAction.h"

// // // Pattern editor state class

CPatternEditorState::CPatternEditorState(const CPatternEditor *pEditor, int Track) :
	Track(Track),
	Cursor(pEditor->GetCursor()),
	OriginalSelection(pEditor->GetSelection()),
	IsSelecting(pEditor->IsSelecting())
{
	Selection = OriginalSelection.GetNormalized();
}

void CPatternEditorState::ApplyState(CPatternEditor *pEditor) const
{
	pEditor->MoveCursor(Cursor);
	IsSelecting ? pEditor->SetSelection(OriginalSelection) : pEditor->CancelSelection();
	pEditor->InvalidateCursor();
}

// CPatternAction /////////////////////////////////////////////////////////////////
//
// Undo/redo commands for pattern editor
//

// // // for note writes
#define STATE_EXPAND(st) (st)->Track, (st)->Cursor.m_iFrame, (st)->Cursor.m_iChannel, (st)->Cursor.m_iRow

CPatternAction::CPatternAction(int iAction) : 
	CAction(iAction),
	m_pUndoState(nullptr),
	m_pRedoState(nullptr),
	m_pClipData(NULL), 
	m_pUndoClipData(NULL),
	m_pAuxiliaryClipData(NULL)		// // //
{
}

CPatternAction::~CPatternAction()
{
	SAFE_RELEASE(m_pUndoState);		// // //
	SAFE_RELEASE(m_pRedoState);		// // //

	SAFE_RELEASE(m_pClipData);
	SAFE_RELEASE(m_pUndoClipData);
	SAFE_RELEASE(m_pAuxiliaryClipData);
}

void CPatternAction::SetPaste(CPatternClipData *pClipData)
{
	m_pClipData = pClipData;
}

void CPatternAction::SetPasteMode(paste_mode_t Mode)		// // //
{
	m_iPasteMode = Mode;
}

void CPatternAction::SetPastePos(paste_pos_t Pos)		// // //
{
	m_iPastePos = Pos;
}

void CPatternAction::SetDragAndDrop(const CPatternClipData *pClipData, bool bDelete, bool bMix, const CSelection *pDragTarget)
{
	m_pClipData		= pClipData;
	m_bDragDelete	= bDelete;
	m_bDragMix		= bMix;
	m_dragTarget	= *pDragTarget;
	m_iPastePos		= PASTE_DRAG;
}

bool CPatternAction::SetTargetSelection(CPatternEditor *pPatternEditor, CSelection &Sel)		// // //
{
	CCursorPos Start;

	if ((m_iPastePos == PASTE_SELECTION || m_iPastePos == PASTE_FILL) && !m_bSelecting)
		m_iPastePos = PASTE_CURSOR;

	switch (m_iPastePos) { // m_iColumn will be written later
	case PASTE_CURSOR:
		Start = m_pUndoState->Cursor;
		break;
	case PASTE_DRAG:
		Start.m_iFrame = m_dragTarget.GetFrameStart();
		Start.m_iRow = m_dragTarget.GetRowStart();
		Start.m_iChannel = m_dragTarget.GetChanStart();
		break;
	case PASTE_SELECTION:
	case PASTE_FILL:
		Start.m_iFrame = m_selection.GetFrameStart();
		Start.m_iRow = m_selection.GetRowStart();
		Start.m_iChannel = m_selection.GetChanStart();
		break;
	}

	CPatternIterator End(CFamiTrackerDoc::GetDoc(), m_pUndoState->Track, Start);
	
	if (m_iPasteMode == PASTE_INSERT) {
		End.m_iFrame = Start.m_iFrame;
		End.m_iRow = pPatternEditor->GetCurrentPatternLength(End.m_iFrame) - 1;
	}
	else
		End += m_pClipData->ClipInfo.Rows - 1;

	switch (m_iPastePos) {
	case PASTE_FILL:
		End.m_iFrame = m_selection.GetFrameEnd();
		End.m_iRow = m_selection.GetRowEnd();
		End.m_iChannel = m_selection.GetChanEnd();
		Start.m_iColumn = GetCursorStartColumn(m_pClipData->ClipInfo.StartColumn);
		End.m_iColumn = GetCursorEndColumn(
			!((End.m_iChannel - Start.m_iChannel + 1) % m_pClipData->ClipInfo.Channels) ?
			m_pClipData->ClipInfo.EndColumn :
			static_cast<column_t>(COLUMN_EFF1 + CFamiTrackerDoc::GetDoc()->GetEffColumns(m_pUndoState->Track, End.m_iChannel)));
		break;
	case PASTE_DRAG:
		End.m_iChannel += m_pClipData->ClipInfo.Channels - 1;
		Start.m_iColumn = m_dragTarget.GetColStart();
		End.m_iColumn = m_dragTarget.GetColEnd();
		break;
	default:
		End.m_iChannel += m_pClipData->ClipInfo.Channels - 1;
		Start.m_iColumn = GetCursorStartColumn(m_pClipData->ClipInfo.StartColumn);
		End.m_iColumn = GetCursorEndColumn(m_pClipData->ClipInfo.EndColumn);
	}

	const bool bOverflow = theApp.GetSettings()->General.bOverflowPaste;
	if (!bOverflow && End.m_iFrame > Start.m_iFrame) {
		End.m_iFrame = Start.m_iFrame;
		End.m_iRow = pPatternEditor->GetCurrentPatternLength(End.m_iFrame) - 1;
	}

	const unsigned EFBEGIN = GetCursorStartColumn(COLUMN_EFF1);
	int OFFS = 3 * (GetSelectColumn(m_pUndoState->Cursor.m_iColumn) - m_pClipData->ClipInfo.StartColumn);
	if (static_cast<int>(EFBEGIN - Start.m_iColumn) > OFFS)
		OFFS = EFBEGIN - Start.m_iColumn;
	if (Start.m_iChannel == End.m_iChannel && Start.m_iColumn >= EFBEGIN && End.m_iColumn >= EFBEGIN) {
		if (m_iPastePos != PASTE_DRAG) {
			End.m_iColumn = static_cast<cursor_column_t>(End.m_iColumn + OFFS);
			Start.m_iColumn = static_cast<cursor_column_t>(Start.m_iColumn + OFFS);
			if (End.m_iColumn > C_EFF4_PARAM2)
				End.m_iColumn = C_EFF4_PARAM2;
		}
	}
	
	CSelection New;
	New.m_cpStart = Start;
	New.m_cpEnd = End;
	pPatternEditor->SetSelection(New);

	sel_condition_t Cond = pPatternEditor->GetSelectionCondition();
	if (Cond == SEL_CLEAN) {
		Sel = New;
		return true;
	}
	else {
		pPatternEditor->SetSelection(m_selection);
		if (!m_bSelecting) pPatternEditor->CancelSelection();
		int Confirm = IDYES;
		switch (Cond) {
		case SEL_REPEATED_ROW:
			Confirm = AfxMessageBox(IDS_PASTE_REPEATED_ROW, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
			break;
		case SEL_NONTERMINAL_SKIP: case SEL_TERMINAL_SKIP:
			if (!bOverflow) break;
			Confirm = AfxMessageBox(IDS_PASTE_NONTERMINAL, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
			break;
		}
		if (Confirm == IDYES) {
			pPatternEditor->SetSelection(Sel = New);
			return true;
		}
		else {
			return false;
		}
	}
}

void CPatternAction::DeleteSelection(CMainFrame *pMainFrm, const CSelection &Sel) const		// // //
{
	auto it = CPatternIterator::FromSelection(Sel,
		static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument(),
		pMainFrm->GetSelectedTrack());
	const column_t ColStart = GetSelectColumn(it.first.m_iColumn);
	const column_t ColEnd = GetSelectColumn(it.second.m_iColumn);

	stChanNote NoteData, Blank;

	do for (int i = it.first.m_iChannel; i <= it.second.m_iChannel; ++i) {
		it.first.Get(i, &NoteData);
		CopyNoteSection(&NoteData, &Blank, PASTE_DEFAULT,
						i == it.first.m_iChannel ? ColStart : COLUMN_NOTE,
						i == it.second.m_iChannel ? ColEnd : COLUMN_EFF4);
		it.first.Set(i, &NoteData);
	} while (++it.first <= it.second);
}

bool CPatternAction::ValidateSelection(const CMainFrame *pMainFrm) const		// // //
{
	if (!m_pUndoState->IsSelecting)
		return false;
	switch (static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor()
		->GetSelectionCondition(m_pUndoState->Selection)) {
	case SEL_CLEAN:
		return true;
	case SEL_REPEATED_ROW:
		static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_REPEATED_ROW); break;
	case SEL_NONTERMINAL_SKIP:
		static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_NONTERMINAL_SKIP); break;
	case SEL_TERMINAL_SKIP:
		static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_TERMINAL_SKIP); break;
	}
	MessageBeep(MB_ICONWARNING);
	return false;
}

void CPatternAction::UpdateView(CFamiTrackerDoc *pDoc) const		// // //
{
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
	pDoc->UpdateAllViews(NULL, UPDATE_FRAME); // cursor might have moved to different channel
}

std::pair<CPatternIterator, CPatternIterator> CPatternAction::GetIterators(const CMainFrame *pMainFrm) const
{
	auto pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	return m_pUndoState->IsSelecting ?
		CPatternIterator::FromSelection(m_pUndoState->Selection, pDoc, m_pUndoState->Track) :
		CPatternIterator::FromCursor(m_pUndoState->Cursor, pDoc, m_pUndoState->Track);
}

// Undo / Redo base methods

bool CPatternAction::SaveState(const CMainFrame *pMainFrm)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();
	CPatternEditor *pPatternEditor = pView->GetPatternEditor();

	// Save old state
	switch (m_iAction) {
		case ACT_DRAG_AND_DROP:
			if (m_bDragDelete)
				m_pAuxiliaryClipData = pPatternEditor->CopyRaw();
			if (!SetTargetSelection(pPatternEditor, m_newSelection))		// // //
				return false;
			m_pUndoClipData = pPatternEditor->CopyRaw();
			break;
		case ACT_EDIT_PASTE:
			if (!SetTargetSelection(pPatternEditor, m_newSelection))		// // //
				return false;
			m_pUndoClipData = pPatternEditor->CopyRaw();
			break;
#ifdef _DEBUG
		default:
			AfxMessageBox(_T("TODO Implement action for this command"));
#endif
	}

	return true;
}

void CPatternAction::SaveUndoState(const CMainFrame *pMainFrm)		// // //
{
	// Save undo cursor position
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	const CPatternEditor *pPatternEditor = pView->GetPatternEditor(); // TODO: remove

	SAFE_RELEASE(m_pUndoState);
	m_pUndoState = new CPatternEditorState {pPatternEditor, pMainFrm->GetSelectedTrack()};
	
	m_bSelecting = pPatternEditor->IsSelecting();
	m_selection = pPatternEditor->GetSelection();
}

void CPatternAction::SaveRedoState(const CMainFrame *pMainFrm)		// // //
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	SAFE_RELEASE(m_pRedoState);
	m_pRedoState = new CPatternEditorState {pView->GetPatternEditor(), pMainFrm->GetSelectedTrack()};
	UpdateView(pView->GetDocument());
}

void CPatternAction::RestoreUndoState(CMainFrame *pMainFrm) const		// // //
{
	if (!m_pUndoState) return;
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	m_pUndoState->ApplyState(pView->GetPatternEditor());
	UpdateView(pView->GetDocument());
}

void CPatternAction::RestoreRedoState(CMainFrame *pMainFrm) const		// // //
{
	if (!m_pRedoState) return;
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	m_pRedoState->ApplyState(pView->GetPatternEditor());
	UpdateView(pView->GetDocument());
}

void CPatternAction::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();
	CPatternEditor *pPatternEditor = pView->GetPatternEditor();

	switch (m_iAction) {
		case ACT_EDIT_PASTE:		// // //
			pPatternEditor->SetSelection(m_newSelection);		// // //
			pPatternEditor->PasteRaw(m_pUndoClipData);
			break;
		case ACT_DRAG_AND_DROP:
			pPatternEditor->SetSelection(m_newSelection);
			pPatternEditor->PasteRaw(m_pUndoClipData);
			if (m_bDragDelete)
				pPatternEditor->PasteRaw(m_pAuxiliaryClipData, m_selection.GetNormalized().m_cpStart);
			break;
#ifdef _DEBUG
		default:
			AfxMessageBox(_T("TODO Undo for this action is not implemented"));
#endif
	}
}

void CPatternAction::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();
	CPatternEditor *pPatternEditor = pView->GetPatternEditor();

	switch (m_iAction) {
		case ACT_EDIT_PASTE:
			pPatternEditor->Paste(m_pClipData, m_iPasteMode, m_iPastePos);		// // //
			break;		// // //
		case ACT_DRAG_AND_DROP:
			pPatternEditor->SetSelection(m_selection);
			if (m_bDragDelete)
				DeleteSelection(pMainFrm, m_selection);		// // //
			pPatternEditor->DragPaste(m_pClipData, &m_dragTarget, m_bDragMix);
			break;
#ifdef _DEBUG
		default:
			AfxMessageBox(_T("TODO: Redo for this action is not implemented"));
#endif
	}
}



CPSelectionAction::CPSelectionAction(int iAction) :
	CPatternAction(iAction), m_pUndoClipData(nullptr)
{
}

CPSelectionAction::~CPSelectionAction()
{
	SAFE_RELEASE(m_pUndoClipData);
}

bool CPSelectionAction::SaveState(const CMainFrame *pMainFrm)
{
	const CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	m_pUndoClipData = pPatternEditor->CopyRaw(m_pUndoState->Selection);
	return true;
}

void CPSelectionAction::Undo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	pPatternEditor->PasteRaw(m_pUndoClipData, m_pUndoState->Selection.m_cpStart);
}



// // // built-in pattern action subtypes



CPActionEditNote::CPActionEditNote(const stChanNote &Note) :
	CPatternAction(ACT_EDIT_NOTE), m_NewNote(Note)
{
}

bool CPActionEditNote::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->GetNoteData(STATE_EXPAND(m_pUndoState), &m_OldNote);
	return true;
}

void CPActionEditNote::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetNoteData(STATE_EXPAND(m_pUndoState), &m_OldNote);
}

void CPActionEditNote::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetNoteData(STATE_EXPAND(m_pUndoState), &m_NewNote);
}



CPActionReplaceNote::CPActionReplaceNote(const stChanNote &Note, int Frame, int Row, int Channel) :
	CPatternAction(ACT_REPLACE_NOTE), m_NewNote(Note),
	m_iFrame(Frame), m_iRow(Row), m_iChannel(Channel)
{
}

bool CPActionReplaceNote::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->GetNoteData(m_pUndoState->Track, m_iFrame, m_iChannel, m_iRow, &m_OldNote);
	return true;
}

void CPActionReplaceNote::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetNoteData(m_pUndoState->Track, m_iFrame, m_iChannel, m_iRow, &m_OldNote);
}

void CPActionReplaceNote::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetNoteData(m_pUndoState->Track, m_iFrame, m_iChannel, m_iRow, &m_NewNote);
}



CPActionInsertRow::CPActionInsertRow() :
	CPatternAction(ACT_INSERT_ROW)
{
}

bool CPActionInsertRow::SaveState(const CMainFrame *pMainFrm)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->GetNoteData(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, m_pUndoState->Cursor.m_iChannel,
					  pDoc->GetPatternLength(m_pUndoState->Track) - 1, &m_OldNote);
	return true;
}

void CPActionInsertRow::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->PullUp(STATE_EXPAND(m_pUndoState));
	pDoc->SetNoteData(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, m_pUndoState->Cursor.m_iChannel,
					  pDoc->GetPatternLength(m_pUndoState->Track) - 1, &m_OldNote);
}

void CPActionInsertRow::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->InsertRow(STATE_EXPAND(m_pUndoState));
}



CPActionDeleteRow::CPActionDeleteRow(bool PullUp, bool Backspace) :
	CPatternAction(ACT_DELETE_ROW), m_bPullUp(PullUp), m_bBack(Backspace)
{
}

bool CPActionDeleteRow::SaveState(const CMainFrame *pMainFrm)
{
	if (m_bBack && !m_pUndoState->Cursor.m_iRow) return false;
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->GetNoteData(STATE_EXPAND(m_pUndoState) - (m_bBack ? 1 : 0), &m_OldNote); // bad
	return true;
}

void CPActionDeleteRow::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	if (m_bPullUp)
		pDoc->InsertRow(STATE_EXPAND(m_pUndoState) - (m_bBack ? 1 : 0));
	pDoc->SetNoteData(STATE_EXPAND(m_pUndoState) - (m_bBack ? 1 : 0), &m_OldNote);
}

void CPActionDeleteRow::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->ClearRowField(STATE_EXPAND(m_pUndoState) - (m_bBack ? 1 : 0), m_pUndoState->Cursor.m_iColumn);
	if (m_bPullUp)
		pDoc->PullUp(STATE_EXPAND(m_pUndoState) - (m_bBack ? 1 : 0));
}



CPActionScrollField::CPActionScrollField(int Amount) :		// // //
	CPatternAction(ACT_INCREASE), m_iAmount(Amount)
{
}

bool CPActionScrollField::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->GetNoteData(STATE_EXPAND(m_pUndoState), &m_OldNote);
	
	switch (m_pUndoState->Cursor.m_iColumn) {
	case C_INSTRUMENT1: case C_INSTRUMENT2:
		return m_OldNote.Instrument < MAX_INSTRUMENTS && m_OldNote.Instrument != HOLD_INSTRUMENT;		// // // 050B
	case C_VOLUME:
		return m_OldNote.Vol < MAX_VOLUME;
	case C_EFF1_NUM: case C_EFF1_PARAM1: case C_EFF1_PARAM2:
		return m_OldNote.EffNumber[0] != EF_NONE;
	case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2:
		return m_OldNote.EffNumber[1] != EF_NONE;
	case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2:
		return m_OldNote.EffNumber[2] != EF_NONE;
	case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2:
		return m_OldNote.EffNumber[3] != EF_NONE;
	}

	return false;
}

void CPActionScrollField::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetNoteData(STATE_EXPAND(m_pUndoState), &m_OldNote);
}

void CPActionScrollField::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	stChanNote Note = m_OldNote;

	const auto ScrollFunc = [&] (unsigned char &Old, int Limit) {
		int New = static_cast<int>(Old) + m_iAmount;
		if (theApp.GetSettings()->General.bWrapPatternValue) {
			New %= Limit;
			if (New < 0) New += Limit;
		}
		else {
			if (New < 0) New = 0;
			if (New >= Limit) New = Limit - 1;
		}
		Old = static_cast<unsigned char>(New);
	};

	switch (m_pUndoState->Cursor.m_iColumn) {
	case C_INSTRUMENT1: case C_INSTRUMENT2:
		ScrollFunc(Note.Instrument, MAX_INSTRUMENTS); break;
	case C_VOLUME:
		ScrollFunc(Note.Vol, MAX_VOLUME); break;
	case C_EFF1_NUM: case C_EFF1_PARAM1: case C_EFF1_PARAM2:
		ScrollFunc(Note.EffParam[0], 0x100); break;
	case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2:
		ScrollFunc(Note.EffParam[1], 0x100); break;
	case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2:
		ScrollFunc(Note.EffParam[2], 0x100); break;
	case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2:
		ScrollFunc(Note.EffParam[3], 0x100); break;
	}

	pDoc->SetNoteData(STATE_EXPAND(m_pUndoState), &Note);
}



CPActionClearSel::CPActionClearSel() :
	CPSelectionAction(ACT_EDIT_DELETE)
{
}

void CPActionClearSel::Redo(CMainFrame *pMainFrm) const
{
	DeleteSelection(pMainFrm, m_pUndoState->Selection);
}



CPActionDeleteAtSel::CPActionDeleteAtSel() :
	CPatternAction(ACT_EDIT_DELETE_ROWS), m_pUndoHead(nullptr), m_pUndoTail(nullptr)
{
}

CPActionDeleteAtSel::~CPActionDeleteAtSel()
{
	SAFE_RELEASE(m_pUndoHead);
	SAFE_RELEASE(m_pUndoTail);
}

bool CPActionDeleteAtSel::SaveState(const CMainFrame *pMainFrm)
{
	if (!m_pUndoState->IsSelecting) return false;

	const CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	m_cpTailPos = CCursorPos {
		m_pUndoState->Selection.m_cpEnd.m_iRow + 1,
		m_pUndoState->Selection.m_cpStart.m_iChannel,
		m_pUndoState->Selection.m_cpStart.m_iColumn,
		m_pUndoState->Selection.m_cpEnd.m_iFrame
	};
	m_pUndoHead = pPatternEditor->CopyRaw(m_pUndoState->Selection);
	int Length = pPatternEditor->GetCurrentPatternLength(m_cpTailPos.m_iFrame) - 1;
	if (m_cpTailPos.m_iRow <= Length)
		m_pUndoTail = pPatternEditor->CopyRaw(CSelection {m_cpTailPos, CCursorPos {
			Length,
			m_pUndoState->Selection.m_cpEnd.m_iChannel,
			m_pUndoState->Selection.m_cpEnd.m_iColumn,
			m_cpTailPos.m_iFrame
		}});
	return true;
}

void CPActionDeleteAtSel::Undo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	pPatternEditor->PasteRaw(m_pUndoHead, m_pUndoState->Selection.m_cpStart);
	if (m_pUndoTail)
		pPatternEditor->PasteRaw(m_pUndoTail, m_cpTailPos);
}

void CPActionDeleteAtSel::Redo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();

	CSelection Sel(m_pUndoState->Selection);
	Sel.m_cpEnd.m_iRow = pPatternEditor->GetCurrentPatternLength(Sel.m_cpEnd.m_iFrame) - 1;
	DeleteSelection(pMainFrm, Sel);
	if (m_pUndoTail)
		pPatternEditor->PasteRaw(m_pUndoTail, m_pUndoState->Selection.m_cpStart);
	pPatternEditor->CancelSelection();
}



CPActionInsertAtSel::CPActionInsertAtSel() :
	CPatternAction(ACT_INSERT_SEL_ROWS), m_pUndoHead(nullptr), m_pUndoTail(nullptr)
{
}

CPActionInsertAtSel::~CPActionInsertAtSel()
{
	SAFE_RELEASE(m_pUndoHead);
	SAFE_RELEASE(m_pUndoTail);
}

bool CPActionInsertAtSel::SaveState(const CMainFrame *pMainFrm)
{
	if (!m_pUndoState->IsSelecting) return false;

	const CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	m_cpTailPos = CCursorPos {
		pPatternEditor->GetCurrentPatternLength(m_pUndoState->Selection.m_cpEnd.m_iFrame) - 1,
		m_pUndoState->Selection.m_cpStart.m_iChannel,
		m_pUndoState->Selection.m_cpStart.m_iColumn,
		m_pUndoState->Selection.m_cpEnd.m_iFrame
	};
	CCursorPos HeadEnd {
		m_cpTailPos.m_iRow,
		m_pUndoState->Selection.m_cpEnd.m_iChannel,
		m_pUndoState->Selection.m_cpEnd.m_iColumn,
		m_cpTailPos.m_iFrame
	};

	m_pUndoTail = pPatternEditor->CopyRaw(CSelection {m_cpTailPos, CCursorPos {HeadEnd}});
	if (--HeadEnd.m_iRow < 0) {
		--HeadEnd.m_iFrame;
		HeadEnd.m_iRow += pPatternEditor->GetCurrentPatternLength(HeadEnd.m_iFrame);
	}
	if (m_pUndoState->Selection.m_cpStart <= HeadEnd) {
		m_pUndoHead = pPatternEditor->CopyRaw(CSelection {m_pUndoState->Selection.m_cpStart, HeadEnd});
		m_cpHeadPos = m_pUndoState->Selection.m_cpStart;
		if (++m_cpHeadPos.m_iRow >= pPatternEditor->GetCurrentPatternLength(m_cpHeadPos.m_iFrame)) {
			++m_cpHeadPos.m_iFrame;
			m_cpHeadPos.m_iRow = 0;
		}
	}

	return true;
}

void CPActionInsertAtSel::Undo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	pPatternEditor->PasteRaw(m_pUndoTail, m_cpTailPos);
	if (m_pUndoHead)
		pPatternEditor->PasteRaw(m_pUndoHead, m_pUndoState->Selection.m_cpStart);
}

void CPActionInsertAtSel::Redo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	
	CSelection Sel(m_pUndoState->Selection);
	Sel.m_cpEnd.m_iRow = pPatternEditor->GetCurrentPatternLength(Sel.m_cpEnd.m_iFrame) - 1;
	DeleteSelection(pMainFrm, Sel);
	if (m_pUndoHead)
		pPatternEditor->PasteRaw(m_pUndoHead, m_cpHeadPos);
}



CPActionTranspose::CPActionTranspose(transpose_t Type) :
	CPSelectionAction(ACT_TRANSPOSE), m_iTransposeMode(Type)
{
}

void CPActionTranspose::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	auto it = GetIterators(pMainFrm);
	stChanNote Note;
	
	int ChanStart     = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).m_iChannel;
	int ChanEnd       = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).m_iChannel;
	column_t ColStart = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).m_iColumn);
	column_t ColEnd   = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).m_iColumn);
	
	const bool bSingular = it.first == it.second && !m_pUndoState->IsSelecting;
	const unsigned Length = pDoc->GetPatternLength(m_pUndoState->Track);

	int Row = 0;		// // //
	int oldRow = -1;
	do {
		if (it.first.m_iRow <= oldRow)
			Row += Length + it.first.m_iRow - oldRow - 1;
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			if (!m_pUndoState->Selection.IsColumnSelected(COLUMN_NOTE, i))
				continue;
			Note = *(m_pUndoClipData->GetPattern(i - ChanStart, Row));		// // //
			if (Note.Note == NONE || Note.Note == HALT || Note.Note == RELEASE)
				continue;
			if (Note.Note == ECHO) {
				if (bSingular) switch (m_iTransposeMode) {		// // //
				case TRANSPOSE_DEC_NOTES: case TRANSPOSE_DEC_OCTAVES:
					if (Note.Octave > 0)
						Note.Octave--;
					break;
				case TRANSPOSE_INC_NOTES: case TRANSPOSE_INC_OCTAVES:
					if (Note.Octave < ECHO_BUFFER_LENGTH)
						Note.Octave++;
					break;
				}
			}
			else {		// // //
				static const int AMOUNT[] = {-1, 1, -NOTE_RANGE, NOTE_RANGE};
				int NewNote = MIDI_NOTE(Note.Octave, Note.Note) + AMOUNT[m_iTransposeMode];
				if (NewNote < 0) NewNote = 0;
				if (NewNote >= NOTE_COUNT) NewNote = NOTE_COUNT - 1;
				Note.Note = GET_NOTE(NewNote);
				Note.Octave = GET_OCTAVE(NewNote);
			}
			it.first.Set(i, &Note);
		}
		++Row;
		oldRow = it.first.m_iRow;
	} while (++it.first <= it.second);
}



CPActionScrollValues::CPActionScrollValues(int Amount) :
	CPSelectionAction(ACT_SCROLL_VALUES), m_iAmount(Amount)
{
}

void CPActionScrollValues::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	auto it = GetIterators(pMainFrm);
	stChanNote Note;

	int ChanStart     = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).m_iChannel;
	int ChanEnd       = (m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).m_iChannel;
	column_t ColStart = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpStart : m_pUndoState->Cursor).m_iColumn);
	column_t ColEnd   = GetSelectColumn(
		(m_pUndoState->IsSelecting ? m_pUndoState->Selection.m_cpEnd : m_pUndoState->Cursor).m_iColumn);

	const bool bSingular = it.first == it.second && !m_pUndoState->IsSelecting;
	const unsigned Length = pDoc->GetPatternLength(m_pUndoState->Track);

	const auto WarpFunc = [this] (unsigned char &x, int Lim) {
		int Val = x + m_iAmount;
		if (theApp.GetSettings()->General.bWrapPatternValue) {
			Val %= Lim;
			if (Val < 0) Val += Lim;
		}
		else {
			if (Val >= Lim) Val = Lim - 1;
			if (Val < 0) Val = 0;
		}
		x = static_cast<unsigned char>(Val);
	};
	
	int Row = 0;
	int oldRow = -1;
	do {
		if (it.first.m_iRow <= oldRow)
			Row += Length + it.first.m_iRow - oldRow - 1;
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			Note = *(m_pUndoClipData->GetPattern(i - ChanStart, Row));		// // //
			for (unsigned k = COLUMN_INSTRUMENT; k < COLUMNS; ++k) {
				if (i == ChanStart && k < ColStart)
					continue;
				if (i == ChanEnd && k > ColEnd)
					continue;
				switch (k) {
				case COLUMN_INSTRUMENT:
					if (Note.Instrument == MAX_INSTRUMENTS || Note.Instrument == HOLD_INSTRUMENT) break;		// // // 050B
					WarpFunc(Note.Instrument, MAX_INSTRUMENTS);
					break;
				case COLUMN_VOLUME:
					if (Note.Vol == MAX_VOLUME) break;
					WarpFunc(Note.Vol, MAX_VOLUME);
					break;
				case COLUMN_EFF1: case COLUMN_EFF2: case COLUMN_EFF3: case COLUMN_EFF4:
					if (Note.EffNumber[k - COLUMN_EFF1] == EF_NONE) break;
					if (bSingular) switch (Note.EffNumber[k - COLUMN_EFF1]) {
					case EF_SWEEPUP: case EF_SWEEPDOWN: case EF_ARPEGGIO: case EF_VIBRATO: case EF_TREMOLO:
					case EF_SLIDE_UP: case EF_SLIDE_DOWN: case EF_VOLUME_SLIDE: case EF_DELAYED_VOLUME: case EF_TRANSPOSE:
						unsigned char Hi = Note.EffParam[k - COLUMN_EFF1] >> 4;
						unsigned char Lo = Note.EffParam[k - COLUMN_EFF1] & 0x0F;
						WarpFunc(pPatternEditor->GetColumn() % 3 == 2 ? Hi : Lo, 0x10);
						Note.EffParam[k - COLUMN_EFF1] = (Hi << 4) | Lo;
						continue;
					}
					WarpFunc(Note.EffParam[k - COLUMN_EFF1], 0x100);
					break;
				}
			}
			it.first.Set(i, &Note);
		}
		++Row;
		oldRow = it.first.m_iRow;
	} while (++it.first <= it.second);
}



CPActionInterpolate::CPActionInterpolate() :
	CPSelectionAction(ACT_INTERPOLATE)
{
}

bool CPActionInterpolate::SaveState(const CMainFrame *pMainFrm)
{
	if (!ValidateSelection(pMainFrm))
		return false;
	const CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	m_iSelectionSize = pPatternEditor->GetSelectionSize();
	return CPSelectionAction::SaveState(pMainFrm);
}

void CPActionInterpolate::Redo(CMainFrame *pMainFrm) const
{
	auto it = GetIterators(pMainFrm);
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	stChanNote StartData, EndData;
	const CSelection &Sel = m_pUndoState->Selection;

	for (int i = Sel.m_cpStart.m_iChannel; i <= Sel.m_cpEnd.m_iChannel; ++i) {
		const int Columns = pDoc->GetEffColumns(m_pUndoState->Track, i) + 4;		// // //
		for (int j = 0; j < Columns; ++j) {
			if (!Sel.IsColumnSelected(static_cast<column_t>(j), i)) continue;
			CPatternIterator r {it.first};		// // //
			r.Get(i, &StartData);
			it.second.Get(i, &EndData);
			double StartValHi, StartValLo;		// // //
			double EndValHi, EndValLo;
			double DeltaHi, DeltaLo;
			bool TwoParam = false;
			effect_t Effect;
			switch (j) {
			case COLUMN_NOTE:
				if (StartData.Note < NOTE_C || StartData.Note > NOTE_B
					|| EndData.Note < NOTE_C || EndData.Note > NOTE_B)
					continue;
				StartValLo = (float)MIDI_NOTE(StartData.Octave, StartData.Note);
				EndValLo = (float)MIDI_NOTE(EndData.Octave, EndData.Note);
				break;
			case COLUMN_INSTRUMENT:
				if (StartData.Instrument == MAX_INSTRUMENTS || EndData.Instrument == MAX_INSTRUMENTS)
					continue;
				if (StartData.Instrument == HOLD_INSTRUMENT || EndData.Instrument == HOLD_INSTRUMENT)		// // // 050B
					continue;
				StartValLo = (float)StartData.Instrument;
				EndValLo = (float)EndData.Instrument;
				break;
			case COLUMN_VOLUME:
				if (StartData.Vol == MAX_VOLUME || EndData.Vol == MAX_VOLUME)
					continue;
				StartValLo = (float)StartData.Vol;
				EndValLo = (float)EndData.Vol;
				break;
			case COLUMN_EFF1: case COLUMN_EFF2: case COLUMN_EFF3: case COLUMN_EFF4:
				if (StartData.EffNumber[j - 3] == EF_NONE || EndData.EffNumber[j - 3] == EF_NONE
					|| StartData.EffNumber[j - 3] != EndData.EffNumber[j - 3])
					continue;
				StartValLo = (float)StartData.EffParam[j - 3];
				EndValLo = (float)EndData.EffParam[j - 3];
				Effect = StartData.EffNumber[j - 3];
				switch (Effect) {
				case EF_SWEEPUP: case EF_SWEEPDOWN: case EF_SLIDE_UP: case EF_SLIDE_DOWN:
				case EF_ARPEGGIO: case EF_VIBRATO: case EF_TREMOLO:
				case EF_VOLUME_SLIDE: case EF_DELAYED_VOLUME: case EF_TRANSPOSE:
					TwoParam = true;
				}
				break;
			}

			if (TwoParam) {
				StartValHi = std::floor(StartValLo / 16.0);
				StartValLo = std::fmod(StartValLo, 16.0);
				EndValHi = std::floor(EndValLo / 16.0);
				EndValLo = std::fmod(EndValLo, 16.0);
			}
			else
				StartValHi = EndValHi = 0.0;
			DeltaHi = (EndValHi - StartValHi) / float(m_iSelectionSize - 1);
			DeltaLo = (EndValLo - StartValLo) / float(m_iSelectionSize - 1);

			while (++r < it.second) {
				StartValLo += DeltaLo;
				StartValHi += DeltaHi;
				r.Get(i, &EndData);
				switch (j) {
				case COLUMN_NOTE:
					EndData.Note = GET_NOTE((int)StartValLo); 
					EndData.Octave = GET_OCTAVE((int)StartValLo); 
					break;
				case COLUMN_INSTRUMENT:
					EndData.Instrument = (int)StartValLo; 
					break;
				case COLUMN_VOLUME:
					EndData.Vol = (int)StartValLo; 
					break;
				case COLUMN_EFF1: case COLUMN_EFF2: case COLUMN_EFF3: case COLUMN_EFF4:
					EndData.EffNumber[j - 3] = Effect;
					EndData.EffParam[j - 3] = (int)StartValLo + ((int)StartValHi << 4); 
					break;
				}
				r.Set(i, &EndData);
			}
		}
	}
}



CPActionReverse::CPActionReverse() :
	CPSelectionAction(ACT_REVERSE)
{
}

bool CPActionReverse::SaveState(const CMainFrame *pMainFrm)
{
	if (!ValidateSelection(pMainFrm))
		return false;
	return CPSelectionAction::SaveState(pMainFrm);
}

void CPActionReverse::Redo(CMainFrame *pMainFrm) const
{
	auto it = GetIterators(pMainFrm);
	const CSelection &Sel = m_pUndoState->Selection;

	const column_t ColStart = GetSelectColumn(Sel.m_cpStart.m_iColumn);
	const column_t ColEnd = GetSelectColumn(Sel.m_cpEnd.m_iColumn);
	stChanNote NoteBegin, NoteEnd, Temp;

	while (it.first < it.second) {
		for (int c = Sel.m_cpStart.m_iChannel; c <= Sel.m_cpEnd.m_iChannel; ++c) {
			it.first.Get(c, &NoteBegin);
			it.second.Get(c, &NoteEnd);
			if (c == Sel.m_cpStart.m_iChannel && ColStart > 0) {		// // //
				Temp = NoteEnd;
				CopyNoteSection(&NoteEnd, &NoteBegin, PASTE_DEFAULT, COLUMN_NOTE, static_cast<column_t>(ColStart - 1));
				CopyNoteSection(&NoteBegin, &Temp, PASTE_DEFAULT, COLUMN_NOTE, static_cast<column_t>(ColStart - 1));
			}
			if (c == Sel.m_cpEnd.m_iChannel && ColEnd < COLUMN_EFF4) {
				Temp = NoteEnd;
				CopyNoteSection(&NoteEnd, &NoteBegin, PASTE_DEFAULT, static_cast<column_t>(ColEnd + 1), COLUMN_EFF4);
				CopyNoteSection(&NoteBegin, &Temp, PASTE_DEFAULT, static_cast<column_t>(ColEnd + 1), COLUMN_EFF4);
			}
			it.first.Set(c, &NoteEnd);
			it.second.Set(c, &NoteBegin);
		}
		++it.first;
		--it.second;
	}
}



CPActionReplaceInst::CPActionReplaceInst(unsigned Index) :
	CPSelectionAction(ACT_REPLACE_INSTRUMENT), m_iInstrumentIndex(Index)
{
}

bool CPActionReplaceInst::SaveState(const CMainFrame *pMainFrm)
{
	if (!m_pUndoState->IsSelecting || m_iInstrumentIndex > static_cast<unsigned>(MAX_INSTRUMENTS))
		return false;
	return CPSelectionAction::SaveState(pMainFrm);
}

void CPActionReplaceInst::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	auto it = GetIterators(pMainFrm);
	const CSelection &Sel = m_pUndoState->Selection;

	const int cBegin = Sel.GetChanStart() + (Sel.IsColumnSelected(COLUMN_INSTRUMENT, Sel.GetChanStart()) ? 0 : 1);
	const int cEnd = Sel.GetChanEnd() - (Sel.IsColumnSelected(COLUMN_INSTRUMENT, Sel.GetChanEnd()) ? 0 : 1);

	stChanNote Note;
	do for (int i = cBegin; i <= cEnd; ++i) {
		it.first.Get(i, &Note);
		if (Note.Instrument != MAX_INSTRUMENTS && Note.Instrument != HOLD_INSTRUMENT)		// // // 050B
			Note.Instrument = m_iInstrumentIndex;
		it.first.Set(i, &Note);
	} while (++it.first <= it.second);
}


/*
// // // TODO: move stuff in CPatternAction::SetTargetSelection to the redo state

CPActionDragDrop::CPActionDragDrop(const CPatternClipData *pClipData, bool bDelete, bool bMix, const CSelection &pDragTarget) :
	CPatternAction(ACT_DRAG_AND_DROP),
	m_pClipData(pClipData), m_bDragDelete(bDelete), m_bDragMix(bMix), m_dragTarget(pDragTarget)
{
}

bool CPActionDragDrop::SaveState(const CMainFrame *pMainFrm)
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	if (m_bDragDelete)
		m_pAuxiliaryClipData = pPatternEditor->CopyRaw();
	if (!SetTargetSelection(pPatternEditor, m_newSelection))		// // //
		return false;
	m_pUndoClipData = pPatternEditor->CopyRaw();
	return true;
}

void CPActionDragDrop::Undo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	pPatternEditor->SetSelection(m_newSelection);
	pPatternEditor->PasteRaw(m_pUndoClipData);
	if (m_bDragDelete)
		pPatternEditor->PasteRaw(m_pAuxiliaryClipData, m_pUndoState->Selection.m_cpStart);
}

void CPActionDragDrop::Redo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	if (m_bDragDelete)
		DeleteSelection(pMainFrm, m_pUndoState->Selection);		// // //
	pPatternEditor->DragPaste(m_pClipData, &m_dragTarget, m_bDragMix);
}
*/


CPActionPatternLen::CPActionPatternLen(int Length) :
	CPatternAction(ACT_PATTERN_LENGTH), m_iNewPatternLen(Length)
{
}

bool CPActionPatternLen::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	m_iOldPatternLen = pDoc->GetPatternLength(m_pUndoState->Track);
	return m_iNewPatternLen != m_iOldPatternLen;
}

void CPActionPatternLen::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetPatternLength(m_pUndoState->Track, m_iOldPatternLen);
	pMainFrm->UpdateControls();
}

void CPActionPatternLen::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetPatternLength(m_pUndoState->Track, m_iNewPatternLen);
	pMainFrm->UpdateControls();
}

bool CPActionPatternLen::Merge(const CAction *Other)		// // //
{
	const CPActionPatternLen *pAction = dynamic_cast<const CPActionPatternLen*>(Other);
	if (!pAction) return false;
	if (m_pUndoState->Track != pAction->m_pUndoState->Track)
		return false;

	*m_pRedoState = *pAction->m_pRedoState;
	m_iNewPatternLen = pAction->m_iNewPatternLen;
	return true;
}



CPActionStretch::CPActionStretch(std::vector<int> Stretch) :
	CPSelectionAction(ACT_STRETCH_PATTERN), m_iStretchMap(Stretch)
{
}

bool CPActionStretch::SaveState(const CMainFrame *pMainFrm)
{
	if (m_iStretchMap.empty())
		return false;
	if (!ValidateSelection(pMainFrm))
		return false;
	return CPSelectionAction::SaveState(pMainFrm);
}

void CPActionStretch::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	auto it = GetIterators(pMainFrm);
	const CSelection &Sel = m_pUndoState->Selection;
	CPatternIterator s {it.first};

	const column_t ColStart = GetSelectColumn(Sel.m_cpStart.m_iColumn);
	const column_t ColEnd = GetSelectColumn(Sel.m_cpEnd.m_iColumn);
	stChanNote Target, Source;

	int Pos = 0;
	int Offset = 0;
	int oldRow = -1;
	do {
		for (int i = Sel.m_cpStart.m_iChannel; i <= Sel.m_cpEnd.m_iChannel; ++i) {
			if (Offset < m_pUndoClipData->ClipInfo.Rows && m_iStretchMap[Pos] > 0)
				Source = *(m_pUndoClipData->GetPattern(i - Sel.m_cpStart.m_iChannel, Offset));
			else 
				Source = stChanNote { };		// // //
			it.first.Get(i, &Target);
			CopyNoteSection(&Target, &Source, PASTE_DEFAULT,
							i == Sel.m_cpStart.m_iChannel ? ColStart : COLUMN_NOTE,
							i == Sel.m_cpEnd.m_iChannel ? ColEnd : COLUMN_EFF4);
			it.first.Set(i, &Target);
		}
		int dist = m_iStretchMap[Pos++];
		for (int i = 0; i < dist; ++i) {
			++Offset;
			oldRow = s.m_iRow;
			++s;
			if (s.m_iRow <= oldRow)
				Offset += pDoc->GetPatternLength(m_pUndoState->Track) + s.m_iRow - oldRow - 1;
		}
		Pos %= m_iStretchMap.size();
	} while (++it.first <= it.second);
}



CPActionEffColumn::CPActionEffColumn(int Channel, int Count) :		// // //
	CPatternAction(ACT_EFFECT_COLUMNS), m_iChannel(Channel), m_iNewColumns(Count)
{
}

bool CPActionEffColumn::SaveState(const CMainFrame *pMainFrm)
{
	if (m_iNewColumns >= static_cast<int>(MAX_EFFECT_COLUMNS)) return false;
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	m_iOldColumns = pDoc->GetEffColumns(m_pUndoState->Track, m_iChannel);
	return true;
}

void CPActionEffColumn::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetEffColumns(m_pUndoState->Track, m_iChannel, m_iOldColumns);
}

void CPActionEffColumn::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetEffColumns(m_pUndoState->Track, m_iChannel, m_iNewColumns);
}

void CPActionEffColumn::UpdateView(CFamiTrackerDoc *pDoc) const		// // //
{
	pDoc->UpdateAllViews(NULL, UPDATE_COLUMNS);
}



CPActionHighlight::CPActionHighlight(stHighlight Hl) :		// // //
	CPatternAction(1), m_NewHighlight(Hl)
{
}

bool CPActionHighlight::SaveState(const CMainFrame *pMainFrm)
{
	const CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	m_OldHighlight = pDoc->GetHighlight();
	return memcmp(&m_NewHighlight, &m_OldHighlight, sizeof(stHighlight)) != 0;
}

void CPActionHighlight::Undo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetHighlight(m_OldHighlight);
}

void CPActionHighlight::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	pDoc->SetHighlight(m_NewHighlight);
}

void CPActionHighlight::UpdateView(CFamiTrackerDoc *pDoc) const
{
	pDoc->UpdateAllViews(NULL, UPDATE_HIGHLIGHT);
}
