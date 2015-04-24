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

#include <algorithm>
#include <vector>		// // //
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "Settings.h"		// // //
#include "MainFrm.h"
#include "PatternEditor.h"
#include "PatternAction.h"

// CPatternAction /////////////////////////////////////////////////////////////////
//
// Undo/redo commands for pattern editor
//

// TODO: optimize the cases where it's not necessary to store the whole pattern
// TODO: split into several classes?

CPatternAction::CPatternAction(int iAction) : 
	CAction(iAction), 
	m_pClipData(NULL), 
	m_pUndoClipData(NULL),
	m_pAuxiliaryClipData(NULL),		// // //
	m_iStretchMap(std::vector<int>())		// // //
{
}

CPatternAction::~CPatternAction()
{
	SAFE_RELEASE(m_pClipData);
	SAFE_RELEASE(m_pUndoClipData);
}

void CPatternAction::SetNote(stChanNote &Note)
{
	m_NewNote = Note;
}

void CPatternAction::SetDelete(bool PullUp, bool Back)
{
	m_bPullUp = PullUp;
	m_bBack = Back;
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

void CPatternAction::SetInstrument(int Instrument)
{
	m_iInstrument = Instrument;
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

void CPatternAction::SetClickedChannel(int Channel)
{
	m_iClickedChannel = Channel;
}

void CPatternAction::SetStretchMap(const std::vector<int> Map)		// // //
{
	m_iStretchMap = Map;
}

void CPatternAction::SaveEntire(const CPatternEditor *pPatternEditor)
{
	// (avoid when possible)
	m_pUndoClipData = pPatternEditor->CopyEntire();
}

void CPatternAction::RestoreEntire(CPatternEditor *pPatternEditor)
{
	pPatternEditor->PasteEntire(m_pUndoClipData);
}

bool CPatternAction::SetTargetSelection(CPatternEditor *pPatternEditor)		// // //
{
	CCursorPos Start;
	CSelection New;

	if ((m_iPastePos == PASTE_SELECTION || m_iPastePos == PASTE_FILL) && !m_bSelecting)
		m_iPastePos = PASTE_CURSOR;

	switch (m_iPastePos) {
	case PASTE_CURSOR:
		Start.m_iFrame = m_iUndoFrame;
		Start.m_iRow = m_iUndoRow;
		Start.m_iChannel = m_iUndoChannel;
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

	CPatternIterator End(pPatternEditor, m_iUndoTrack, Start);
	
	if (m_iPasteMode == PASTE_INSERT) {
		End.m_iFrame = Start.m_iFrame;
		End.m_iRow = pPatternEditor->GetCurrentPatternLength(End.m_iFrame) - 1;
	}
	else
		End += m_pClipData->ClipInfo.Rows - 1;

	if (m_iPastePos == PASTE_FILL) {
		End.m_iFrame = m_selection.GetFrameEnd();
		End.m_iRow = m_selection.GetRowEnd();
		End.m_iChannel = m_selection.GetChanEnd();
		bool Cut = (End.m_iChannel - Start.m_iChannel + 1) % m_pClipData->ClipInfo.Channels == 0;
		Start.m_iColumn = CPatternEditor::GetCursorStartColumn(m_pClipData->ClipInfo.StartColumn);
		End.m_iColumn = CPatternEditor::GetCursorStartColumn(Cut ? m_pClipData->ClipInfo.EndColumn :
			4 + 3 * CFamiTrackerDoc::GetDoc()->GetEffColumns(m_iUndoTrack, End.m_iChannel));
	}
	else if (m_iPastePos == PASTE_DRAG) {
		Start.m_iColumn = m_dragTarget.GetColStart();
		End.m_iColumn = m_dragTarget.GetColEnd();
	}
	else {
		End.m_iChannel += m_pClipData->ClipInfo.Channels - 1;
		Start.m_iColumn = CPatternEditor::GetCursorStartColumn(m_pClipData->ClipInfo.StartColumn);
		End.m_iColumn = CPatternEditor::GetCursorStartColumn(m_pClipData->ClipInfo.EndColumn);
	}

	if (Start.m_iChannel == End.m_iChannel && Start.m_iColumn >= COLUMN_EFF1 && End.m_iColumn >= COLUMN_EFF1) {
		if (m_iPastePos != PASTE_DRAG) {
			Start.m_iColumn += 3 * (CPatternEditor::GetSelectColumn(m_iUndoColumn) - m_pClipData->ClipInfo.StartColumn);
			End.m_iColumn += 3 * (CPatternEditor::GetSelectColumn(m_iUndoColumn) - m_pClipData->ClipInfo.StartColumn);
			End.m_iColumn = std::min(End.m_iColumn, 15);
		}
	}

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
		int Confirm;
		switch (Cond) {
		case SEL_REPEATED_ROW:
			Confirm = AfxMessageBox(IDS_PASTE_REPEATED_ROW, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
			break;
		case SEL_NONTERMINAL_SKIP: case SEL_TERMINAL_SKIP:
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
	m_pUndoClipData = pPatternEditor->CopyRaw();
}

void CPatternAction::PasteSelection(CPatternEditor *pPatternEditor)		// // //
{
	pPatternEditor->Paste(m_pUndoClipData, PASTE_DEFAULT, PASTE_SELECTION);
}

void CPatternAction::CopyAuxiliary(const CPatternEditor *pPatternEditor)		// // //
{
	m_pAuxiliaryClipData = pPatternEditor->CopyRaw();
}

void CPatternAction::PasteAuxiliary(CPatternEditor *pPatternEditor)		// // //
{
	pPatternEditor->Paste(m_pAuxiliaryClipData, PASTE_DEFAULT, PASTE_SELECTION);
}

void CPatternAction::IncreaseRowAction(CFamiTrackerDoc *pDoc) const
{
	stChanNote Note;
	bool bUpdate = false;
	
	pDoc->GetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow, &Note);

	switch (m_iUndoColumn) {
		case C_INSTRUMENT1:
		case C_INSTRUMENT2: 
			if (Note.Instrument < MAX_INSTRUMENTS - 1) {		// // //
				++Note.Instrument;
				bUpdate = true;
			}
			break;
		case C_VOLUME: 
			if (Note.Vol < MAX_VOLUME - 1) {		// // //
				++Note.Vol;
				bUpdate = true;
			}
			break;
		case C_EFF_NUM: case C_EFF_PARAM1: case C_EFF_PARAM2: 
			if (Note.EffParam[0] < 0xFF && Note.EffNumber[0] != EF_NONE) {		// // //
				++Note.EffParam[0];
				bUpdate = true;
			}
			break;
		case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2: 
			if (Note.EffParam[1] < 0xFF && Note.EffNumber[1] != EF_NONE) {		// // //
				++Note.EffParam[1];
				bUpdate = true;
			}
			break;
		case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2: 
			if (Note.EffParam[2] < 0xFF && Note.EffNumber[2] != EF_NONE) {		// // //
				++Note.EffParam[2];
				bUpdate = true;
			}
			break;
		case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2: 
			if (Note.EffParam[3] < 0xFF && Note.EffNumber[3] != EF_NONE) {		// // //
				++Note.EffParam[3];
				bUpdate = true;
			}
			break;
	}

	if (bUpdate)
		pDoc->SetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow, &Note);
}

void CPatternAction::DecreaseRowAction(CFamiTrackerDoc *pDoc) const
{
	stChanNote Note;
	bool bUpdate = false;
	
	pDoc->GetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow, &Note);

	switch (m_iUndoColumn) {
		case C_INSTRUMENT1:
		case C_INSTRUMENT2:
			if (Note.Instrument > 0 && Note.Instrument != MAX_INSTRUMENTS) {		// // //
				--Note.Instrument;
				bUpdate = true;
			}
			break;
		case C_VOLUME: 
			if (Note.Vol > 0 && Note.Vol != MAX_VOLUME) {		// // //
				--Note.Vol;
				bUpdate = true;
			}
			break;
		case C_EFF_NUM: case C_EFF_PARAM1: case C_EFF_PARAM2: 
			if (Note.EffParam[0] > 0 && Note.EffNumber[0] != EF_NONE) {		// // //
				--Note.EffParam[0];
				bUpdate = true;
			}
			break;
		case C_EFF2_NUM: case C_EFF2_PARAM1: case C_EFF2_PARAM2: 
			if (Note.EffParam[1] > 0 && Note.EffNumber[1] != EF_NONE) {		// // //
				--Note.EffParam[1];
				bUpdate = true;
			}
			break;
		case C_EFF3_NUM: case C_EFF3_PARAM1: case C_EFF3_PARAM2: 
			if (Note.EffParam[2] > 0 && Note.EffNumber[2] != EF_NONE) {		// // //
				--Note.EffParam[2];
				bUpdate = true;
			}
			break;
		case C_EFF4_NUM: case C_EFF4_PARAM1: case C_EFF4_PARAM2: 	
			if (Note.EffParam[3] > 0 && Note.EffNumber[3] != EF_NONE) {		// // //
				--Note.EffParam[3];
				bUpdate = true;
			}
			break;
	}

	if (bUpdate)
		pDoc->SetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow, &Note);
}

void CPatternAction::InsertRows(CFamiTrackerDoc *pDoc) const
{
	for (int i = m_selection.GetChanStart(); i <= m_selection.GetChanEnd(); ++i) {
		pDoc->InsertRow(m_iUndoTrack, m_iUndoFrame, i, m_selection.GetRowStart());
	}
}

void CPatternAction::PullUpRows(CFamiTrackerDoc *pDoc) const
{
	const int ColStart = CPatternEditor::GetSelectColumn(m_selection.GetColStart());		// // //
	const int ColEnd = CPatternEditor::GetSelectColumn(m_selection.GetColEnd());
	stChanNote Target, Source;
	
	CPatternIterator it = GetStartIterator();		// // //
	it.m_iFrame = m_iUndoFrame;
	it.m_iRow = (m_selection.GetFrameStart() < m_iUndoFrame) ? 0 : m_selection.GetRowStart();
	CPatternIterator front = CPatternIterator(it);
	front.m_iRow = (m_selection.GetFrameEnd() > m_iUndoFrame) ? pDoc->GetPatternLength(m_iUndoTrack) : m_selection.GetRowEnd() + 1;

	while (it.m_iFrame == m_iUndoFrame) {
		for (int i = m_selection.GetChanStart(); i <= m_selection.GetChanEnd(); ++i) {
			it.Get(i, &Target);
			if (front.m_iFrame == m_iUndoFrame)
				front.Get(i, &Source);
			else
				Source = BLANK_NOTE;
			CopyNoteSection(&Target, &Source, PASTE_DEFAULT, (i == m_selection.GetChanStart()) ? ColStart : COLUMN_NOTE,
				(i == m_selection.GetChanEnd()) ? ColEnd : COLUMN_EFF4);
			it.Set(i, &Target);
		}
		it++;
		front++;
	}
}

void CPatternAction::StretchPattern(CFamiTrackerDoc *pDoc) const		// // //
{
	CPatternIterator it = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();
	const int ColStart = CPatternEditor::GetSelectColumn(m_selection.GetColStart());		// // //
	const int ColEnd = CPatternEditor::GetSelectColumn(m_selection.GetColEnd());
	stChanNote Target, Source;
	
	int Pos = 0;
	int Offset = 0;
	do {
		for (int i = 0; i <= m_selection.GetChanEnd() - m_selection.GetChanStart(); ++i) {
			if (Offset < m_iSelectionSize && m_iStretchMap[Pos] > 0)
				Source = *(m_pUndoClipData->GetPattern(i, Offset));
			else 
				Source = BLANK_NOTE;		// // //
			it.Get(i + m_selection.GetChanStart(), &Target);
			CopyNoteSection(&Target, &Source, PASTE_DEFAULT, i == 0 ? ColStart : COLUMN_NOTE,
				i == m_selection.GetChanEnd() - m_selection.GetChanStart() ? ColEnd : COLUMN_EFF4);
			it.Set(i + m_selection.GetChanStart(), &Target);
		}
		Offset += m_iStretchMap[Pos++];
		Pos %= m_iStretchMap.size();
	} while (++it <= End);
}

void CPatternAction::ReplaceInstrument(CFamiTrackerDoc *pDoc) const
{
	CPatternIterator it = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();
	
	int cBegin = m_selection.GetChanStart();
	int cEnd = m_selection.GetChanEnd();
	if (!m_selection.IsColumnSelected(COLUMN_INSTRUMENT, cBegin)) cBegin++;		// // //
	if (!m_selection.IsColumnSelected(COLUMN_INSTRUMENT, cEnd)) cEnd--;
	do {
		for (int i = cBegin; i <= cEnd; ++i) {
			stChanNote Note;
			it.Get(i, &Note);
			if (Note.Instrument != MAX_INSTRUMENTS)
				Note.Instrument = m_iInstrument;
			it.Set(i, &Note);
		}
	} while (++it <= End);
}

void CPatternAction::Transpose(CFamiTrackerDoc *pDoc) const
{
	CPatternIterator it = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();
	int ChanStart = m_selection.GetChanStart();
	int ChanEnd	= m_selection.GetChanEnd();
	stChanNote Note;

	if (!m_bSelecting) {
		ChanStart = m_iUndoChannel;
		ChanEnd = m_iUndoChannel;
	}
	
	const bool bSingular = (it == End) && (ChanStart == ChanEnd);
	int Row = 0;		// // //
	do {
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
				NewNote = std::max(std::min(NewNote, NOTE_COUNT - 1), 0);
				Note.Note = GET_NOTE(NewNote);
				Note.Octave = GET_OCTAVE(NewNote);
			}
			it.Set(i, &Note);
		}
		Row++;
	} while (++it <= End);
}

