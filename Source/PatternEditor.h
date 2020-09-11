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


// CPatternEditor, the pattern editor class

#include "Common.h"
#include "PatternEditorTypes.h"


// Row color cache
struct RowColorInfo_t {
	COLORREF Note;
	COLORREF Instrument;
	COLORREF Volume;
	COLORREF Effect;
	COLORREF Back;
	COLORREF Shaded;
	COLORREF Compact;		// // //
};

extern void CopyNoteSection(stChanNote *Target, const stChanNote *Source, paste_mode_t Mode, column_t Begin, column_t End);		// // //

// External classes
class CFamiTrackerDoc;
class CFamiTrackerView;
class CMainFrame;

// CPatternEditor
class CPatternEditor {
	// Public methods
public:
	CPatternEditor();
	~CPatternEditor();

	void ApplyColorScheme();
	void SetDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView);
	void SetWindowSize(int width, int height);
	void ResetCursor();

	// Invalidation
	void InvalidatePatternData();
	void InvalidateCursor();
	void InvalidateBackground();
	void InvalidateHeader();

	// Drawing
	void DrawScreen(CDC *pDC, CFamiTrackerView *pView);	// Draw pattern area
	void DrawMeters(CDC *pDC);							// Draw channel meters
	void CreateBackground(CDC *pDC);					// Create off-screen buffers

	bool CursorUpdated();								// Update cursor state, returns true if erase is needed
	void UpdatePatternLength();							// Read pattern length
	void SetFocus(bool bFocus);

	CRect GetActiveRect() const;
	CRect GetHeaderRect() const;
	CRect GetPatternRect() const;
	CRect GetUnbufferedRect() const;
	CRect GetInvalidatedRect() const;

	// DPCM state
	void SetDPCMState(const stDPCMState &State);

	// Cursor movement
	void MoveDown(int Step);
	void MoveUp(int Step);
	void MoveLeft();
	void MoveRight();
	void MoveToTop();
	void MoveToBottom();
	void NextChannel();
	void PreviousChannel();
	void FirstChannel();
	void LastChannel();
	void MoveChannelLeft();
	void MoveChannelRight();
	void OnHomeKey();
	void OnEndKey();

	void MoveCursor(const CCursorPos &Pos);		// // // primitive cursor setter
	void MoveToRow(int Row);
	void MoveToFrame(int Frame);
	void MoveToChannel(int Channel);
	void MoveToColumn(cursor_column_t Column);
	void NextFrame();
	void PreviousFrame();

	void ScrollLeft();
	void ScrollRight();
	void ScrollNextChannel();
	void ScrollPreviousChannel();

	// Cursor state
	int GetFrame() const;
	int GetChannel() const;
	int GetRow() const;
	cursor_column_t GetColumn() const;
	CCursorPos GetCursor() const;		// // //
	
	cursor_column_t GetChannelColumns(int Channel) const;		// // //

	// Mouse
	void OnMouseDown(const CPoint &point);
	void OnMouseUp(const CPoint &point);
	bool OnMouseHover(UINT nFlags, const CPoint &point);
	bool OnMouseNcMove();
	void OnMouseMove(UINT nFlags, const CPoint &point);
	void OnMouseDblClk(const CPoint &point);
	void OnMouseScroll(int Delta);
	void OnMouseRDown(const CPoint &point);

	bool CancelDragging();
	void CancelSelection();

	bool IsOverHeader(const CPoint &point) const;
	bool IsOverPattern(const CPoint &point) const;
	bool IsInsidePattern(const CPoint &point) const;
	bool IsInsideRowColumn(const CPoint &point) const;
	int GetChannelAtPoint(int PointX) const;

	// Edit: Copy & paste, selection
	CPatternClipData *CopyEntire() const;
	CPatternClipData *Copy() const;
	CPatternClipData *CopyRaw() const;		// // //
	CPatternClipData *CopyRaw(const CSelection &Sel) const;		// // //
	void Cut();
	void PasteEntire(const CPatternClipData *pClipData);
	void Paste(const CPatternClipData *pClipData, const paste_mode_t PasteMode, const paste_pos_t PastePos);		// // //
	void PasteRaw(const CPatternClipData *pClipData);		// // //
	void PasteRaw(const CPatternClipData *pClipData, const CCursorPos &Pos);		// // //

	bool IsSelecting() const;
	void SelectChannel();
	void SelectAllChannels();
	void SelectAll();

	void GetVolumeColumn(CString &str) const;
	void GetSelectionAsText(CString &str) const;		// // //
	void GetSelectionAsPPMCK(CString &str) const;		// // //

	// Various
	int GetCurrentPatternLength(int Frame) const;		// // // allow negative frames
	bool IsInRange(const CSelection &sel, int Frame, int Row, int Channel, cursor_column_t Column) const;		// // //

	// Settings
	void SetHighlight(const stHighlight Hl);		// // //
	void SetFollowMove(bool bEnable);
	void SetCompactMode(bool bEnable);		// // //

	bool IsPlayCursorVisible() const;

	// Scrolling
	void AutoScroll(const CPoint &point, UINT nFlags);
	bool ScrollTimerCallback();
	void OnVScroll(UINT nSBCode, UINT nPos);
	void OnHScroll(UINT nSBCode, UINT nPos);

	// Selection
	void SetBlockStart();
	void SetBlockEnd();
	CSelection GetSelection() const;
	void SetSelection(const CSelection &selection);
	void SetSelection(int Scope);		// // //

	int GetSelectionSize() const;		// // //
	sel_condition_t GetSelectionCondition() const;		// // //
	sel_condition_t GetSelectionCondition(const CSelection &Sel) const;		// // //
	void UpdateSelectionCondition();		// // //

	void DragPaste(const CPatternClipData *pClipData, const CSelection *pDragTarget, bool bMix);

	// OLE support
	void BeginDrag(const CPatternClipData *pClipData);
	void EndDrag();
	bool PerformDrop(const CPatternClipData *pClipData, bool bCopy, bool bCopyMix);
	void UpdateDrag(const CPoint &point);

