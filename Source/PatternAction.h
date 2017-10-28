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


#pragma once

#include "Action.h"
#include "PatternEditorTypes.h"
#include "SongData.h"		// // //
#include "PatternNote.h"		// // //
#include <vector>		// // //
#include <memory>		// // //

enum transpose_t {
	TRANSPOSE_DEC_NOTES,
	TRANSPOSE_INC_NOTES,
	TRANSPOSE_DEC_OCTAVES,
	TRANSPOSE_INC_OCTAVES
};

class CPatternEditor;		// // //
class CMainFrame;		// // //
class CPatternClipData;		// // //

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
	int Track;

	/*!	\brief The current cursor position at the time of the state's creation. */
	CCursorPos Cursor;

	/*!	\brief The current selection position at the time of the state's creation. */
	CSelection Selection;

	/*!	\brief Whether a selection is active at the time of the state's creation. */
	bool IsSelecting;

private:
	CSelection OriginalSelection;
};

// Pattern commands
class CPatternAction : public CAction
{
public:
	virtual ~CPatternAction();

	void SaveUndoState(const CMainFrame &MainFrm) override;		// // //
	void SaveRedoState(const CMainFrame &MainFrm) override;		// // //
	void RestoreUndoState(CMainFrame &MainFrm) const override;		// // //
	void RestoreRedoState(CMainFrame &MainFrm) const override;		// // //

private:
	virtual void UpdateView(CFamiTrackerDoc *pDoc) const;		// // //

protected:
	bool SetTargetSelection(CPatternEditor *pPatternEditor, CSelection &Sel);		// // //
	void DeleteSelection(CFamiTrackerDoc &Doc, unsigned Track, const CSelection &Sel) const;		// // //
	bool ValidateSelection(const CPatternEditor &Editor) const;		// // //
	std::pair<CPatternIterator, CPatternIterator> GetIterators(CFamiTrackerDoc &doc) const;		// // //

protected:
	std::unique_ptr<CPatternEditorState> m_pUndoState;		// // //
	std::unique_ptr<CPatternEditorState> m_pRedoState;

protected:
	const CPatternClipData *m_pClipData = nullptr;
	CPatternClipData *m_pUndoClipData = nullptr;		// // //
	paste_mode_t m_iPasteMode;		// // //
	paste_pos_t m_iPastePos;		// // //
	
	bool m_bSelecting;
	CSelection m_selection, m_newSelection;		// // //

	CSelection m_dragTarget;
};

/*!
	\brief Specialization of the pattern action class for actions operating on a selection without
	modifying its span.
*/
class CPSelectionAction : public CPatternAction
{
protected:
	virtual ~CPSelectionAction();
protected:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
protected:
	CPatternClipData *m_pUndoClipData = nullptr;
};

// // // built-in pattern action subtypes

class CPActionEditNote : public CPatternAction
{
public:
	CPActionEditNote(const stChanNote &Note);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;

	stChanNote m_NewNote, m_OldNote;
};

class CPActionReplaceNote : public CPatternAction
{
public:
	CPActionReplaceNote(const stChanNote &Note, int Frame, int Row, int Channel);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;

	stChanNote m_NewNote, m_OldNote;
	int m_iFrame, m_iRow, m_iChannel;
};

class CPActionInsertRow : public CPatternAction
{
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;

	stChanNote m_OldNote;
};

class CPActionDeleteRow : public CPatternAction
{
public:
	CPActionDeleteRow(bool PullUp, bool Backspace);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;

	stChanNote m_OldNote;
	bool m_bPullUp, m_bBack;
};

class CPActionScrollField : public CPatternAction		// // //
{
public:
	CPActionScrollField(int Amount);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;

	stChanNote m_OldNote;
	int m_iAmount;
};

class CPActionPaste : public CPatternAction {
public:
	CPActionPaste(CPatternClipData *pClipData, paste_mode_t Mode, paste_pos_t Pos);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
};

class CPActionClearSel : public CPSelectionAction
{
	void Redo(CMainFrame &MainFrm) override;
};

class CPActionDeleteAtSel : public CPatternAction
{
public:
	virtual ~CPActionDeleteAtSel();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;

	CCursorPos m_cpTailPos;
	CPatternClipData *m_pUndoHead = nullptr;
	CPatternClipData *m_pUndoTail = nullptr;
};

class CPActionInsertAtSel : public CPatternAction
{
public:
	virtual ~CPActionInsertAtSel();
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;

	CCursorPos m_cpHeadPos, m_cpTailPos;
	CPatternClipData *m_pUndoHead = nullptr;
	CPatternClipData *m_pUndoTail = nullptr;
};

class CPActionTranspose : public CPSelectionAction
{
public:
	CPActionTranspose(transpose_t Type);
private:
	void Redo(CMainFrame &MainFrm) override;

	transpose_t m_iTransposeMode;
};

class CPActionScrollValues : public CPSelectionAction
{
public:
	CPActionScrollValues(int Amount);
private:
	void Redo(CMainFrame &MainFrm) override;

	int m_iAmount;
};

class CPActionInterpolate : public CPSelectionAction
{
	bool SaveState(const CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;

	int m_iSelectionSize;
};

class CPActionReverse : public CPSelectionAction
{
	bool SaveState(const CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
};

class CPActionReplaceInst : public CPSelectionAction
{
public:
	CPActionReplaceInst(unsigned Index);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
private:
	unsigned m_iInstrumentIndex;
};

class CPActionDragDrop : public CPatternAction
{
public:
	CPActionDragDrop(const CPatternClipData *pClipData, bool bDelete, bool bMix, const CSelection &pDragTarget);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
private:
//	const CPatternClipData *m_pClipData;
	std::unique_ptr<CPatternClipData> m_pAuxiliaryClipData; // TODO: remove
	bool m_bDragDelete;
	bool m_bDragMix;
//	CSelection m_newSelection;
//	CSelection m_dragTarget;
};

class CPActionPatternLen : public CPatternAction
{
public:
	CPActionPatternLen(int Length) : m_iNewPatternLen(Length) { }
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &Other) override;		// // //
private:
	int m_iOldPatternLen, m_iNewPatternLen;
};

class CPActionStretch : public CPSelectionAction
{
public:
	CPActionStretch(const std::vector<int> &Stretch);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;

	std::vector<int> m_iStretchMap;
};

class CPActionEffColumn : public CPatternAction
{
public:
	CPActionEffColumn(int Channel, int Count);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	void UpdateView(CFamiTrackerDoc *pDoc) const;

	unsigned m_iChannel;
	unsigned m_iOldColumns, m_iNewColumns;
};

class CPActionHighlight : public CPatternAction		// // //
{
public:
	CPActionHighlight(const stHighlight &Hl);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	void UpdateView(CFamiTrackerDoc *pDoc) const;

	stHighlight m_OldHighlight, m_NewHighlight;
};

class CPActionUniquePatterns : public CPatternAction {
public:
	CPActionUniquePatterns(unsigned index) : index_(index) { }
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	void UpdateView(CFamiTrackerDoc *pDoc) const;

	std::unique_ptr<CSongData> song_;
	std::unique_ptr<CSongData> songNew_;
	unsigned index_ = 0;
};