void CPatternAction::Interpolate(CFamiTrackerDoc *pDoc) const
{
	const CPatternIterator End = GetEndIterator();		// // //
	stChanNote StartData, EndData;

	for (int i = m_selection.GetChanStart(); i <= m_selection.GetChanEnd(); ++i) {
		const int Columns = pDoc->GetEffColumns(m_iUndoTrack, i) + 4;		// // //
		for (int j = 0; j < Columns; ++j) {
			CPatternIterator it = GetStartIterator();		// // //
			it.Get(i, &StartData);
			End.Get(i, &EndData);
			double StartValHi, StartValLo;		// // //
			double EndValHi, EndValLo;
			double DeltaHi, DeltaLo;
			bool TwoParam = false;
			int Effect;
			switch (j) {
			case 0: // // // Note
				if (!m_selection.IsColumnSelected(COLUMN_NOTE, i)
					|| StartData.Note < NOTE_C || StartData.Note > NOTE_B
					|| EndData.Note < NOTE_C || EndData.Note > NOTE_B)
					continue;
				StartValLo = (float)MIDI_NOTE(StartData.Octave, StartData.Note);
				EndValLo = (float)MIDI_NOTE(EndData.Octave, EndData.Note);
				break;
			case 1: // // // Instrument
				if (!m_selection.IsColumnSelected(COLUMN_INSTRUMENT, i)
					|| StartData.Instrument == MAX_INSTRUMENTS || EndData.Instrument == MAX_INSTRUMENTS)
					continue;
				StartValLo = (float)StartData.Instrument;
				EndValLo = (float)EndData.Instrument;
				break;
			case 2:	// // // Volume
				if (!m_selection.IsColumnSelected(COLUMN_VOLUME, i)
					|| StartData.Vol == MAX_VOLUME || EndData.Vol == MAX_VOLUME)
					continue;
				StartValLo = (float)StartData.Vol;
				EndValLo = (float)EndData.Vol;
				break;
			case 3:	// // // Effects
			case 4:
			case 5:
			case 6:
				if (!m_selection.IsColumnSelected(COLUMN_EFF1 + j - 3, i)
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
	const int ColStart = CPatternEditor::GetSelectColumn(m_selection.GetColStart());
	const int ColEnd = CPatternEditor::GetSelectColumn(m_selection.GetColEnd());
	stChanNote NoteBegin, NoteEnd, Temp;
	
	while (itb < ite) {
		for (int c = m_selection.GetChanStart(); c <= m_selection.GetChanEnd(); ++c) {
			itb.Get(c, &NoteBegin);
			ite.Get(c, &NoteEnd);
			if (c == m_selection.GetChanStart()) {		// // //
				Temp = NoteEnd;
				CopyNoteSection(&NoteEnd, &NoteBegin, PASTE_DEFAULT, 0, ColStart - 1);
				CopyNoteSection(&NoteBegin, &Temp, PASTE_DEFAULT, 0, ColStart - 1);
			}
			if (c == m_selection.GetChanEnd()) {
				Temp = NoteEnd;
				CopyNoteSection(&NoteEnd, &NoteBegin, PASTE_DEFAULT, ColEnd + 1, COLUMN_EFF4);
				CopyNoteSection(&NoteBegin, &Temp, PASTE_DEFAULT, ColEnd + 1, COLUMN_EFF4);
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

	int ChanStart	= m_bSelecting ? m_selection.GetChanStart() : m_iUndoChannel;
	int ChanEnd		= m_bSelecting ? m_selection.GetChanEnd() : m_iUndoChannel;
	int ColStart	= CPatternEditor::GetSelectColumn(m_bSelecting ? m_selection.GetColStart() : m_iUndoColumn);
	int ColEnd		= CPatternEditor::GetSelectColumn(m_bSelecting ? m_selection.GetColEnd() : m_iUndoColumn);

	const bool bWarp = theApp.GetSettings()->General.bWrapPatternValue;
	const bool bSingular = (it == End) && (ChanStart == ChanEnd) && (ColStart == ColEnd) && (m_iScrollValue == -1 || m_iScrollValue == 1);
	
	int Row = 0;
	do {
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			Note = *(m_pUndoClipData->GetPattern(i - ChanStart, Row));		// // //
			for (int k = 1; k < COLUMNS; ++k) {
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
							Note.Instrument = std::max(std::min(Val, MAX_INSTRUMENTS - 1), 0);
						}
					}
					break;
				case COLUMN_VOLUME:
					if (Note.Vol != MAX_VOLUME) {
						if (bWarp)		// // //
							Note.Vol = (Note.Vol + m_iScrollValue + MAX_VOLUME) % MAX_VOLUME;
						else {
							int Val = Note.Vol + m_iScrollValue;
							Note.Vol = std::max(std::min(Val, MAX_VOLUME - 1), 0);
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
								int CurCol = CFamiTrackerView::GetView()->GetPatternEditor()->GetColumn();
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
	} while (++it <= End);
}

void CPatternAction::DeleteSelection(CFamiTrackerDoc *pDoc) const
{
	// Delete selection
	CPatternIterator it = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();
	const int ColStart = CPatternEditor::GetSelectColumn(m_selection.GetColStart());		// // //
	const int ColEnd = CPatternEditor::GetSelectColumn(m_selection.GetColEnd());

	stChanNote NoteData, Blank = BLANK_NOTE;		// // //

	do {
		for (int i = m_selection.GetChanStart(); i <= m_selection.GetChanEnd(); ++i) {
			it.Get(i, &NoteData);
			CopyNoteSection(&NoteData, &Blank, PASTE_DEFAULT, i == m_selection.GetChanStart() ? ColStart : COLUMN_NOTE,
				i == m_selection.GetChanEnd() ? ColEnd : COLUMN_EFF4);		// // //
			it.Set(i, &NoteData);
		}
	} while (++it <= End);
}

CPatternIterator CPatternAction::GetStartIterator() const		// // //
{
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(AfxGetMainWnd());
	CCursorPos Pos = m_selection.m_cpStart < m_selection.m_cpEnd ? m_selection.m_cpStart : m_selection.m_cpEnd;
	if (!m_bSelecting) Pos = CCursorPos(m_iUndoRow, m_iUndoChannel, m_iUndoColumn, m_iUndoFrame);
	return CPatternIterator(static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor(), m_iUndoTrack, Pos);
}

CPatternIterator CPatternAction::GetEndIterator() const
{
	CMainFrame *pMainFrm = static_cast<CMainFrame*>(AfxGetMainWnd());
	CCursorPos Pos = m_selection.m_cpStart < m_selection.m_cpEnd ? m_selection.m_cpEnd : m_selection.m_cpStart;
	if (!m_bSelecting) Pos = CCursorPos(m_iUndoRow, m_iUndoChannel, m_iUndoColumn, m_iUndoFrame);
	return CPatternIterator(static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView())->GetPatternEditor(), m_iUndoTrack, Pos);
}

// Undo / Redo base methods

bool CPatternAction::SaveState(CMainFrame *pMainFrm)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();
	CPatternEditor *pPatternEditor = pView->GetPatternEditor();

	// Save undo cursor position
	m_iUndoTrack	= pMainFrm->GetSelectedTrack();
	m_iUndoFrame	= m_iRedoFrame	 = pPatternEditor->GetFrame();
	m_iUndoChannel  = m_iRedoChannel = pPatternEditor->GetChannel();
	m_iUndoRow		= m_iRedoRow	 = pPatternEditor->GetRow();
	m_iUndoColumn   = m_iRedoColumn  = pPatternEditor->GetColumn();

	m_bSelecting = pPatternEditor->IsSelecting();
	m_selection = pPatternEditor->GetSelection();
	m_iSelectionSize = pPatternEditor->GetSelectionSize();		// // //
	sel_condition_t Cond = pPatternEditor->GetSelectionCondition();		// // //

	// Save old state
	switch (m_iAction) {
		case ACT_EDIT_NOTE:
			// Edit note
			pDoc->GetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow, &m_OldNote);
			break;
		case ACT_DELETE_ROW:
			// Delete row
			if (m_bBack && m_iUndoRow == 0)
				return false;
			pDoc->GetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow - (m_bBack ? 1 : 0), &m_OldNote);
			break;
		case ACT_INSERT_ROW:
			// Insert empty row
			pDoc->GetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, pDoc->GetPatternLength(m_iUndoTrack) - 1, &m_OldNote);
			break;
		case ACT_DRAG_AND_DROP:
			if (m_bDragDelete)
				CopyAuxiliary(pPatternEditor);
			// continue
		case ACT_EDIT_PASTE:
			if (!SetTargetSelection(pPatternEditor))		// // //
				return false;
			CopySelection(pPatternEditor);
			break;
		case ACT_EDIT_DELETE:		// // //
			CopySelection(pPatternEditor);		// // //
			break;
		case ACT_EDIT_DELETE_ROWS:
			if (!(m_selection.GetFrameStart() <= m_iUndoFrame && m_selection.GetFrameEnd() >= m_iUndoFrame))
				return false;
			// continue
		case ACT_INSERT_SEL_ROWS:
			SaveEntire(pPatternEditor);
			break;
		case ACT_TRANSPOSE:		// // //
		case ACT_SCROLL_VALUES:
			pPatternEditor->SetSelection(m_selection);
			CopySelection(pPatternEditor);
			break;
		case ACT_INTERPOLATE: // 0CC: Copy only the selection if it is guaranteed to remain unmodified
		case ACT_REVERSE:
		case ACT_STRETCH_PATTERN:		// // //
			if (!pPatternEditor->IsSelecting())
				return false;
			switch (Cond) {
			case SEL_REPEATED_ROW:
				pMainFrm->SetMessageText(IDS_SEL_REPEATED_ROW); break;
			case SEL_NONTERMINAL_SKIP:
				pMainFrm->SetMessageText(IDS_SEL_NONTERMINAL_SKIP); break;
			case SEL_TERMINAL_SKIP:
				pMainFrm->SetMessageText(IDS_SEL_TERMINAL_SKIP); break;
			}
			if (Cond != SEL_CLEAN) {
				MessageBeep(MB_ICONWARNING);
				return false;
			}
			CopySelection(pPatternEditor);
			break;
		case ACT_REPLACE_INSTRUMENT:
			if (!pPatternEditor->IsSelecting())
				return false;
			CopySelection(pPatternEditor);
			break;
		case ACT_INCREASE:
		case ACT_DECREASE:		// // //
			// Increase action
			pDoc->GetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow, &m_OldNote);
			break;
		case ACT_PATTERN_LENGTH:
			// Change pattern length
			m_iOldPatternLen = pDoc->GetPatternLength(m_iUndoTrack);
			break;
		case ACT_EXPAND_COLUMNS:
		case ACT_SHRINK_COLUMNS:		// // //
			// Add / remove effect column
			m_iUndoColumnCount = pDoc->GetEffColumns(m_iUndoTrack, m_iClickedChannel);
			break;
#ifdef _DEBUG_
		default:
			AfxMessageBox(_T("TODO Implement action for this command"));
#endif
	}

	// Redo will perform the action
	Redo(pMainFrm);

	return true;
}

void CPatternAction::Undo(CMainFrame *pMainFrm)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();
	CPatternEditor *pPatternEditor = pView->GetPatternEditor();
	int UpdateHint = UPDATE_PATTERN;

	// Save redo-position
	m_iRedoFrame	= pPatternEditor->GetFrame();
	m_iRedoChannel  = pPatternEditor->GetChannel();
	m_iRedoRow		= pPatternEditor->GetRow();
	m_iRedoColumn   = pPatternEditor->GetColumn();

	pPatternEditor->MoveToFrame(m_iUndoFrame);
	pPatternEditor->MoveToChannel(m_iUndoChannel);
	pPatternEditor->MoveToRow(m_iUndoRow);
	pPatternEditor->MoveToColumn(m_iUndoColumn);
	pPatternEditor->InvalidateCursor();

	switch (m_iAction) {
		case ACT_EDIT_NOTE:
		case ACT_INCREASE:		// // //
		case ACT_DECREASE:		// // //
			pDoc->SetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow, &m_OldNote);
			break;
		case ACT_DELETE_ROW:
			if (m_bPullUp)
				pDoc->InsertRow(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow - (m_bBack ? 1 : 0));
			pDoc->SetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow - (m_bBack ? 1 : 0), &m_OldNote);
			break;
		case ACT_INSERT_ROW:
			pDoc->PullUp(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow);
			pDoc->SetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, pDoc->GetPatternLength(m_iUndoTrack) - 1, &m_OldNote);
			break;
		case ACT_EDIT_PASTE:		// // //
		case ACT_EDIT_DELETE:
		case ACT_TRANSPOSE:
		case ACT_SCROLL_VALUES:
		case ACT_INTERPOLATE:
		case ACT_REVERSE:
		case ACT_REPLACE_INSTRUMENT:
		case ACT_STRETCH_PATTERN:		// // //
			RestoreSelection(pPatternEditor);
			PasteSelection(pPatternEditor);		// // //
			break;
		case ACT_INSERT_SEL_ROWS:
		case ACT_EDIT_DELETE_ROWS:
			RestoreSelection(pPatternEditor);
			RestoreEntire(pPatternEditor);
			break;
		case ACT_DRAG_AND_DROP:
			pPatternEditor->SetSelection(m_newSelection);
			PasteSelection(pPatternEditor);		// // //
			RestoreSelection(pPatternEditor);
			if (m_bDragDelete)
				PasteAuxiliary(pPatternEditor);
			break;
		case ACT_PATTERN_LENGTH:
			pDoc->SetPatternLength(m_iUndoTrack, m_iOldPatternLen);
			pMainFrm->UpdateControls();
			break;
		case ACT_EXPAND_COLUMNS:
		case ACT_SHRINK_COLUMNS:
			pDoc->SetEffColumns(m_iUndoTrack, m_iClickedChannel, m_iUndoColumnCount);
			UpdateHint = UPDATE_COLUMNS;
			break;

