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

#include <vector>		// // //
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
	// Track {pEditor->GetTrack()},
	Track(Track),
	Cursor(pEditor->GetCursor()),
	Selection(pEditor->GetSelection()),
	IsSelecting(pEditor->IsSelecting())
{
}

void CPatternEditorState::ApplyState(CPatternEditor *pEditor) const
{
	pEditor->MoveCursor(Cursor);
	if (IsSelecting)
		pEditor->SetSelection(Selection);
	else
		pEditor->CancelSelection();
	pEditor->InvalidateCursor();
}

// CPatternAction /////////////////////////////////////////////////////////////////
//
// Undo/redo commands for pattern editor
//

// TODO: optimize the cases where it's not necessary to store the whole pattern
// TODO: split into several classes?

// // // for note writes
#define STATE_EXPAND(st) (st)->Track, (st)->Cursor.m_iFrame, (st)->Cursor.m_iChannel, (st)->Cursor.m_iRow

CPatternAction::CPatternAction(int iAction) : 
	CAction(iAction),
	m_pUndoState(nullptr),
	m_pRedoState(nullptr),
	m_pClipData(NULL), 
	m_pUndoClipData(NULL),
	m_pAuxiliaryClipData(NULL),		// // //
	m_iStretchMap()		// // //
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

void CPatternAction::SetTranspose(transpose_t Mode)
{
	m_iTransposeMode = Mode;
}

void CPatternAction::SetScroll(int Scroll)
{
	m_iScrollValue = Scroll;
}

void CPatternAction::SetDragAndDrop(const CPatternClipData *pClipData, bool bDelete, bool bMix, const CSelection *pDragTarget)
{
	m_pClipData		= pClipData;
	m_bDragDelete	= bDelete;
	m_bDragMix		= bMix;
	m_dragTarget	= *pDragTarget;
	m_iPastePos		= PASTE_DRAG;
}

void CPatternAction::SetPatternLength(int Length)
{
	m_iNewPatternLen = Length;
}

void CPatternAction::SetStretchMap(const std::vector<int> Map)		// // //
{
	m_iStretchMap = Map;
}

bool CPatternAction::SetTargetSelection(CPatternEditor *pPatternEditor)		// // //
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

	CPatternIterator End(pPatternEditor, m_pUndoState->Track, Start);
	
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
		Start.m_iColumn = CPatternEditor::GetCursorStartColumn(m_pClipData->ClipInfo.StartColumn);
		End.m_iColumn = CPatternEditor::GetCursorEndColumn(
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
		Start.m_iColumn = CPatternEditor::GetCursorStartColumn(m_pClipData->ClipInfo.StartColumn);
		End.m_iColumn = CPatternEditor::GetCursorEndColumn(m_pClipData->ClipInfo.EndColumn);
	}

	const bool bOverflow = theApp.GetSettings()->General.bOverflowPaste;
	if (!bOverflow && End.m_iFrame > Start.m_iFrame) {
		End.m_iFrame = Start.m_iFrame;
		End.m_iRow = pPatternEditor->GetCurrentPatternLength(End.m_iFrame) - 1;
	}

	const unsigned EFBEGIN = CPatternEditor::GetCursorStartColumn(COLUMN_EFF1);
	int OFFS = 3 * (CPatternEditor::GetSelectColumn(m_pUndoState->Cursor.m_iColumn) - m_pClipData->ClipInfo.StartColumn);
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
		m_newSelection = New;
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
			pPatternEditor->SetSelection(New);
			m_newSelection = New;
			return true;
		}
		else {
			return false;
		}
	}
}

void CPatternAction::CopySelection(const CPatternEditor *pPatternEditor)		// // //
{
	SAFE_RELEASE(m_pUndoClipData);
	m_pUndoClipData = pPatternEditor->CopyRaw();
}

void CPatternAction::PasteSelection(CPatternEditor *pPatternEditor) const		// // //
{
	pPatternEditor->PasteRaw(m_pUndoClipData);
}

void CPatternAction::CopyAuxiliary(const CPatternEditor *pPatternEditor)		// // //
{
	SAFE_RELEASE(m_pAuxiliaryClipData);
	m_pAuxiliaryClipData = pPatternEditor->CopyRaw();
}

