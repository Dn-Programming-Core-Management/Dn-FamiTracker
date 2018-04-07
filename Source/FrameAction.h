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

#include "Action.h"
#include "FrameEditorTypes.h"		// // //
#include "IntRange.h"		// // //

class CFrameEditor;		// // //
class CFamiTrackerView;
class CMainFrame;
class CFamiTrackerDoc;

/*
	\brief A structure responsible for recording the cursor and selection state of the frame
	editor for use by frame actions. Uses CFamiTrackerView since the frame editor class is not
	independent.
*/
struct CFrameEditorState		// TODO maybe merge this with CPatternEditorState
{
	/*!	\brief Constructor of the frame editor state.
		\details On construction, the object retrieves the current state of the frame editor
		immediately. Once created, a state object remains constant and can be applied back to the
		frame editor as many times as desired.
		\param pView Pointer to the tracker view.
		\param Track The track number. */
	CFrameEditorState(const CFamiTrackerView *pEditor, int Track);

	/*!	\brief Applies the state to a frame editor.
		\param pView Pointer to the tracker view. */
	void ApplyState(CFamiTrackerView *pView) const;
	
	/*!	\brief Obtains the first selected frame.
		\return Starting frame index. */
	int GetFrameStart() const;
	/*!	\brief Obtains the last selected frame.
		\return Ending channel index. */
	int GetFrameEnd() const;
	/*!	\brief Obtains the first selected channel.
		\return Starting frame index. */
	int GetChanStart() const;
	/*!	\brief Obtains the last selected channel.
		\return Ending channel index. */
	int GetChanEnd() const;

	/*!	\brief The current track number at the time of the state's creation. */
	int Track;

	/*!	\brief The current cursor position at the time of the state's creation. */
	CFrameCursorPos Cursor;

	/*!	\brief The current selection position at the time of the state's creation. */
	CFrameSelection Selection;

	/*!	\brief Whether a selection is active at the time of the state's creation. */
	bool IsSelecting;

private:
	CFrameSelection OriginalSelection;
};

// Frame commands
class CFrameAction : public CAction
{
protected:
	CFrameAction();		// // //

public:
	virtual ~CFrameAction();

	void SaveUndoState(const CMainFrame *pMainFrm);		// // //
	void SaveRedoState(const CMainFrame *pMainFrm);		// // //
	void RestoreUndoState(CMainFrame *pMainFrm) const;		// // //
	void RestoreRedoState(CMainFrame *pMainFrm) const;		// // //

protected:
	static int ClipPattern(int Pattern);

	CIntRange<int> m_itFrames, m_itChannels;

protected:
	CFrameEditorState *m_pUndoState = nullptr, *m_pRedoState = nullptr;		// // //
};

// // // built-in frame action subtypes

class CFActionAddFrame : public CFrameAction
{
public:
	CFActionAddFrame() { }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
};

class CFActionRemoveFrame : public CFrameAction
{
public:
	CFActionRemoveFrame() { }
	~CFActionRemoveFrame() { SAFE_RELEASE(m_pRowClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
private:
	CFrameClipData *m_pRowClipData = nullptr;
};

class CFActionDuplicateFrame : public CFrameAction
{
public:
	CFActionDuplicateFrame() { }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
};

class CFActionCloneFrame : public CFrameAction
{
public:
	CFActionCloneFrame() { }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
};

class CFActionFrameCount : public CFrameAction
{
public:
	CFActionFrameCount(int Count) : m_iNewFrameCount(Count) { }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
	bool Merge(const CAction *Other) override;		// // //
private:
	int m_iOldFrameCount, m_iNewFrameCount;
};

class CFActionSetPattern : public CFrameAction
{
public:
	CFActionSetPattern(int Pattern) : m_iNewPattern(Pattern) { }
	~CFActionSetPattern() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
	bool Merge(const CAction *Other) override;		// // //
private:
	int m_iNewPattern;
	CFrameClipData *m_pClipData = nullptr;
};

class CFActionSetPatternAll : public CFrameAction
{
public:
	CFActionSetPatternAll(int Pattern) : m_iNewPattern(Pattern) { }
	~CFActionSetPatternAll() { SAFE_RELEASE(m_pRowClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
	bool Merge(const CAction *Other) override;		// // //
private:
	int m_iNewPattern;
	CFrameClipData *m_pRowClipData = nullptr;
};

class CFActionChangePattern : public CFrameAction
{
public:
	CFActionChangePattern(int Offset) : m_iPatternOffset(Offset) { }
	~CFActionChangePattern() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
	bool Merge(const CAction *Other) override;		// // //
private:
	int m_iPatternOffset;
	CFrameClipData *m_pClipData = nullptr;
	mutable bool m_bOverflow = false;
};

class CFActionChangePatternAll : public CFrameAction
{
public:
	CFActionChangePatternAll(int Offset) : m_iPatternOffset(Offset) { }
	~CFActionChangePatternAll() { SAFE_RELEASE(m_pRowClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
	bool Merge(const CAction *Other) override;		// // //
private:
	int m_iPatternOffset;
	CFrameClipData *m_pRowClipData = nullptr;
	mutable bool m_bOverflow = false;
};

class CFActionMoveDown : public CFrameAction
{
public:
	CFActionMoveDown() { }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
};

class CFActionMoveUp : public CFrameAction
{
public:
	CFActionMoveUp() { }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
};

class CFActionClonePatterns : public CFrameAction		// // //
{
public:
	CFActionClonePatterns() { }
	~CFActionClonePatterns() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
private:
	int m_iOldPattern, m_iNewPattern;
	CFrameClipData *m_pClipData = nullptr;
};

class CFActionPaste : public CFrameAction
{
public:
	CFActionPaste(CFrameClipData *pData, int Frame, bool Clone) : m_pClipData(pData), m_iTargetFrame(Frame), m_bClone(Clone) { }
	~CFActionPaste() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
private:
	CFrameClipData *m_pClipData = nullptr;
	int m_iTargetFrame;
	bool m_bClone;
};

class CFActionPasteOverwrite : public CFrameAction
{
public:
	CFActionPasteOverwrite(CFrameClipData *pData) : m_pClipData(pData) { }
	~CFActionPasteOverwrite() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
private:
	CFrameClipData *m_pClipData = nullptr, *m_pOldClipData = nullptr;
	CFrameSelection m_TargetSelection;
};

class CFActionDropMove : public CFrameAction
{
public:
	CFActionDropMove(CFrameClipData *pData, int Frame) : m_pClipData(pData), m_iTargetFrame(Frame) { }
	~CFActionDropMove() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
private:
	CFrameClipData *m_pClipData = nullptr;
	int m_iTargetFrame;
};

class CFActionDeleteSel : public CFrameAction
{
public:
	CFActionDeleteSel() { }
	~CFActionDeleteSel() { SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
private:
	CFrameClipData *m_pClipData = nullptr;
};

class CFActionMergeDuplicated : public CFrameAction
{
public:
	CFActionMergeDuplicated() { }
	~CFActionMergeDuplicated() { SAFE_RELEASE(m_pOldClipData); SAFE_RELEASE(m_pClipData); }
private:
	bool SaveState(const CMainFrame *pMainFrm) override;
	void Undo(CMainFrame *pMainFrm) const override;
	void Redo(CMainFrame *pMainFrm) const override;
private:
	CFrameClipData *m_pClipData = nullptr, *m_pOldClipData = nullptr;
};