#ifdef _DEBUG_
		default:
			AfxMessageBox(_T("TODO Undo for this action is not implemented"));
#endif
	}

	pDoc->UpdateAllViews(NULL, UpdateHint);
}

void CPatternAction::Redo(CMainFrame *pMainFrm)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();
	CPatternEditor *pPatternEditor = pView->GetPatternEditor();
	int UpdateHint = UPDATE_PATTERN;

	pPatternEditor->MoveToFrame(m_iUndoFrame);
	pPatternEditor->MoveToChannel(m_iUndoChannel);
	pPatternEditor->MoveToRow(m_iUndoRow);
	pPatternEditor->MoveToColumn(m_iUndoColumn);
	pPatternEditor->InvalidateCursor();

	switch (m_iAction) {
		case ACT_EDIT_NOTE:
			pDoc->SetNoteData(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow, &m_NewNote);
			break;
		case ACT_DELETE_ROW:
			pDoc->ClearRowField(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow - (m_bBack ? 1 : 0), m_iUndoColumn);
			if (m_bPullUp)
				pDoc->PullUp(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow - (m_bBack ? 1 : 0));
			break;
		case ACT_INSERT_ROW:
			pDoc->InsertRow(m_iUndoTrack, m_iUndoFrame, m_iUndoChannel, m_iUndoRow);
			break;
		case ACT_EDIT_PASTE:
			pPatternEditor->Paste(m_pClipData, m_iPasteMode, m_iPastePos);		// // //
			break;		// // //
		case ACT_EDIT_DELETE:
			pPatternEditor->SetSelection(m_selection);
			DeleteSelection(pDoc);
			break;
		case ACT_EDIT_DELETE_ROWS:
			PullUpRows(pDoc);
			pPatternEditor->CancelSelection();
			break;
		case ACT_INSERT_SEL_ROWS:
			InsertRows(pDoc);
			break;
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
		case ACT_REPLACE_INSTRUMENT:
			pPatternEditor->SetSelection(m_selection);
			ReplaceInstrument(pDoc);
			break;
		case ACT_INCREASE:
			IncreaseRowAction(pDoc);
			break;
		case ACT_DECREASE:
			DecreaseRowAction(pDoc);
			break;
		case ACT_DRAG_AND_DROP:
			RestoreSelection(pPatternEditor);
			if (m_bDragDelete)
				DeleteSelection(pDoc);
			pPatternEditor->DragPaste(m_pClipData, &m_dragTarget, m_bDragMix);
			break;
		case ACT_PATTERN_LENGTH:
			pDoc->SetPatternLength(m_iUndoTrack, m_iNewPatternLen);
			pMainFrm->UpdateControls();
			break;
		case ACT_STRETCH_PATTERN:		// // //
			StretchPattern(pDoc);
			break;
		case ACT_EXPAND_COLUMNS:
			pDoc->SetEffColumns(m_iUndoTrack, m_iClickedChannel, m_iUndoColumnCount + 1);
			UpdateHint = UPDATE_COLUMNS;
			break;
		case ACT_SHRINK_COLUMNS:
			pDoc->SetEffColumns(m_iUndoTrack, m_iClickedChannel, m_iUndoColumnCount - 1);
			UpdateHint = UPDATE_COLUMNS;
			break;
#ifdef _DEBUG_
		default:
			AfxMessageBox(_T("TODO: Redo for this action is not implemented"));
#endif
	}

	pPatternEditor->MoveToFrame(m_iRedoFrame);
	pPatternEditor->MoveToChannel(m_iRedoChannel);
	pPatternEditor->MoveToRow(m_iRedoRow);
	pPatternEditor->MoveToColumn(m_iRedoColumn);
	pPatternEditor->InvalidateCursor();

	pDoc->UpdateAllViews(NULL, UpdateHint);
}

void CPatternAction::Update(CMainFrame *pMainFrm)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(pMainFrm->GetActiveView());
	CFamiTrackerDoc *pDoc = pView->GetDocument();

	switch (m_iAction) {
		case ACT_PATTERN_LENGTH:
			pDoc->SetPatternLength(m_iUndoTrack, m_iNewPatternLen);
			pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
			pMainFrm->UpdateControls();
			break;
	}
}

void CPatternAction::RestoreSelection(CPatternEditor *pPatternEditor)
{
	if (m_bSelecting)
		pPatternEditor->SetSelection(m_selection);
	else
		pPatternEditor->CancelSelection();
}