#ifdef _DEBUG
	void DrawLog(CDC *pDC);
#endif

	// Private methods
private:

	// Layout
	bool CalculatePatternLayout();
	void CalcLayout();
	// // //
	unsigned int GetColumnWidth(cursor_column_t Column) const;		// // //
	unsigned int GetColumnSpace(cursor_column_t Column) const;
	unsigned int GetSelectWidth(cursor_column_t Column) const;
	unsigned int GetChannelWidth(int EffColumns) const;

	// Main draw methods
	void PerformFullRedraw(CDC *pDC);
	void PerformQuickRedraw(CDC *pDC);
	void DrawUnbufferedArea(CDC *pDC);
	void DrawHeader(CDC *pDC);

	// Helper draw methods
	void MovePatternArea(CDC *pDC, int FromRow, int ToRow, int NumRows) const;
	void ScrollPatternArea(CDC *pDC, int Rows) const;
	void ClearRow(CDC *pDC, int Line) const;
	void PrintRow(CDC *pDC, int Row, int Line, int Frame) const;
	void DrawRow(CDC *pDC, int Row, int Line, int Frame, bool bPreview) const;
	// // //
	void DrawCell(CDC *pDC, int PosX, cursor_column_t Column, int Channel, bool bInvert, stChanNote *pNoteData, RowColorInfo_t *pColorInfo) const;
	void DrawChar(CDC *pDC, int x, int y, TCHAR c, COLORREF Color) const;

	// Other drawing
	void DrawChannelStates(CDC *pDC);
	void DrawRegisters(CDC *pDC);

	// Scrolling
	void UpdateVerticalScroll();
	void UpdateHorizontalScroll();

	// Translation
	cursor_column_t GetColumnAtPoint(int PointX) const;		// // //
	int  GetSelectedTrack() const;
	int  GetFrameCount() const;		// // //
	int	 GetChannelCount() const;
	int	 RowToLine(int Row) const;

	CCursorPos GetCursorAtPoint(const CPoint &point) const;

	// Selection methods
	class CSelectionGuard		// // //
	{
	public:
		CSelectionGuard(CPatternEditor *pEditor);
		~CSelectionGuard();
	private:
		CPatternEditor *m_pPatternEditor;
	};

	void SetSelectionStart(const CCursorPos &start);
	void SetSelectionEnd(const CCursorPos &end);

	void BeginMouseSelection(const CPoint &point);
	void ContinueMouseSelection(const CPoint &point);

	std::pair<CPatternIterator, CPatternIterator> GetIterators() const;		// // //

	// Editing
	void IncreaseEffectColumn(int Channel);
	void DecreaseEffectColumn(int Channel);

	// Keys
	static bool IsShiftPressed();		// // // static
	static bool IsControlPressed();

	// Mouse
	void OnMouseDownHeader(const CPoint &point);
	void OnMouseDownPattern(const CPoint &point);

	// Main frame
	CMainFrame *GetMainFrame() const;	

public:
	// Public consts
	static const int HEADER_HEIGHT_NODPI;
	const int HEADER_HEIGHT;

	// Variables
public:
	CFamiTrackerDoc	 *m_pDocument;