void CPatternAction::PasteAuxiliary(CPatternEditor *pPatternEditor) const		// // //
{
	pPatternEditor->PasteRaw(m_pAuxiliaryClipData);
}

void CPatternAction::StretchPattern(CFamiTrackerDoc *pDoc) const		// // //
{
	CPatternIterator it = GetStartIterator();		// // //
	CPatternIterator s = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();
	const column_t ColStart = CPatternEditor::GetSelectColumn(m_selection.GetColStart());		// // //
	const column_t ColEnd = CPatternEditor::GetSelectColumn(m_selection.GetColEnd());
	stChanNote Target, Source;
	
	int Pos = 0;
	int Offset = 0;
	int oldRow = -1;
	do {
		for (int i = 0; i <= m_selection.GetChanEnd() - m_selection.GetChanStart(); ++i) {
			if (Offset < m_pUndoClipData->ClipInfo.Rows && m_iStretchMap[Pos] > 0)
				Source = *(m_pUndoClipData->GetPattern(i, Offset));
			else 
				Source = stChanNote { };		// // //
			it.Get(i + m_selection.GetChanStart(), &Target);
			CopyNoteSection(&Target, &Source, PASTE_DEFAULT, i == 0 ? ColStart : COLUMN_NOTE,
				i == m_selection.GetChanEnd() - m_selection.GetChanStart() ? ColEnd : COLUMN_EFF4);
			it.Set(i + m_selection.GetChanStart(), &Target);
		}
		int dist = m_iStretchMap[Pos++];
		for (int i = 0; i < dist; i++) {
			Offset++;
			oldRow = s.m_iRow;
			s++;
			if (s.m_iRow <= oldRow)
				Offset += pDoc->GetPatternLength(m_pUndoState->Track) + s.m_iRow - oldRow - 1;
		}
		Pos %= m_iStretchMap.size();
	} while (++it <= End);
}

void CPatternAction::Transpose(CFamiTrackerDoc *pDoc) const
{
	CPatternIterator it = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();
	int ChanStart = m_selection.GetChanStart();
	int ChanEnd	= m_selection.GetChanEnd();
	stChanNote Note;

	if (!m_bSelecting)
		ChanStart = ChanEnd = m_pUndoState->Cursor.m_iChannel;
	
	const bool bSingular = (it == End) && (ChanStart == ChanEnd);
	int Row = 0;		// // //
	int oldRow = -1;
	do {
		if (it.m_iRow <= oldRow) {
			Row += pDoc->GetPatternLength(m_pUndoState->Track) + it.m_iRow - oldRow - 1;
		}
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			if (!m_selection.IsColumnSelected(COLUMN_NOTE, i))
				continue;
			Note = *(m_pUndoClipData->GetPattern(i - ChanStart, Row));		// // //
			if (Note.Note == NONE || Note.Note == HALT || Note.Note == RELEASE)
				continue;
			if (Note.Note == ECHO) {
				if (bSingular) switch (m_iTransposeMode) {		// // //
				case TRANSPOSE_DEC_NOTES:
				case TRANSPOSE_DEC_OCTAVES:
					if (Note.Octave > 0)
						Note.Octave--;
					break;
				case TRANSPOSE_INC_NOTES:
				case TRANSPOSE_INC_OCTAVES:
					if (Note.Octave < ECHO_BUFFER_LENGTH)
						Note.Octave++;
					break;
				}
			}
			else {		// // //
				static const int AMOUNT[] = {-1, 1, -12, 12};
				int NewNote = MIDI_NOTE(Note.Octave, Note.Note) + AMOUNT[m_iTransposeMode];
				if (NewNote < 0) NewNote = 0;
				if (NewNote >= NOTE_COUNT) NewNote = NOTE_COUNT - 1;
				Note.Note = GET_NOTE(NewNote);
				Note.Octave = GET_OCTAVE(NewNote);
			}
			it.Set(i, &Note);
		}
		Row++;
		oldRow = it.m_iRow;
	} while (++it <= End);
}

