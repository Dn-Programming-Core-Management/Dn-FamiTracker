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

#include "FrameEditorTypes.h"		// // //

class CFamiTrackerDoc;
class CFamiTrackerView;
class CFrameEditor;

class CFrameEditorDropTarget : public COleDropTarget
{
public:
	CFrameEditorDropTarget(CFrameEditor *pParent) 
		: m_iClipboard(0), m_nDropEffect(DROPEFFECT_NONE), m_bCopyNewPatterns(false), m_pParent(pParent) {};
	void SetClipBoardFormat(UINT iClipboard);
	DROPEFFECT OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	DROPEFFECT OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);
	BOOL OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	void OnDragLeave(CWnd* pWnd);
	
	bool IsDragging() const;
	bool CopyToNewPatterns() const;

private:
	UINT m_iClipboard;
	int m_nDropEffect;
	bool m_bCopyNewPatterns;
	CFrameEditor *m_pParent;
};

// CFrameEditor

class CFrameEditor : public CWnd
{
	DECLARE_DYNAMIC(CFrameEditor)
public:
	CFrameEditor(CMainFrame *pMainFrm);
	virtual ~CFrameEditor();

	// General
	void AssignDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView);

	// Drawing
	void RedrawFrameEditor();
	void InvalidateFrameData();
	void SetupColors();

	// Input
	bool Translate(HWND hWnd, MSG *pMsg) const;
	void EnableInput();
	bool InputEnabled() const;

	// Selection, copy & paste, drag & drop
	CFrameSelection GetSelection() const;		// // //
	bool IsSelecting() const;		// // //
	void SetSelection(const CFrameSelection &Sel);		// // //
	void CancelSelection();

	bool IsClipboardAvailable() const;
	bool IsCopyValid(COleDataObject* pDataObject) const;
	void UpdateDrag(const CPoint &point);
	BOOL DropData(COleDataObject* pDataObject, DROPEFFECT dropEffect);
	void PerformDragOperation(unsigned int Track, CFrameClipData *pClipData, int DragTarget, bool bDelete, bool bNewPatterns);

	// Commands
	CFrameClipData *Copy() const;		// // //
	CFrameClipData *Copy(const CFrameSelection &Sel) const;		// // //
	CFrameClipData *CopyFrame(int Frame) const;		// // //
	void Paste(unsigned int Track, const CFrameClipData *pClipData);
	void PasteAt(unsigned int Track, const CFrameClipData *pClipData, const CFrameCursorPos &Pos);		// // //
	void PasteNew(unsigned int Track, const CFrameClipData *pClipData);

	// Return window width in pixels
	unsigned int CalcWidth(int Channels) const;

private:
	// Drawing
	void DrawFrameEditor(CDC *pDC);
	bool NeedUpdate() const;

	// Translation
	int GetRowFromPoint(const CPoint &point, bool DropTarget) const;
	int GetChannelFromPoint(const CPoint &point) const;
	bool IsOverFrameColumn(const CPoint &point) const;		// // //

	// Drag & drop
	void InitiateDrag();

	std::pair<CFrameIterator, CFrameIterator> GetIterators() const;		// // //

	void AutoScroll(const CPoint &point);

public:
	// Window layout
	static const int ROW_COLUMN_WIDTH	= 26;	// The left-most column width
	static const int FRAME_ITEM_WIDTH	= 20;	// Pattern item width 
	static const int ROW_HEIGHT			= 15;	// Height of rows
	static const int TOP_OFFSET			= 3;	// Top offset
	static const int DEFAULT_HEIGHT		= 161;	// Window height at top position
	static const int CURSOR_WIDTH		= 8;	// Cursor box width

	static const TCHAR CLIPBOARD_ID[];

private:
	// Object pointers
	CMainFrame		 *m_pMainFrame;
	CFamiTrackerDoc  *m_pDocument;
	CFamiTrackerView *m_pView;

	// GDI objects
	CFont	m_Font;
	CBitmap m_bmpBack;
	CDC		m_dcBack;

	UINT	m_iClipboard;

	// Window size
	int		m_iWinWidth;
	int		m_iWinHeight;

	// Cursor
	int		m_iHiglightLine;
	int		m_iFirstChannel;
	int		m_iCursorPos;
	int		m_iNewPattern;
	int		m_iRowsVisible;
	int		m_iMiddleRow;
	bool	m_bInputEnable;
	bool	m_bCursor;

	// Draw info
	bool	m_bInvalidated;
	int		m_iLastCursorFrame;
	int		m_iLastCursorChannel;
	int		m_iLastPlayFrame;

	// Accelerator table
	HACCEL	m_hAccel;

	// Select/drag
	bool	m_bSelecting;
	bool	m_bStartDrag;
	bool	m_bDeletedRows;
	bool	m_bFullFrameSelect = false;		// // //
	CFrameSelection m_selection;		// // //
	int		m_iDragRow;

	int		m_iTopScrollArea;
	int		m_iBottomScrollArea;

	CPoint	m_ButtonPoint;

	CFrameEditorDropTarget m_DropTarget;

	// Numbers of pixels until selection is initiated
	int		m_iDragThresholdX;
	int		m_iDragThresholdY;

	// Benchmark
	mutable int m_iUpdates;
	mutable int m_iPaints;

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnEditCut();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditPasteNewPatterns();
	afx_msg void OnEditDelete();
	afx_msg void OnModuleInsertFrame();
	afx_msg void OnModuleRemoveFrame();
	afx_msg void OnModuleDuplicateFrame();
	afx_msg void OnModuleDuplicateFramePatterns();
	afx_msg void OnModuleMoveFrameDown();
	afx_msg void OnModuleMoveFrameUp();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	// // //
	afx_msg void OnModuleDuplicateCurrentPattern();
};