private:
	CFamiTrackerView *m_pView;

	// GDI objects
	CDC		*m_pPatternDC;
	CDC		*m_pHeaderDC;
	CDC		*m_pRegisterDC;		// // //
	CBitmap *m_pPatternBmp;
	CBitmap	*m_pHeaderBmp;
	CBitmap	*m_pRegisterBmp;		// // //
	CFont	m_fontHeader;
	CFont	m_fontPattern;
	CFont	m_fontCourierNew;

	// Window
	int		m_iWinWidth;					// Window height & width
	int		m_iWinHeight;

	// Flags
	bool	m_bPatternInvalidated;
	bool	m_bCursorInvalidated;
	bool	m_bBackgroundInvalidated;
	bool	m_bHeaderInvalidated;
	bool	m_bSelectionInvalidated;

	// Draw state variables
	int		m_iCenterRow;					// The row in the middle of the editor, will always point to a valid row in current frame
	
	int		m_iPatternLength;				// Pattern length of selected frame
	// // // gone

	// Previous draw state
	int		m_iLastCenterRow;				// Previous center row
	int		m_iLastFrame;					// Previous frame
	int		m_iLastFirstChannel;			// Previous first visible channel
	int		m_iLastPlayRow;					// Previous play row

	// Play cursor
	int		m_iPlayRow;
	int		m_iPlayFrame;

//	CPatternEditorLayout m_Layout;

	// Pattern layout
	int		m_iPatternWidth;				// Width of channels in pattern area
	int		m_iPatternHeight;				// Height of channels in pattern area
	int		m_iLinesVisible;				// Number of lines visible on screen (may include one incomplete line)
	int		m_iLinesFullVisible;			// Number of lines full visible on screen
	int		m_iChannelsVisible;				// Number of channels visible on screen (may include one incomplete channel)
	int		m_iChannelsFullVisible;			// Number of channels full visible on screen
	int		m_iFirstChannel;				// First drawn channel
	int		m_iRowHeight;					// Height of each row in pixels
	int		m_iPatternFontSize;				// Size of pattern font
	int		m_iCharWidth;					// // // no longer static const
	int		m_iColumnSpacing;				// // //
	int		m_iRowColumnWidth;				// // //

	int		m_iChannelWidths[MAX_CHANNELS];	// Cached width in pixels of each channel
	int		m_iChannelOffsets[MAX_CHANNELS];// Cached x position of channels
	cursor_column_t	m_iColumns[MAX_CHANNELS]; // // // Cached *index of rightmost column* in each channel

	// Drawing (TODO remove these)
	int		m_iDrawCursorRow;
	int		m_iDrawFrame;

	// Settings
	bool	m_bFollowMode;					// Follow mode enable/disable
	bool	m_bCompactMode;					// // // display notes only
	bool	m_bHasFocus;					// Pattern editor has focus
	stHighlight m_vHighlight;				// // // Pattern highlight settings

	// Colors
	COLORREF m_colEmptyBg;
	COLORREF m_colSeparator;
	COLORREF m_colHead1;
	COLORREF m_colHead2;
	COLORREF m_colHead3;
	COLORREF m_colHead4;
	COLORREF m_colHead5;		// // //

	// Meters and DPCM
	stDPCMState m_DPCMState;

	int		m_iMouseHoverChan;
	int		m_iMouseHoverEffArrow;

	// Cursor position
	CCursorPos m_cpCursorPos;

	// Selection
	bool	m_bSelecting;			// Selection is active
	bool	m_bCurrentlySelecting;	// TODO remove this
	bool	m_bDragStart;			// Indicates that drag & drop is being initiated
	bool	m_bDragging;			// Drag & drop is active
	bool	m_bFullRowSelect;		// Enable full row selection
	int		m_iWarpCount;			// // //
	int		m_iDragBeginWarp;		// // //
	sel_condition_t m_iSelectionCondition;		// // //

	// Mouse
	bool	m_bMouseActive;			// Indicates that mouse activity is in progess by the user
	int		m_iChannelPushed;		// Pushed channel state
	bool	m_bChannelPushed;

	CSelection m_selection;
	// // // gone

	// Drag
	CSelection m_selDrag;
	CCursorPos m_cpDragPoint;

	CPoint m_ptSelStartPoint;

	// Numbers of pixels until selection is initiated
	int		m_iDragThresholdX;
	int		m_iDragThresholdY;

	// OLE support
	int		m_iDragChannels;
	int		m_iDragRows;
	cursor_column_t m_iDragStartCol;		// // //
	cursor_column_t m_iDragEndCol;		// // //

	int		m_iDragOffsetChannel;
	int		m_iDragOffsetRow;

	// Scrolling
	CPoint	m_ptScrollMousePos;
	UINT	m_nScrollFlags;
	unsigned int m_iScrolling;		// // //
	int		m_iCurrentHScrollPos;

	// Benchmarking
	mutable int m_iRedraws;
	mutable int m_iFullRedraws;
	mutable int m_iQuickRedraws;
	mutable int m_iHeaderRedraws;
	mutable int m_iPaints;
	mutable int m_iErases;
	mutable int m_iBuffers;
	mutable int m_iCharsDrawn;
};