void CPatternAction::Interpolate(CFamiTrackerDoc *pDoc) const
{
	const CPatternIterator End = GetEndIterator();		// // //
	stChanNote StartData, EndData;

	for (int i = m_selection.GetChanStart(); i <= m_selection.GetChanEnd(); ++i) {
		const int Columns = pDoc->GetEffColumns(m_pUndoState->Track, i) + 4;		// // //
		for (int j = 0; j < Columns; ++j) {
			CPatternIterator it = GetStartIterator();		// // //
			it.Get(i, &StartData);
			End.Get(i, &EndData);
			double StartValHi, StartValLo;		// // //
			double EndValHi, EndValLo;
			double DeltaHi, DeltaLo;
			bool TwoParam = false;
			effect_t Effect;
			switch (j) {
			case COLUMN_NOTE:
				if (!m_selection.IsColumnSelected(COLUMN_NOTE, i)
					|| StartData.Note < NOTE_C || StartData.Note > NOTE_B
					|| EndData.Note < NOTE_C || EndData.Note > NOTE_B)
					continue;
				StartValLo = (float)MIDI_NOTE(StartData.Octave, StartData.Note);
				EndValLo = (float)MIDI_NOTE(EndData.Octave, EndData.Note);
				break;
			case COLUMN_INSTRUMENT:
				if (!m_selection.IsColumnSelected(COLUMN_INSTRUMENT, i)
					|| StartData.Instrument == MAX_INSTRUMENTS || EndData.Instrument == MAX_INSTRUMENTS)
					continue;
				StartValLo = (float)StartData.Instrument;
				EndValLo = (float)EndData.Instrument;
				break;
			case COLUMN_VOLUME:
				if (!m_selection.IsColumnSelected(COLUMN_VOLUME, i)
					|| StartData.Vol == MAX_VOLUME || EndData.Vol == MAX_VOLUME)
					continue;
				StartValLo = (float)StartData.Vol;
				EndValLo = (float)EndData.Vol;
				break;
			case COLUMN_EFF1: case COLUMN_EFF2: case COLUMN_EFF3: case COLUMN_EFF4:
				if (!m_selection.IsColumnSelected(static_cast<column_t>(COLUMN_EFF1 + j - 3), i)
					|| StartData.EffNumber[j - 3] == EF_NONE || EndData.EffNumber[j - 3] == EF_NONE
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

			while (++it < End) {
				StartValLo += DeltaLo;
				StartValHi += DeltaHi;
				it.Get(i, &EndData);
				switch (j) {
					case 0: // // //
						EndData.Note = GET_NOTE((int)StartValLo); 
						EndData.Octave = GET_OCTAVE((int)StartValLo); 
						break;
					case 1: // // //
						EndData.Instrument = (int)StartValLo; 
						break;
					case 2: // // //
						EndData.Vol = (int)StartValLo; 
						break;
					case 3: // // //
					case 4: 
					case 5: 
					case 6:
						EndData.EffNumber[j - 3] = Effect;
						EndData.EffParam[j - 3] = (int)StartValLo + ((int)StartValHi << 4); 
						break;
				}
				it.Set(i, &EndData);
			}
		}
	}
}

void CPatternAction::Reverse(CFamiTrackerDoc *pDoc) const
{
	CPatternIterator itb = GetStartIterator();		// // //
	CPatternIterator ite = GetEndIterator();
	const column_t ColStart = CPatternEditor::GetSelectColumn(m_selection.GetColStart());
	const column_t ColEnd = CPatternEditor::GetSelectColumn(m_selection.GetColEnd());
	stChanNote NoteBegin, NoteEnd, Temp;
	
	while (itb < ite) {
		for (int c = m_selection.GetChanStart(); c <= m_selection.GetChanEnd(); ++c) {
			itb.Get(c, &NoteBegin);
			ite.Get(c, &NoteEnd);
			if (c == m_selection.GetChanStart() && ColStart > 0) {		// // //
				Temp = NoteEnd;
				CopyNoteSection(&NoteEnd, &NoteBegin, PASTE_DEFAULT, COLUMN_NOTE, static_cast<column_t>(ColStart - 1));
				CopyNoteSection(&NoteBegin, &Temp, PASTE_DEFAULT, COLUMN_NOTE, static_cast<column_t>(ColStart - 1));
			}
			if (c == m_selection.GetChanEnd() && ColEnd < COLUMN_EFF4) {
				Temp = NoteEnd;
				CopyNoteSection(&NoteEnd, &NoteBegin, PASTE_DEFAULT, static_cast<column_t>(ColEnd + 1), COLUMN_EFF4);
				CopyNoteSection(&NoteBegin, &Temp, PASTE_DEFAULT, static_cast<column_t>(ColEnd + 1), COLUMN_EFF4);
			}
			itb.Set(c, &NoteEnd);
			ite.Set(c, &NoteBegin);
		}
		itb++;
		ite--;
	}
}

void CPatternAction::ScrollValues(CFamiTrackerDoc *pDoc) const
{
	CPatternIterator it = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();
	stChanNote Note;

	int ChanStart     = m_bSelecting ? m_selection.GetChanStart() : m_pUndoState->Cursor.m_iChannel;
	int ChanEnd       = m_bSelecting ? m_selection.GetChanEnd() : m_pUndoState->Cursor.m_iChannel;
	column_t ColStart = CPatternEditor::GetSelectColumn(m_bSelecting ? m_selection.GetColStart() : m_pUndoState->Cursor.m_iColumn);
	column_t ColEnd   = CPatternEditor::GetSelectColumn(m_bSelecting ? m_selection.GetColEnd() : m_pUndoState->Cursor.m_iColumn);

	const bool bWarp = theApp.GetSettings()->General.bWrapPatternValue;
	const bool bSingular = (it == End) && (ChanStart == ChanEnd) && (ColStart == ColEnd) && (m_iScrollValue == -1 || m_iScrollValue == 1);
	const unsigned Length = pDoc->GetPatternLength(m_pUndoState->Track);
	
	int Row = 0;
	int oldRow = -1;
	do {
		if (it.m_iRow <= oldRow)
			Row += Length + it.m_iRow - oldRow - 1;
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			Note = *(m_pUndoClipData->GetPattern(i - ChanStart, Row));		// // //
			for (unsigned k = COLUMN_INSTRUMENT; k < COLUMNS; ++k) {
				if (i == ChanStart && k < ColStart)
					continue;
				if (i == ChanEnd && k > ColEnd)
					continue;
				switch (k) {
				case COLUMN_INSTRUMENT:
					if (Note.Instrument != MAX_INSTRUMENTS) {
						if (bWarp)		// // //
							Note.Instrument = (Note.Instrument + m_iScrollValue + MAX_INSTRUMENTS) % MAX_INSTRUMENTS;
						else {
							int Val = Note.Instrument + m_iScrollValue;
							if (Val < 0) Val = 0;
							if (Val >= MAX_INSTRUMENTS) Val = MAX_INSTRUMENTS - 1;
							Note.Instrument = Val;
						}
					}
					break;
				case COLUMN_VOLUME:
					if (Note.Vol != MAX_VOLUME) {
						if (bWarp)		// // //
							Note.Vol = (Note.Vol + m_iScrollValue + MAX_VOLUME) % MAX_VOLUME;
						else {
							int Val = Note.Vol + m_iScrollValue;
							if (Val < 0) Val = 0;
							if (Val >= MAX_VOLUME) Val = MAX_VOLUME - 1;
							Note.Vol = Val;
						}
					}
					break;
				case COLUMN_EFF1:
				case COLUMN_EFF2:
				case COLUMN_EFF3:
				case COLUMN_EFF4:
					if (Note.EffNumber[k - COLUMN_EFF1] != EF_NONE) {
						int Type = m_iScrollValue;		// // //
						if (bSingular) {
							int Hi = Note.EffParam[k - COLUMN_EFF1] >> 4;
							int Lo = Note.EffParam[k - COLUMN_EFF1] & 0x0F;
							switch (Note.EffNumber[k - COLUMN_EFF1]) {
							case EF_SWEEPUP: case EF_SWEEPDOWN:
							case EF_ARPEGGIO: case EF_VIBRATO: case EF_TREMOLO:
							case EF_SLIDE_UP: case EF_SLIDE_DOWN: case EF_VOLUME_SLIDE:
							case EF_DELAYED_VOLUME: case EF_TRANSPOSE:
								int CurCol = static_cast<int>(CFamiTrackerView::GetView()->GetPatternEditor()->GetColumn());
								if (CurCol % 3 != 2) { // y
									if (bWarp)
										Lo = (Lo + Type) & 0x0F;
									else {
										Lo += Type;
										if (Lo > 0x0F) Lo = 0x0F;
										if (Lo < 0)    Lo = 0;
									}
								}
								else { // x
									if (bWarp)
										Hi = (Hi + Type) & 0x0F;
									else {
										Hi += Type;
										if (Hi > 0x0F) Hi = 0x0F;
										if (Hi < 0)    Hi = 0;
									}
								}
								Type = 0;
								Note.EffParam[k - COLUMN_EFF1] = (Hi << 4) | Lo;
							}
						}
						if (!bWarp) {
							if (Note.EffParam[k - COLUMN_EFF1] + Type > 0xFF)
								Type = 0xFF - Note.EffParam[k - COLUMN_EFF1];
							if (Note.EffParam[k - COLUMN_EFF1] + Type < 0)
								Type = -Note.EffParam[k - COLUMN_EFF1];
						}
						Note.EffParam[k - COLUMN_EFF1] += Type;
					}
					break;
				}
			}
			it.Set(i, &Note);
		}
		Row++;
		oldRow = it.m_iRow;
	} while (++it <= End);
}

void CPatternAction::DeleteSelection(CFamiTrackerDoc *pDoc) const
{
	// Delete selection
	CPatternIterator it = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();
	const column_t ColStart = CPatternEditor::GetSelectColumn(m_selection.GetColStart());		// // //
	const column_t ColEnd = CPatternEditor::GetSelectColumn(m_selection.GetColEnd());

	stChanNote NoteData, Blank { };		// // //

	do {
		for (int i = m_selection.GetChanStart(); i <= m_selection.GetChanEnd(); ++i) {
			it.Get(i, &NoteData);
			CopyNoteSection(&NoteData, &Blank, PASTE_DEFAULT, i == m_selection.GetChanStart() ? ColStart : COLUMN_NOTE,
				i == m_selection.GetChanEnd() ? ColEnd : COLUMN_EFF4);		// // //
			it.Set(i, &NoteData);
		}
	} while (++it <= End);
}

void CPatternAction::UpdateView(CFamiTrackerDoc *pDoc) const		// // //
{
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
	pDoc->UpdateAllViews(NULL, UPDATE_FRAME); // cursor might have moved to different channel
}

CPatternIterator CPatternAction::GetStartIterator() const		// // //
{
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(AfxGetMainWnd());
	CCursorPos Pos = m_bSelecting ?
		(m_selection.m_cpStart < m_selection.m_cpEnd ? m_selection.m_cpStart : m_selection.m_cpEnd) :
		CCursorPos(m_pUndoState->Cursor);
	return CPatternIterator(static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor(), m_pUndoState->Track, Pos);
}

CPatternIterator CPatternAction::GetEndIterator() const
{
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(AfxGetMainWnd());
	CCursorPos Pos = m_bSelecting ?
		(m_selection.m_cpEnd < m_selection.m_cpStart ? m_selection.m_cpStart : m_selection.m_cpEnd) :
		CCursorPos(m_pUndoState->Cursor);
	return CPatternIterator(static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor(), m_pUndoState->Track, Pos);
}

std::pair<CPatternIterator, CPatternIterator> CPatternAction::GetIterators(const CMainFrame *pMainFrm) const
{
	CCursorPos c_it {m_pUndoState->Cursor}, c_end {m_pUndoState->Cursor};
	if (m_pUndoState->IsSelecting)
		m_pUndoState->Selection.Normalize(c_it, c_end);
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	return std::make_pair(
		CPatternIterator {pPatternEditor, static_cast<unsigned>(m_pUndoState->Track), c_it},
		CPatternIterator {pPatternEditor, static_cast<unsigned>(m_pUndoState->Track), c_end}
	);
}

// Undo / Redo base methods

bool CPatternAction::SaveState(const CMainFrame *pMainFrm)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();
	CPatternEditor *pPatternEditor = pView->GetPatternEditor();

	const int Track = m_pUndoState->Track;		// // //
	const int Frame = m_pUndoState->Cursor.m_iFrame;
	const int Row = m_pUndoState->Cursor.m_iRow;
	const int Channel = m_pUndoState->Cursor.m_iChannel;
	const cursor_column_t Column = m_pUndoState->Cursor.m_iColumn;

	// Save old state
	switch (m_iAction) {
		case ACT_DRAG_AND_DROP:
			if (m_bDragDelete)
				CopyAuxiliary(pPatternEditor);
			// continue
		case ACT_EDIT_PASTE:
			if (!SetTargetSelection(pPatternEditor))		// // //
				return false;
			CopySelection(pPatternEditor);
			break;
		case ACT_TRANSPOSE:		// // //
		case ACT_SCROLL_VALUES:
			pPatternEditor->SetSelection(m_selection);
			CopySelection(pPatternEditor);
			break;
		case ACT_INTERPOLATE: // 0CC: Copy only the selection if it is guaranteed to remain unmodified
		case ACT_REVERSE:
		case ACT_STRETCH_PATTERN: {		// // //
			sel_condition_t Cond = pPatternEditor->GetSelectionCondition();
			if (!pPatternEditor->IsSelecting())
				return false;
			switch (Cond) {
			case SEL_REPEATED_ROW:
				static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_REPEATED_ROW); break;
			case SEL_NONTERMINAL_SKIP:
				static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_NONTERMINAL_SKIP); break;
			case SEL_TERMINAL_SKIP:
				static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_SEL_TERMINAL_SKIP); break;
			}
			if (Cond != SEL_CLEAN) {
				MessageBeep(MB_ICONWARNING);
				return false;
			}
			CopySelection(pPatternEditor);
		}	break;
		case ACT_PATTERN_LENGTH:
			// Change pattern length
			m_iOldPatternLen = pDoc->GetPatternLength(Track);
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
	SAFE_RELEASE(m_pUndoState);
	m_pUndoState = new CPatternEditorState {pView->GetPatternEditor(), pMainFrm->GetSelectedTrack()};
	
	const CPatternEditor *pPatternEditor = pView->GetPatternEditor(); // TODO: remove
	m_bSelecting = pPatternEditor->IsSelecting();
	m_selection = pPatternEditor->GetSelection();
	m_iSelectionSize = pPatternEditor->GetSelectionSize();
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

	const int Track = m_pUndoState->Track;		// // //
	const int Frame = m_pUndoState->Cursor.m_iFrame;
	const int Row = m_pUndoState->Cursor.m_iRow;
	const int Channel = m_pUndoState->Cursor.m_iChannel;
	const cursor_column_t Column = m_pUndoState->Cursor.m_iColumn;

	switch (m_iAction) {
		case ACT_EDIT_PASTE:		// // //
			pPatternEditor->SetSelection(m_newSelection);		// // //
			PasteSelection(pPatternEditor);
			break;
		case ACT_TRANSPOSE:
		case ACT_SCROLL_VALUES:
		case ACT_INTERPOLATE:
		case ACT_REVERSE:
		case ACT_STRETCH_PATTERN:		// // //
			pPatternEditor->SetSelection(m_selection);		// // //
			PasteSelection(pPatternEditor);
			break;
		case ACT_DRAG_AND_DROP:
			pPatternEditor->SetSelection(m_newSelection);
			PasteSelection(pPatternEditor);		// // //
			RestoreSelection(pPatternEditor);
			if (m_bDragDelete)
				PasteAuxiliary(pPatternEditor);
			break;
		case ACT_PATTERN_LENGTH:
			pDoc->SetPatternLength(Track, m_iOldPatternLen);
			pMainFrm->UpdateControls();
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

	const int Track = m_pUndoState->Track;		// // //
	const int Frame = m_pUndoState->Cursor.m_iFrame;
	const int Row = m_pUndoState->Cursor.m_iRow;
	const int Channel = m_pUndoState->Cursor.m_iChannel;
	const cursor_column_t Column = m_pUndoState->Cursor.m_iColumn;

	switch (m_iAction) {
		case ACT_EDIT_PASTE:
			pPatternEditor->Paste(m_pClipData, m_iPasteMode, m_iPastePos);		// // //
			break;		// // //
		case ACT_TRANSPOSE:
			// // //
			Transpose(pDoc);
			break;
		case ACT_SCROLL_VALUES:
			// // //
			ScrollValues(pDoc);
			break;
		case ACT_INTERPOLATE:
			pPatternEditor->SetSelection(m_selection);
			Interpolate(pDoc);
			break;
		case ACT_REVERSE:
			pPatternEditor->SetSelection(m_selection);
			Reverse(pDoc);
			break;
		case ACT_DRAG_AND_DROP:
			RestoreSelection(pPatternEditor);
			if (m_bDragDelete)
				DeleteSelection(pDoc);
			pPatternEditor->DragPaste(m_pClipData, &m_dragTarget, m_bDragMix);
			break;
		case ACT_PATTERN_LENGTH:
			pDoc->SetPatternLength(Track, m_iNewPatternLen);
			pMainFrm->UpdateControls();
			break;
		case ACT_STRETCH_PATTERN:		// // //
			StretchPattern(pDoc);
			break;
#ifdef _DEBUG
		default:
			AfxMessageBox(_T("TODO: Redo for this action is not implemented"));
#endif
	}
}

