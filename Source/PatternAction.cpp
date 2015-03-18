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
	const int Channels = pDoc->GetAvailableChannels();
	const int PatternLength = pDoc->GetPatternLength(m_iUndoTrack);
	for (int i = std::max(m_selection.GetChanStart(), 0); i <= std::min(m_selection.GetChanEnd(), Channels - 1); ++i) {
		for (int j = std::max(m_selection.GetRowStart(), 0); j <= std::min(m_selection.GetRowEnd(), PatternLength - 1); ++j) {
			pDoc->PullUp(m_iUndoTrack, m_iUndoFrame, i, m_selection.GetRowStart());
		}		
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
	CPatternClipData *Original = static_cast<CFamiTrackerView*>(static_cast<CMainFrame*>(AfxGetMainWnd())->GetActiveView())->GetPatternEditor()->Copy();

	do {
		for (int i = 0; i <= m_selection.GetChanEnd() - m_selection.GetChanStart(); ++i) {
			if (Offset < m_iSelectionSize && m_iStretchMap[Pos] > 0)
				Source = *(Original->GetPattern(i, Offset));
			else {
				memset(&Source, 0, sizeof(stChanNote));
				Source.Instrument = MAX_INSTRUMENTS;
				Source.Vol = MAX_VOLUME;
			}
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
	
	do {
		for (int i = m_selection.GetChanStart(); i <= m_selection.GetChanEnd(); ++i) {
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
	stChanNote Note;
	int ChanStart = m_selection.GetChanStart();
	int ChanEnd	= m_selection.GetChanEnd();

	if (!m_bSelecting) {
		ChanStart = m_iUndoChannel;
		ChanEnd = m_iUndoChannel;
	}

	do {
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			if (!m_selection.IsColumnSelected(COLUMN_NOTE, i))
				continue;
			it.Get(i, &Note);
			if (Note.Note == NONE || Note.Note == HALT || Note.Note == RELEASE)
				continue;
			switch (m_iTransposeMode) {
				case TRANSPOSE_DEC_NOTES:
					if (!m_bSelecting && Note.Note == ECHO && Note.Octave > 1)		// // //
						Note.Octave--;
					else if (Note.Note > C) 
						Note.Note--;
					else if (Note.Octave > 0) {
						Note.Note = B;
						Note.Octave--;
					}
					break;
				case TRANSPOSE_INC_NOTES:
					if (!m_bSelecting && Note.Note == ECHO && Note.Octave < ECHO_BUFFER_LENGTH)		// // //
						Note.Octave++;
					else if (Note.Note < B)
						Note.Note++;
					else if (Note.Octave < 7) {
						Note.Note = C;
						Note.Octave++;
					}
					break;
				case TRANSPOSE_DEC_OCTAVES:
					if (Note.Octave > 0 && (Note.Note != ECHO || (!m_bSelecting && Note.Octave > 1)))		// // //
						Note.Octave--;
					break;
				case TRANSPOSE_INC_OCTAVES:
					if (Note.Octave < 7 && (Note.Note != ECHO || (!m_bSelecting && Note.Octave < ECHO_BUFFER_LENGTH)))		// // //
						Note.Octave++;
					break;
			}
			it.Set(i, &Note);
		}
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
					|| StartData.Note < C || StartData.Note > B || EndData.Note < C || EndData.Note > B)
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

			do {
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
				StartValLo += DeltaLo;
				StartValHi += DeltaHi;
				it.Set(i, &EndData);
			} while (++it <= End);
		}
	}
}

void CPatternAction::Reverse(CFamiTrackerDoc *pDoc) const
{
	stChanNote NoteBegin, NoteEnd;
	CPatternIterator itb = GetStartIterator();		// // //
	CPatternIterator ite = GetEndIterator();
	
	while (itb < ite) {
		for (int c = m_selection.GetChanStart(); c <= m_selection.GetChanEnd(); ++c) {
			itb.Get(c, &NoteBegin);
			ite.Get(c, &NoteEnd);
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

	int ChanStart = m_selection.GetChanStart();
	int ChanEnd = m_selection.GetChanEnd();
	int ColStart = CPatternEditor::GetSelectColumn(m_selection.GetColStart());
	int ColEnd = CPatternEditor::GetSelectColumn(m_selection.GetColEnd());
	if (!m_bSelecting) {
		ChanStart = m_iUndoChannel;
		ChanEnd = m_iUndoChannel;
		ColStart = CPatternEditor::GetSelectColumn(m_iUndoColumn);
		ColEnd = CPatternEditor::GetSelectColumn(m_iUndoColumn);
	}

	bool bWarp = theApp.GetSettings()->General.bWrapPatternValue;		// // //
	bool bSingular = (it == End) && (ChanStart == ChanEnd) && (ColStart == ColEnd);		// // //
	
	do {
		for (int i = ChanStart; i <= ChanEnd; ++i) {
			for (int k = 1; k < COLUMNS; ++k) {
				if (i == ChanStart && k < ColStart)
					continue;
				if (i == ChanEnd && k > ColEnd)
					continue;
				it.Get(i, &Note);
				switch (k) {
					case COLUMN_INSTRUMENT:
						if (Note.Instrument != MAX_INSTRUMENTS) {
							if (bWarp)		// // //
								Note.Instrument = (Note.Instrument + m_iScrollValue + MAX_INSTRUMENTS) % MAX_INSTRUMENTS;
							else if ((m_iScrollValue < 0 && Note.Instrument > 0) || (m_iScrollValue > 0 && Note.Instrument < MAX_INSTRUMENTS - 1))
								Note.Instrument += m_iScrollValue;
						}
						break;
					case COLUMN_VOLUME:
						if (Note.Vol != MAX_VOLUME) {
							if (bWarp)		// // //
								Note.Vol = (Note.Vol + m_iScrollValue + MAX_VOLUME) % MAX_VOLUME;
							else if ((m_iScrollValue < 0 && Note.Vol > 0) || (m_iScrollValue > 0 && Note.Vol < 0x0F))
								Note.Vol += m_iScrollValue;
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
				it.Set(i, &Note);
			}
		}
	} while (++it <= End);
}

void CPatternAction::DeleteSelection(CFamiTrackerDoc *pDoc) const
{
	// Delete selection
	CPatternIterator it = GetStartIterator();		// // //
	const CPatternIterator End = GetEndIterator();

	do {
		for (int i = m_selection.GetChanStart(); i <= m_selection.GetChanEnd(); ++i) {
			stChanNote NoteData;
			it.Get(i, &NoteData);

			if (m_selection.IsColumnSelected(COLUMN_NOTE, i)) {
				NoteData.Note = 0;
				NoteData.Octave = 0;
			}
			if (m_selection.IsColumnSelected(COLUMN_INSTRUMENT, i)) {
				NoteData.Instrument = MAX_INSTRUMENTS;
			}
			if (m_selection.IsColumnSelected(COLUMN_VOLUME, i)) {
				NoteData.Vol = MAX_VOLUME;
			}
			if (m_selection.IsColumnSelected(COLUMN_EFF1, i)) {
				NoteData.EffNumber[0] = NoteData.EffParam[0] = 0;
			}
			if (m_selection.IsColumnSelected(COLUMN_EFF2, i)) {
				NoteData.EffNumber[1] = NoteData.EffParam[1] = 0;
			}
			if (m_selection.IsColumnSelected(COLUMN_EFF3, i)) {
				NoteData.EffNumber[2] = NoteData.EffParam[2] = 0;
			}
			if (m_selection.IsColumnSelected(COLUMN_EFF4, i)) {
				NoteData.EffNumber[3] = NoteData.EffParam[3] = 0;
			}
		
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
		case ACT_EDIT_PASTE:
		case ACT_EDIT_DELETE:		// // //
		case ACT_EDIT_DELETE_ROWS:
		case ACT_INSERT_SEL_ROWS:
		case ACT_TRANSPOSE:
		case ACT_SCROLL_VALUES:
		case ACT_DRAG_AND_DROP:
			SaveEntire(pPatternEditor);		// // //
			break;
		case ACT_INTERPOLATE:
			if (!pPatternEditor->IsSelecting() || m_iSelectionSize < 3)		// // //
				return false;
			SaveEntire(pPatternEditor);
			break;
		case ACT_REVERSE:
		case ACT_REPLACE_INSTRUMENT:
		case ACT_EXPAND_PATTERN:
		case ACT_SHRINK_PATTERN:
			if (!pPatternEditor->IsSelecting())
				return false;
			SaveEntire(pPatternEditor);
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
		case ACT_EDIT_DELETE_ROWS:
		case ACT_INSERT_SEL_ROWS:
		case ACT_TRANSPOSE:
		case ACT_SCROLL_VALUES:
		case ACT_INTERPOLATE:
		case ACT_REVERSE:
		case ACT_REPLACE_INSTRUMENT:
		case ACT_EXPAND_PATTERN:
		case ACT_SHRINK_PATTERN:
			RestoreSelection(pPatternEditor);
			RestoreEntire(pPatternEditor);
			break;
		case ACT_DRAG_AND_DROP:
			RestoreEntire(pPatternEditor);
			RestoreSelection(pPatternEditor);
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
			RestoreSelection(pPatternEditor);
			Transpose(pDoc);
			break;
		case ACT_SCROLL_VALUES:
			pPatternEditor->SetSelection(m_selection);
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
		case ACT_EXPAND_PATTERN:
			m_iStretchMap.resize(2);
			m_iStretchMap[0] = 1;
			m_iStretchMap[1] = 0;
			StretchPattern(pDoc);
			break;
		case ACT_SHRINK_PATTERN:
			m_iStretchMap.resize(1);
			m_iStretchMap[0] = 2;
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
