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
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#pragma once

#include <vector>		// // //
#include "Action.h"
#include "PatternEditorTypes.h"

enum transpose_t {
	TRANSPOSE_DEC_NOTES,
	TRANSPOSE_INC_NOTES,
	TRANSPOSE_DEC_OCTAVES,
	TRANSPOSE_INC_OCTAVES
};

/*
	\brief A structure responsible for recording the cursor and selection state of the pattern
	editor for use by pattern actions.
*/
struct CPatternEditorState		// // // TODO: might be moved to PatternEditor.h
{
	/*!	\brief Constructor of the pattern editor state.
		\details On construction, the object retrieves the current state of the pattern editor
		immediately. Once created, a state object remains constant and can be applied back to the
		pattern editor as many times as desired.
		\param pEditor Pointer to the pattern editor.
		\param Track The track number. */
	CPatternEditorState(const CPatternEditor *pEditor, int Track);

	/*!	\brief Applies the state to a pattern editor.
		\param pEditor Pointer to the pattern editor. */
	void ApplyState(CPatternEditor *pEditor) const;

	/*!	\brief The current track number at the time of the state's creation. */
	const int Track;

	/*!	\brief The current cursor position at the time of the state's creation. */
	const CCursorPos Cursor;

	/*!	\brief The current selection position at the time of the state's creation. */
	const CSelection Selection;

	/*!	\brief Whether a selection is active at the time of the state's creation. */
	const bool IsSelecting;
};

// Pattern commands
class CPatternAction : public CAction
{
public:
	enum ACTIONS
	{
		ACT_EDIT_NOTE,
		ACT_REPLACE_NOTE,		// // //
		ACT_INSERT_ROW,
		ACT_DELETE_ROW,
		ACT_INCREASE,
		ACT_DECREASE,
		ACT_EDIT_PASTE,		// // //
		ACT_EDIT_DELETE,
		ACT_EDIT_DELETE_ROWS,
		ACT_INSERT_SEL_ROWS,
		ACT_TRANSPOSE,
		ACT_SCROLL_VALUES,
		ACT_INTERPOLATE,
		ACT_REVERSE,
		ACT_REPLACE_INSTRUMENT,
		ACT_DRAG_AND_DROP,
		ACT_PATTERN_LENGTH,
		ACT_STRETCH_PATTERN,		// // //
		ACT_EFFECT_COLUMNS,		// // //
	};

// protected:
public:
	CPatternAction(int iAction);

public:
	virtual ~CPatternAction();

	virtual bool SaveState(const CMainFrame *pMainFrm);
	virtual void Undo(CMainFrame *pMainFrm) const;
	virtual void Redo(CMainFrame *pMainFrm) const;

	void SaveUndoState(const CMainFrame *pMainFrm);		// // //
	void SaveRedoState(const CMainFrame *pMainFrm);		// // //
	void RestoreUndoState(CMainFrame *pMainFrm) const;		// // //
	void RestoreRedoState(CMainFrame *pMainFrm) const;		// // //

public:
	void SetPaste(CPatternClipData *pClipData);
	void SetPasteMode(paste_mode_t Mode);		// // //
	void SetPastePos(paste_pos_t Pos);		// // //
	void SetTranspose(transpose_t Mode);
	void SetScroll(int Scroll);
	void SetDragAndDrop(const CPatternClipData *pClipData, bool bDelete, bool bMix, const CSelection *pDragTarget);
	void SetPatternLength(int Length);
	void Update(CMainFrame *pMainFrm);
	void SetStretchMap(const std::vector<int> Map);		// // //

private:
	void SaveEntire(const CPatternEditor *pPatternEditor);
	void RestoreEntire(CPatternEditor *pPatternEditor) const;
	bool SetTargetSelection(CPatternEditor *pPatternEditor);		// // //
	void CopySelection(const CPatternEditor *pPatternEditor);		// // //
	void PasteSelection(CPatternEditor *pPatternEditor) const;		// // //
	void CopyAuxiliary(const CPatternEditor *pPatternEditor);		// // //
	void PasteAuxiliary(CPatternEditor *pPatternEditor) const;		// // //

	void RestoreSelection(CPatternEditor *pPatternEditor) const;

	void InsertRows(CFamiTrackerDoc *pDoc) const;
	void PullUpRows(CFamiTrackerDoc *pDoc) const;
	void StretchPattern(CFamiTrackerDoc *pDoc) const;		// // //
	void Transpose(CFamiTrackerDoc *pDoc) const;
	void Interpolate(CFamiTrackerDoc *pDoc) const;
	void Reverse(CFamiTrackerDoc *pDoc) const;
	void ScrollValues(CFamiTrackerDoc *pDoc) const;
	void DeleteSelection(CFamiTrackerDoc *pDoc) const;

protected:
	CPatternIterator GetStartIterator() const;		// // //
	CPatternIterator GetEndIterator() const;
	virtual void UpdateView(CFamiTrackerDoc *pDoc) const;		// // //

protected:
	CPatternEditorState *m_pUndoState;		// // //
	CPatternEditorState *m_pRedoState;

private:
	int m_iNewPatternLen;
	int m_iOldPatternLen;

	const CPatternClipData *m_pClipData;
	CPatternClipData *m_pUndoClipData, *m_pAuxiliaryClipData;		// // //
	paste_mode_t m_iPasteMode;		// // //
	paste_pos_t m_iPastePos;		// // //
	
	bool m_bSelecting;
	CSelection m_selection, m_newSelection;		// // //
	int m_iSelectionSize;		// // //

	transpose_t m_iTransposeMode;
	int m_iScrollValue;

	bool m_bDragDelete;
	bool m_bDragMix;
	CSelection m_dragTarget;

	std::vector<int> m_iStretchMap;		// // //
};

// // // built-in pattern action subtypes

class CPActionEditNote : public CPatternAction
{
public:
	CPActionEditNote(const stChanNote &Note);
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	stChanNote m_NewNote, m_OldNote;
};

class CPActionReplaceNote : public CPatternAction
{
public:
	CPActionReplaceNote(const stChanNote &Note, int Frame, int Row, int Channel);
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	stChanNote m_NewNote, m_OldNote;
	int m_iFrame, m_iRow, m_iChannel;
};

class CPActionInsertRow : public CPatternAction
{
public:
	CPActionInsertRow();
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	stChanNote m_OldNote;
};

class CPActionDeleteRow : public CPatternAction
{
public:
	CPActionDeleteRow(bool PullUp, bool Backspace);
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	stChanNote m_OldNote;
	bool m_bPullUp, m_bBack;
};

class CPActionScrollField : public CPatternAction		// // //
{
public:
	CPActionScrollField(int Amount);
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	stChanNote m_OldNote;
	int m_iAmount;
};

class CPActionReplaceInst : public CPatternAction
{
public:
	CPActionReplaceInst(unsigned Index);
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
private:
	unsigned m_iInstrumentIndex;
	CPatternClipData *m_pUndoClipData;
};

class CPActionEffColumn : public CPatternAction
{
public:
	CPActionEffColumn(int Channel, int Count);
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
	void UpdateView(CFamiTrackerDoc *pDoc) const;
private:
	unsigned m_iChannel;
	unsigned m_iOldColumns, m_iNewColumns;
};