void CPatternAction::Update(CMainFrame *pMainFrm)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();

	switch (m_iAction) {
		case ACT_PATTERN_LENGTH:
			pDoc->SetPatternLength(m_pUndoState->Track, m_iNewPatternLen);		// // //
			pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
			pMainFrm->UpdateControls();
			break;
	}
}

void CPatternAction::RestoreSelection(CPatternEditor *pPatternEditor) const
{
	if (m_bSelecting)
		pPatternEditor->SetSelection(m_selection);
	else
		pPatternEditor->CancelSelection();
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
		return m_OldNote.Instrument < MAX_INSTRUMENTS;
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
	bool bUpdate = false;
	int Val;

	const auto ScrollFunc = [&] (unsigned char &Old, int Limit) {
		int New = static_cast<int>(Old) + m_iAmount;
		if (theApp.GetSettings()->General.bWrapPatternValue) {
			New %= Limit;
			if (New < 0) Val += Limit;
		}
		else {
			if (New < 0) New = 0;
			if (New >= Limit) New = Limit - 1;
		}
		New = static_cast<unsigned char>(New);
		bUpdate = New != Old;
		Old = New;
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

	if (bUpdate)
		pDoc->SetNoteData(STATE_EXPAND(m_pUndoState), &Note);
}



CPActionClearSel::CPActionClearSel() :
	CPatternAction(ACT_EDIT_DELETE)
{
}

bool CPActionClearSel::SaveState(const CMainFrame *pMainFrm)
{
	const CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	m_pUndoClipData = pPatternEditor->CopyRaw(m_pUndoState->Selection);
	return true;
}

void CPActionClearSel::Undo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	pPatternEditor->PasteRaw(m_pUndoClipData);
}

