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

class CFrameEditor;		// // //

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

	/*!	\brief The current track number at the time of the state's creation. */
	int Track;

	/*!	\brief The current frame position at the time of the state's creation. */
	int Frame;

	/*!	\brief The current channel position at the time of the state's creation. */
	int Channel;

	stSelectInfo Selection;
};

// Frame commands
class CFrameAction : public CAction
{
public:
	enum ACTIONS
	{
		ACT_ADD,
		ACT_REMOVE,
		ACT_DUPLICATE,
		ACT_DUPLICATE_PATTERNS,
		ACT_CHANGE_COUNT,
		ACT_SET_PATTERN,
		ACT_SET_PATTERN_ALL,
		ACT_CHANGE_PATTERN,
		ACT_CHANGE_PATTERN_ALL,
		ACT_MOVE_DOWN,
		ACT_MOVE_UP,
		ACT_PASTE,
		ACT_PASTE_NEW,
		ACT_DRAG_AND_DROP_MOVE,
		ACT_DRAG_AND_DROP_COPY,
		ACT_DRAG_AND_DROP_COPY_NEW,
		ACT_DELETE_SELECTION,
		ACT_MERGE_DUPLICATED_PATTERNS,
		ACT_DUPLICATE_CURRENT		// // //
	};

public:
	CFrameAction(int iAction);
	virtual ~CFrameAction();

	virtual bool SaveState(const CMainFrame *pMainFrm);
	virtual void Undo(CMainFrame *pMainFrm) const;
	virtual void Redo(CMainFrame *pMainFrm) const;

	void SaveUndoState(const CMainFrame *pMainFrm);		// // //
	void SaveRedoState(const CMainFrame *pMainFrm);		// // //
	void RestoreUndoState(CMainFrame *pMainFrm) const;		// // //
	void RestoreRedoState(CMainFrame *pMainFrm) const;		// // //

public:
	void SetFrameCount(unsigned int FrameCount);
	void SetPattern(unsigned int Pattern);
	void SetPatternDelta(int Delta);		// // //
	void Update(CMainFrame *pMainFrm);
	void SetPasteData(CFrameClipData *pClipData);
	void SetDragInfo(int DragTarget, CFrameClipData *pClipData, bool Remove);

private:
	void SaveFrame(CFamiTrackerDoc *pDoc);
	void RestoreFrame(CFamiTrackerDoc *pDoc) const;

	void SaveAllFrames(CFamiTrackerDoc *pDoc);
	void RestoreAllFrames(CFamiTrackerDoc *pDoc) const;

	int ClipPattern(int Pattern) const;

	void ClearPatterns(CFamiTrackerDoc *pDoc, int Target) const;

protected:
	CFrameEditorState *m_pUndoState, *m_pRedoState;		// // //

private:
	unsigned int m_iUndoTrack;
	unsigned int m_iUndoFramePos;
	unsigned int m_iUndoChannelPos;
	unsigned int m_iRedoFramePos;
	unsigned int m_iRedoChannelPos;

	unsigned int m_iNewFrameCount;
	unsigned int m_iUndoFrameCount;
	unsigned int m_iNewPattern;
	unsigned int m_iOldPattern;
	
	int m_iPatternDelta;

	unsigned int m_iPatterns[MAX_CHANNELS];

	bool m_bDragRemove;
	unsigned int m_iDragTarget;

	unsigned int *m_pAllPatterns;

	CFrameClipData *m_pClipData;

	stSelectInfo m_oSelInfo;
};

// // // built-in frame action subtypes

class CFActionAddFrame : public CFrameAction
{
public:
	CFActionAddFrame();
private:
	bool SaveState(const CMainFrame *pMainFrm);
	void Undo(CMainFrame *pMainFrm) const;
	void Redo(CMainFrame *pMainFrm) const;
};