void CPActionClearSel::Redo(CMainFrame *pMainFrm) const
{
	auto it = GetIterators(pMainFrm);		// // //
	const column_t ColStart = CPatternEditor::GetSelectColumn(it.first.m_iColumn);		// // //
	const column_t ColEnd = CPatternEditor::GetSelectColumn(it.second.m_iColumn);

	stChanNote NoteData, Blank { };		// // //

	do {
		for (int i = it.first.m_iChannel; i <= it.second.m_iChannel; ++i) {
			it.first.Get(i, &NoteData);
			CopyNoteSection(&NoteData, &Blank, PASTE_DEFAULT,
							i == it.first.m_iChannel ? ColStart : COLUMN_NOTE,
							i == it.second.m_iChannel ? ColEnd : COLUMN_EFF4);		// // //
			it.first.Set(i, &NoteData);
		}
	} while (++it.first <= it.second);
}



CPActionDeleteAtSel::CPActionDeleteAtSel() :
	CPatternAction(ACT_EDIT_DELETE_ROWS)
{
}

bool CPActionDeleteAtSel::SaveState(const CMainFrame *pMainFrm)
{
	int Frame = m_pUndoState->Cursor.m_iFrame;
	if (!(m_pUndoState->Selection.GetFrameStart() <= Frame && m_pUndoState->Selection.GetFrameEnd() >= Frame))
		return false;
	const CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	m_pUndoClipData = pPatternEditor->CopyEntire();
	return true;
}

void CPActionDeleteAtSel::Undo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	pPatternEditor->PasteEntire(m_pUndoClipData);
}

void CPActionDeleteAtSel::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();

	const CSelection &Sel = m_pUndoState->Selection;
	const int Frame = m_pUndoState->Cursor.m_iFrame;		// // //
	const column_t ColStart = CPatternEditor::GetSelectColumn(Sel.GetColStart());		// // //
	const column_t ColEnd = CPatternEditor::GetSelectColumn(Sel.GetColEnd());
	stChanNote Target, Source;
	
	CPatternIterator it = GetStartIterator();		// // //
	it.m_iFrame = Frame;
	it.m_iRow = (Sel.GetFrameStart() < Frame) ? 0 : Sel.GetRowStart();
	CPatternIterator front {it};
	front.m_iRow = (Sel.GetFrameEnd() > Frame) ? pDoc->GetPatternLength(m_pUndoState->Track) : Sel.GetRowEnd() + 1;

	while (it.m_iFrame == Frame) {
		for (int i = Sel.GetChanStart(); i <= Sel.GetChanEnd(); ++i) {
			it.Get(i, &Target);
			if (front.m_iFrame == Frame)
				front.Get(i, &Source);
			else
				Source = stChanNote { };
			CopyNoteSection(&Target, &Source, PASTE_DEFAULT, (i == Sel.GetChanStart()) ? ColStart : COLUMN_NOTE,
				(i == Sel.GetChanEnd()) ? ColEnd : COLUMN_EFF4);
			it.Set(i, &Target);
		}
		++it;
		++front;
	}

	pPatternEditor->CancelSelection();
}



CPActionInsertAtSel::CPActionInsertAtSel() :
	CPatternAction(ACT_INSERT_SEL_ROWS)
{
}

bool CPActionInsertAtSel::SaveState(const CMainFrame *pMainFrm)
{
	const CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	m_pUndoClipData = pPatternEditor->CopyEntire();
	return true;
}

void CPActionInsertAtSel::Undo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	pPatternEditor->PasteEntire(m_pUndoClipData);
}

void CPActionInsertAtSel::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	for (int i = m_pUndoState->Selection.GetChanStart(); i <= m_pUndoState->Selection.GetChanEnd(); ++i)
		pDoc->InsertRow(m_pUndoState->Track, m_pUndoState->Cursor.m_iFrame, i, m_pUndoState->Selection.GetRowStart());
}



CPActionReplaceInst::CPActionReplaceInst(unsigned Index) :
	CPatternAction(ACT_REPLACE_INSTRUMENT), m_iInstrumentIndex(Index)
{
}

bool CPActionReplaceInst::SaveState(const CMainFrame *pMainFrm)
{
	if (m_iInstrumentIndex > static_cast<unsigned>(MAX_INSTRUMENTS))
		return false;
	const CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	if (!m_pUndoState->IsSelecting)
		return false;
	m_pUndoClipData = pPatternEditor->CopyRaw();
	return true;
}

void CPActionReplaceInst::Undo(CMainFrame *pMainFrm) const
{
	CPatternEditor *pPatternEditor = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor();
	pPatternEditor->PasteRaw(m_pUndoClipData);
}

void CPActionReplaceInst::Redo(CMainFrame *pMainFrm) const
{
	CFamiTrackerDoc *pDoc = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetDocument();
	CPatternIterator it = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();
	const CSelection &Sel = m_pUndoState->Selection;

	const int cBegin = Sel.GetChanStart() + (Sel.IsColumnSelected(COLUMN_INSTRUMENT, Sel.GetChanStart()) ? 0 : 1);
	const int cEnd = Sel.GetChanEnd() - (Sel.IsColumnSelected(COLUMN_INSTRUMENT, Sel.GetChanEnd()) ? 0 : 1);

	stChanNote Note;
	do {
		for (int i = cBegin; i <= cEnd; ++i) {
			it.Get(i, &Note);
			if (Note.Instrument != MAX_INSTRUMENTS)
				Note.Instrument = m_iInstrumentIndex;
			it.Set(i, &Note);
		}
	} while (++it <= End);
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
