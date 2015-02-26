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

#include <algorithm>
#include <cmath>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "FrameAction.h"
#include "Accelerator.h"
#include "PatternEditor.h"
#include "FrameEditor.h"
#include "SoundGen.h"
#include "Settings.h"
#include "Graphics.h"
#include "Clipboard.h"

/*
 * CFrameEditor
 * This is the frame(order) editor to the left in the control panel
 *
 */

const TCHAR CFrameEditor::DEFAULT_FONT[] = _T("System");
const TCHAR CFrameEditor::CLIPBOARD_ID[] = _T("FamiTracker Frames");

IMPLEMENT_DYNAMIC(CFrameEditor, CWnd)

CFrameEditor::CFrameEditor(CMainFrame *pMainFrm) :
	m_pMainFrame(pMainFrm),
	m_pDocument(NULL),
	m_pView(NULL),
	m_iClipboard(0),
	m_iWinWidth(0),
	m_iWinHeight(0),
	m_iHiglightLine(0),
	m_iFirstChannel(0),
	m_iCursorPos(0),
	m_iNewPattern(0),
	m_iRowsVisible(0),
	m_iMiddleRow(0),
	m_bInputEnable(false),
	m_bCursor(false),
	m_bInvalidated(false),
	m_iLastCursorFrame(0),
	m_iLastCursorChannel(0),
	m_iLastPlayFrame(0),
	m_hAccel(0),
	m_bSelecting(false),
	m_bStartDrag(false),
	m_bDeletedRows(false),
	m_iSelStartRow(0),
	m_iSelEndRow(0),
	m_iSelStartChan(0),
	m_iSelEndChan(0),
	m_iDragRow(0),
	m_iTopScrollArea(0),
	m_iBottomScrollArea(0),
	m_DropTarget(this),
	m_iDragThresholdX(0),
	m_iDragThresholdY(0),
	m_iUpdates(0),
	m_iPaints(0)
{
}

CFrameEditor::~CFrameEditor()
{
}

BEGIN_MESSAGE_MAP(CFrameEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_NCMOUSEMOVE()
	ON_WM_CONTEXTMENU()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_KEYDOWN()
	ON_WM_TIMER()
	ON_COMMAND(ID_FRAME_CUT, OnEditCut)
	ON_COMMAND(ID_FRAME_COPY, OnEditCopy)
	ON_COMMAND(ID_FRAME_PASTE, OnEditPaste)
	ON_COMMAND(ID_FRAME_PASTENEWPATTERNS, OnEditPasteNewPatterns)
	ON_COMMAND(ID_FRAME_DELETE, OnEditDelete)
	ON_COMMAND(ID_MODULE_INSERTFRAME, OnModuleInsertFrame)
	ON_COMMAND(ID_MODULE_REMOVEFRAME, OnModuleRemoveFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAME, OnModuleDuplicateFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAMEPATTERNS, OnModuleDuplicateFramePatterns)
	ON_COMMAND(ID_MODULE_MOVEFRAMEDOWN, OnModuleMoveFrameDown)
	ON_COMMAND(ID_MODULE_MOVEFRAMEUP, OnModuleMoveFrameUp)
	// // //
	ON_COMMAND(ID_MODULE_DUPLICATECURRENTPATTERN, OnModuleDuplicateCurrentPattern)
END_MESSAGE_MAP()


void CFrameEditor::AssignDocument(CFamiTrackerDoc *pDoc, CFamiTrackerView *pView)
{
	m_pDocument = pDoc;
	m_pView		= pView;
}

// CFrameEditor message handlers

int CFrameEditor::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_hAccel = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_FRAMEWND));

	m_Font.CreateFont(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, DEFAULT_FONT);

	m_iClipboard = ::RegisterClipboardFormat(CLIPBOARD_ID);

	m_DropTarget.Register(this);
	m_DropTarget.SetClipBoardFormat(m_iClipboard);

	m_iDragThresholdX = ::GetSystemMetrics(SM_CXDRAG);
	m_iDragThresholdY = ::GetSystemMetrics(SM_CYDRAG);

	return 0;
}

void CFrameEditor::OnPaint()
{
	CPaintDC dc(this); // device context for painting

//#define BENCHMARK

	// Do not call CWnd::OnPaint() for painting messages
	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	// Get window size
	CRect WinRect;
	GetClientRect(&WinRect);

	if (!CFamiTrackerDoc::GetDoc()->IsFileLoaded()) {
		dc.FillSolidRect(WinRect, 0);
		return;
	}
	else if (pSoundGen == NULL)
		return;
	else if (pSoundGen->IsBackgroundTask())
		return;

	unsigned int Width = WinRect.Width();
	unsigned int Height = WinRect.Height();

	// Check if width has changed, delete objects then
	if (m_bmpBack.m_hObject != NULL) {
		CSize size = m_bmpBack.GetBitmapDimension();
		if (size.cx != m_iWinWidth) {
			m_bmpBack.DeleteObject();
			m_dcBack.DeleteDC();
		}
	}

	// Allocate object
	if (m_dcBack.m_hDC == NULL) {
		m_bmpBack.CreateCompatibleBitmap(&dc, m_iWinWidth, m_iWinHeight);
		m_bmpBack.SetBitmapDimension(m_iWinWidth, m_iWinHeight);
		m_dcBack.CreateCompatibleDC(&dc);
		m_dcBack.SelectObject(&m_bmpBack);
		InvalidateFrameData();
	}

	if (NeedUpdate()) {
		DrawFrameEditor(&m_dcBack);
		++m_iUpdates;
	}

	m_bInvalidated = false;

	dc.BitBlt(0, 0, m_iWinWidth, m_iWinHeight, &m_dcBack, 0, 0, SRCCOPY);

#ifdef BENCHMARK
	++m_iPaints;
	CString txt;
	txt.Format("Updates %i", m_iUpdates);
	dc.TextOut(1, 1, txt);
	txt.Format("Paints %i", m_iPaints);
	dc.TextOut(1, 18, txt);
#endif
}

void CFrameEditor::DrawFrameEditor(CDC *pDC)
{
	// Draw the frame editor window

	const COLORREF QUEUE_COLOR	  = 0x108010;	// Colour of row in play queue
	const COLORREF RED_BAR_COLOR  = 0x4030A0;
	const COLORREF BLUE_BAR_COLOR = 0xA02030;

	// Cache settings
	const COLORREF ColBackground	= theApp.GetSettings()->Appearance.iColBackground;
	const COLORREF ColText			= theApp.GetSettings()->Appearance.iColPatternText;
	const COLORREF ColTextHilite	= theApp.GetSettings()->Appearance.iColPatternTextHilite;
	const COLORREF ColCursor		= theApp.GetSettings()->Appearance.iColCursor;
	const COLORREF ColSelect		= theApp.GetSettings()->Appearance.iColSelection;
	const COLORREF ColDragCursor	= INTENSITY(ColBackground) > 0x80 ? 0x000000 : 0xFFFFFF;
	const COLORREF ColSelectEdge	= BLEND(ColSelect, 0xFFFFFF, 70);
	const COLORREF ColTextDimmed	= DIM(theApp.GetSettings()->Appearance.iColPatternText, 90);

	const bool bHexRows				= theApp.GetSettings()->General.bRowInHex;

	LPCTSTR ROW_FORMAT = bHexRows ? _T("%02X") : _T("%02i");

	const CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	const CFamiTrackerView *pView = CFamiTrackerView::GetView();

	const int Track			= m_pMainFrame->GetSelectedTrack();
	const int FrameCount	= pDoc->GetFrameCount(Track);
	const int ChannelCount	= pDoc->GetAvailableChannels();
	int ActiveFrame			= pView->GetSelectedFrame();
	int ActiveChannel		= pView->GetSelectedChannel();
	int SelectStart			= std::min(m_iSelStartRow, m_iSelEndRow);
	int SelectEnd			= std::max(m_iSelStartRow, m_iSelEndRow);	
	int PatternAreaWidth	= m_iWinWidth - ROW_COLUMN_WIDTH;

	if (ActiveChannel < m_iFirstChannel)
		m_iFirstChannel = ActiveChannel;
	if (ActiveChannel >= (m_iFirstChannel + (ChannelCount - 1)))
		m_iFirstChannel = ActiveChannel - (ChannelCount - 1);

	if (ActiveFrame > (FrameCount - 1))
		ActiveFrame = (FrameCount - 1);
	if (ActiveFrame < 0)
		ActiveFrame = 0;

	CFont *pOldFont = m_dcBack.SelectObject(&m_Font);

	m_dcBack.SetBkMode(TRANSPARENT);

	// Draw background
	m_dcBack.FillSolidRect(0, 0, m_iWinWidth, m_iWinHeight, ColBackground);
	
	// Selected row
	COLORREF RowColor = BLEND(ColCursor, ColBackground, 50);

	if (GetFocus() == this) {
		if (m_bInputEnable)
			RowColor = BLEND(RED_BAR_COLOR, 0, 80);
		else
			RowColor = BLEND(BLUE_BAR_COLOR, ColBackground, 80);
	}

	// Draw selected row
	GradientBar(&m_dcBack, 0, SY(m_iMiddleRow * ROW_HEIGHT + 3), m_iWinWidth, SY(ROW_HEIGHT + 1), RowColor, ColBackground);	

	int FirstVisibleFrame = ActiveFrame - m_iMiddleRow;
	int Frame = 0;
	int Start = 0;
	int End = m_iRowsVisible;

	int PlayFrame = theApp.GetSoundGenerator()->GetPlayerFrame();

	if (ActiveFrame > m_iMiddleRow)
		Frame = ActiveFrame - m_iMiddleRow;
	if (FirstVisibleFrame + Start < 0)
		Start = -FirstVisibleFrame;
	if (FirstVisibleFrame + End >= FrameCount)
		End = Start + FrameCount - ((FirstVisibleFrame > 0) ? FirstVisibleFrame : 0);

	// Draw rows
	for (int i = Start; i < End; ++i) {
		
		// Play cursor
		if (PlayFrame == Frame && !pView->GetFollowMode() && theApp.IsPlaying()) {
			GradientBar(&m_dcBack, 0, SY(i * ROW_HEIGHT + 4), SX(m_iWinWidth), SY(ROW_HEIGHT - 1), CPatternEditor::ROW_PLAY_COLOR, ColBackground);
		}

		// Queue cursor
		if (theApp.GetSoundGenerator()->GetQueueFrame() == Frame) {
			GradientBar(&m_dcBack, 0, SY(i * ROW_HEIGHT + 4), SX(m_iWinWidth), SY(ROW_HEIGHT - 1), QUEUE_COLOR, ColBackground);
		}

		bool bSelectedRow = m_bSelecting && (Frame >= SelectStart) && (Frame <= SelectEnd);

		// Selection
		if (bSelectedRow) {
			m_dcBack.FillSolidRect(SX(ROW_COLUMN_WIDTH), SY(i * ROW_HEIGHT + 3), SX(m_iWinWidth - ROW_COLUMN_WIDTH), SY(ROW_HEIGHT), ColSelect);
			if (Frame == SelectStart)
				m_dcBack.FillSolidRect(SX(ROW_COLUMN_WIDTH), SY(i * ROW_HEIGHT + 3), SX(PatternAreaWidth), SY(1), ColSelectEdge);
			if (Frame == SelectEnd) 
				m_dcBack.FillSolidRect(SX(ROW_COLUMN_WIDTH), SY((i + 1) * ROW_HEIGHT + 3 - 1), SX(PatternAreaWidth), SY(1), ColSelectEdge);
		}

		if (i == m_iMiddleRow) {
			// Cursor box
			int x = ((ActiveChannel - m_iFirstChannel) * FRAME_ITEM_WIDTH);
			int y = m_iMiddleRow * ROW_HEIGHT + 3;
			
			GradientBar(&m_dcBack, SX(ROW_COLUMN_WIDTH + 2 + x), SY(y), SX(FRAME_ITEM_WIDTH), SY(ROW_HEIGHT + 1), ColCursor, ColBackground);
			m_dcBack.Draw3dRect(SX(ROW_COLUMN_WIDTH + 2 + x), SY(y), SX(FRAME_ITEM_WIDTH), SY(ROW_HEIGHT + 1), BLEND(ColCursor, 0xFFFFFF, 90), BLEND(ColCursor, ColBackground, 60));

			if (m_bInputEnable && m_bCursor) {
				// Flashing black box indicating that input is active
				m_dcBack.FillSolidRect(SX(ROW_COLUMN_WIDTH + 4 + x + CURSOR_WIDTH * m_iCursorPos), SY(y + 2), SX(CURSOR_WIDTH), SY(ROW_HEIGHT - 3), ColBackground);
			}
		}

		m_dcBack.SetTextColor(ColTextHilite);
		m_dcBack.TextOut(SX(5), SY(i * ROW_HEIGHT + 3), MakeIntString(Frame, ROW_FORMAT));

		COLORREF CurrentColor = (i == m_iHiglightLine || m_iHiglightLine == -1) ? ColText : ColTextDimmed;

		for (int j = 0; j < ChannelCount; ++j) {
			int Chan = j + m_iFirstChannel;

			// Dim patterns that are different from current
			if (pDoc->GetPatternAtFrame(Track, Frame, Chan) == pDoc->GetPatternAtFrame(Track, ActiveFrame, Chan) || bSelectedRow)
				m_dcBack.SetTextColor(CurrentColor);
			else
				m_dcBack.SetTextColor(DIM(CurrentColor, 70));

			CRect rect(SX(30 + j * FRAME_ITEM_WIDTH), SY(i * ROW_HEIGHT + 3), SX(28 + j * FRAME_ITEM_WIDTH + FRAME_ITEM_WIDTH), SY(i * ROW_HEIGHT + 3 + 20));
			m_dcBack.DrawText(MakeIntString(pDoc->GetPatternAtFrame(Track, Frame, Chan), _T("%02X")), rect, DT_LEFT | DT_TOP | DT_NOCLIP);
		}

		Frame++;
	}

	if (m_DropTarget.IsDragging()) {
		// Draw cursor when dragging
		if (!m_bSelecting || (m_iDragRow <= SelectStart || m_iDragRow >= (SelectEnd + 1))) {
			if (m_iDragRow >= FirstVisibleFrame && m_iDragRow <= FirstVisibleFrame + m_iRowsVisible) {
				int y = m_iDragRow - FirstVisibleFrame;
				m_dcBack.FillSolidRect(SX(ROW_COLUMN_WIDTH), SY(y * ROW_HEIGHT + 3), SX(m_iWinWidth), SY(2), ColDragCursor);
			}
		}
	}

	COLORREF colSeparator = BLEND(ColBackground, (ColBackground ^ 0xFFFFFF), 75);

	// Row number separator
	m_dcBack.FillSolidRect(SX(ROW_COLUMN_WIDTH - 1), 0, SY(1), m_iWinHeight, colSeparator);

	// Save draw info
	m_iLastCursorFrame = ActiveFrame;
	m_iLastCursorChannel = ActiveChannel;
	m_iLastPlayFrame = PlayFrame;

	// Restore old objects
	m_dcBack.SelectObject(pOldFont);

	// Update scrollbars
	if (FrameCount == 1)
		SetScrollRange(SB_VERT, 0, 1);
	else
		SetScrollRange(SB_VERT, 0, FrameCount - 1);
	
	SetScrollPos(SB_VERT, ActiveFrame);
	SetScrollRange(SB_HORZ, 0, ChannelCount - 1);
	SetScrollPos(SB_HORZ, ActiveChannel);
}

void CFrameEditor::SetupColors()
{
	// Color scheme has changed
	InvalidateFrameData();
}

void CFrameEditor::RedrawFrameEditor()
{
	// Public method for redrawing this editor

	if (NeedUpdate())
		RedrawWindow();
}

bool CFrameEditor::NeedUpdate() const
{
	// Checks if the cursor or frame data has changed and area needs to be redrawn

	const CFamiTrackerView *pView = CFamiTrackerView::GetView();
	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	const int ActiveFrame	= pView->GetSelectedFrame();
	const int ActiveChannel = pView->GetSelectedChannel();
	const int PlayFrame	    = theApp.GetSoundGenerator()->GetPlayerFrame();

	if (m_iLastCursorFrame != ActiveFrame)
		return true;

	if (m_iLastCursorChannel != ActiveChannel)
		return true;

	if (m_iLastPlayFrame != PlayFrame)
		return true;

	return m_bInvalidated;
}

void CFrameEditor::InvalidateFrameData()
{
	// Frame data has changed, must update
	m_bInvalidated = true;
}

void CFrameEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
		case SB_ENDSCROLL:
			return;
		case SB_LINEDOWN:
		case SB_PAGEDOWN:
			m_pView->SelectNextFrame();
			break;
		case SB_PAGEUP:
		case SB_LINEUP:
			m_pView->SelectPrevFrame();
			break;
		case SB_TOP:
			m_pView->SelectFirstFrame();
			break;
		case SB_BOTTOM:
			m_pView->SelectLastFrame();
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			if (m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) > 1)
				m_pView->SelectFrame(nPos);
			break;	
	}

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CFrameEditor::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
		case SB_ENDSCROLL:
			return;
		case SB_LINERIGHT:
		case SB_PAGERIGHT:
			m_pView->MoveCursorNextChannel();
			break;
		case SB_PAGELEFT:
		case SB_LINELEFT:
			m_pView->MoveCursorPrevChannel();
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_pView->SelectChannel(nPos);
			break;	
	}

	CWnd::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CFrameEditor::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	// Install frame editor's accelerator
	theApp.GetAccelerator()->SetAccelerator(m_hAccel);

	InvalidateFrameData();
	Invalidate();
}

void CFrameEditor::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	m_bInputEnable = false;
	InvalidateFrameData();
	Invalidate();
	theApp.GetAccelerator()->SetAccelerator(NULL);
}

void CFrameEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	const int PAGE_SIZE = 4;

	CMainFrame *pMainFrame = static_cast<CMainFrame*>(GetParentFrame());
	int Track = pMainFrame->GetSelectedTrack();
	
	bool bShift = (::GetKeyState(VK_SHIFT) & 0x80) == 0x80;

	int Num = -1;

	if (m_bInputEnable) {
		// Keyboard input is active

		if (nChar > 47 && nChar < 58)		// 0 - 9
			Num = nChar - 48;
		else if (nChar >= VK_NUMPAD0 && nChar <= VK_NUMPAD9)
			Num = nChar - VK_NUMPAD0;
		else if (nChar > 64 && nChar < 71)	// A - F
			Num = nChar - 65 + 0x0A;

		unsigned int ChannelCount = m_pDocument->GetChannelCount();
		unsigned int FrameCount = m_pDocument->GetFrameCount(Track);
		unsigned int Channel = m_pView->GetSelectedChannel();
		unsigned int Frame = m_pView->GetSelectedFrame();

		switch (nChar) {
			case VK_UP:
			case VK_DOWN:
			case VK_LEFT:
			case VK_RIGHT:
			case VK_NEXT:
			case VK_PRIOR:
			case VK_HOME:
			case VK_END:
				if (bShift && !m_bSelecting) {
					m_bSelecting = true;
					m_iSelStartRow = Frame;
					m_iSelEndRow = Frame;
					m_iSelStartChan = 0;
					m_iSelEndChan = ChannelCount - 1;
				}
				break;
		}

		switch (nChar) {
			case VK_LEFT:
				if (Channel == 0)
					Channel = ChannelCount - 1;
				else
					Channel -= 1;
				m_pView->SelectChannel(Channel);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, Frame, Channel);
				break;
			case VK_RIGHT:
				if (Channel == ChannelCount - 1)
					Channel = 0;
				else
					Channel += 1;
				m_pView->SelectChannel(Channel);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, Frame, Channel);
				break;
			case VK_UP:
				if (Frame == 0)
					Frame = FrameCount - 1;
				else
					Frame -= 1;
				m_pView->SelectFrame(Frame);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, Frame, Channel);
				break;
			case VK_DOWN:
				if (Frame == FrameCount - 1)
					Frame = 0;
				else 
					Frame += 1;
				m_pView->SelectFrame(Frame);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, Frame, Channel);
				break;
			case VK_RETURN:
				m_pView->SetFocus();
				break;
			case VK_INSERT:
				OnModuleInsertFrame();
				break;
			case VK_DELETE:
				OnModuleRemoveFrame();
				break;
			case VK_NEXT:
				if (Frame + PAGE_SIZE >= FrameCount)
					Frame = FrameCount - 1;
				else
					Frame += PAGE_SIZE;
				m_pView->SelectFrame(Frame);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, Frame, Channel);
				break;
			case VK_PRIOR:
				if ((signed)Frame - PAGE_SIZE < 0)
					Frame = 0;
				else
					Frame -= PAGE_SIZE;
				m_pView->SelectFrame(Frame);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, Frame, Channel);
				break;
			case VK_HOME:
				Frame = 0;
				m_pView->SelectFrame(Frame);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, Frame, Channel);
				break;
			case VK_END:
				Frame = FrameCount - 1;
				m_pView->SelectFrame(Frame);
				m_iCursorPos = 0;
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, Frame, Channel);
				break;
		}

		switch (nChar) {
			case VK_UP:
			case VK_DOWN:
			case VK_LEFT:
			case VK_RIGHT:
			case VK_NEXT:
			case VK_PRIOR:
			case VK_HOME:
			case VK_END:
				if (bShift) {
					m_iSelEndRow = Frame;
					InvalidateFrameData();
					Invalidate();
				}
				else if (m_bSelecting) {
					m_bSelecting = false;
					InvalidateFrameData();
					Invalidate();
				}
				break;
		}

		if (Num != -1) {
			if (m_iCursorPos == 0)
				m_iNewPattern = (m_iNewPattern & 0x0F) | (Num << 4);
			else if (m_iCursorPos == 1)
				m_iNewPattern = (m_iNewPattern & 0xF0) | Num;

			m_iNewPattern = std::min(m_iNewPattern, MAX_PATTERN - 1);

			if (pMainFrame->ChangeAllPatterns()) {
				CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_SET_PATTERN_ALL);
				pAction->SetPattern(m_iNewPattern);
				pMainFrame->AddAction(pAction);
			}
			else {
				CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_SET_PATTERN);
				pAction->SetPattern(m_iNewPattern);
				pMainFrame->AddAction(pAction);
			}

			m_pDocument->SetModifiedFlag();

			const int SelectedChannel = (m_pView->GetSelectedChannel() + 1) % m_pDocument->GetAvailableChannels();		// // //
			const int SelectedFrame = m_pView->GetSelectedFrame();

			if (m_iCursorPos == 1) {
				m_iCursorPos = 0;
				m_bCursor = true;
				m_pView->SelectChannel(SelectedChannel);		// // //
				m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, SelectedFrame, SelectedChannel);
			}
			else
				m_iCursorPos = 1;
		}
	}

	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CFrameEditor::OnTimer(UINT_PTR nIDEvent)
{
	if (m_bInputEnable) {
		m_bCursor = !m_bCursor;
		InvalidateFrameData();
		Invalidate();
	}

	CWnd::OnTimer(nIDEvent);
}

BOOL CFrameEditor::PreTranslateMessage(MSG* pMsg)
{
	// Temporary fix, accelerated messages must be sent to the main window
	if (theApp.GetAccelerator()->Translate(theApp.m_pMainWnd->m_hWnd, pMsg)) {
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN) {
		OnKeyDown(pMsg->wParam, pMsg->lParam & 0xFFFF, pMsg->lParam & 0xFF0000);
		// Remove the beep
		pMsg->message = WM_NULL;
	}

	return CWnd::PreTranslateMessage(pMsg);
}

int CFrameEditor::GetRowFromPoint(const CPoint &point, bool DropTarget) const
{
	// Translate a point value to a row
	int Delta = ((point.y - TOP_OFFSET) / ROW_HEIGHT) - m_iMiddleRow;
	int NewFrame = m_pView->GetSelectedFrame() + Delta;
	int FrameCount = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack());
	
	if (NewFrame < 0)
		NewFrame = 0;
	if (NewFrame >= FrameCount)
		NewFrame = FrameCount - (DropTarget ? 0 : 1);

	return NewFrame;
}

int CFrameEditor::GetChannelFromPoint(const CPoint &point) const
{
	// Translate a point value to a channel
	return (point.x - ROW_COLUMN_WIDTH - 2) / FRAME_ITEM_WIDTH;
}

unsigned int CFrameEditor::CalcWidth(int Channels) const
{
	return ROW_COLUMN_WIDTH + FRAME_ITEM_WIDTH * Channels + 25;
}

//// Mouse ////////////////////////////////////////////////////////////////////////////////////////

void CFrameEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	ScaleMouse(point);

	int Row = GetRowFromPoint(point, false);

	m_ButtonPoint = point;

	if (m_bSelecting) {

		int SelectStart	= std::min(m_iSelStartRow, m_iSelEndRow);
		int SelectEnd	= std::max(m_iSelStartRow, m_iSelEndRow);

		if (Row < SelectStart || Row > SelectEnd) {
			if (nFlags & MK_SHIFT) {
				m_iSelEndRow = Row;
				InvalidateFrameData();
				Invalidate();
			}
			else {
				m_iSelEndRow = m_iSelStartRow = Row;
				m_bSelecting = false;
				m_pView->SetFocus();
			}
		}
		else {
			m_bStartDrag = true;
		}
	}
	else {
		if (nFlags & MK_SHIFT) {
			m_iSelStartRow = m_pView->GetSelectedFrame();
			m_iSelEndRow = Row;
			m_bSelecting = true;
		}
		else
			m_iSelEndRow = m_iSelStartRow = Row;
	}
	
	CWnd::OnLButtonDown(nFlags, point);
}

void CFrameEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	ScaleMouse(point);

	int Channel	 = GetChannelFromPoint(point);
	int NewFrame = GetRowFromPoint(point, false);

	if (m_bSelecting) {
		if (m_bStartDrag) {
			m_bSelecting = false;
			m_bStartDrag = false;
			m_pView->SetFocus();
		}
		InvalidateFrameData();
		Invalidate();
	}
	else {
		if ((nFlags & MK_CONTROL) && theApp.IsPlaying()) {
			// Queue this frame
			if (NewFrame == theApp.GetSoundGenerator()->GetQueueFrame())
				// Remove
				theApp.GetSoundGenerator()->SetQueueFrame(-1);
			else
				// Set new
				theApp.GetSoundGenerator()->SetQueueFrame(NewFrame);

			InvalidateFrameData();
			Invalidate();
		}
		else {
			// Switch to frame
			m_pView->SelectFrame(NewFrame);
			if (Channel >= 0)
				m_pView->SelectChannel(Channel);
		}
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CFrameEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	ScaleMouse(point);
	
	if (nFlags & MK_LBUTTON) {
		if (!m_bSelecting) {
			if (abs(m_ButtonPoint.x - point.x) > m_iDragThresholdX || abs(m_ButtonPoint.y - point.y) > m_iDragThresholdY) {
				m_bSelecting = true;
				SetFocus();
				InvalidateFrameData();
				Invalidate();
			}
		}
		if (m_bStartDrag) {
			if (abs(m_ButtonPoint.x - point.x) > m_iDragThresholdX || abs(m_ButtonPoint.y - point.y) > m_iDragThresholdY) {
				InitiateDrag();
			}
		}
		else if (m_bSelecting) {
			AutoScroll(point);
			m_iSelEndRow = GetRowFromPoint(point, false);
			InvalidateFrameData();
			Invalidate();
		}
	}

	// Highlight
	int LastHighlightLine = m_iHiglightLine;

	m_iHiglightLine = (point.y - TOP_OFFSET) / ROW_HEIGHT;

	if (LastHighlightLine != m_iHiglightLine) {
		InvalidateFrameData();
		Invalidate();
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CFrameEditor::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	m_iHiglightLine = -1;
	InvalidateFrameData();
	Invalidate();
	CWnd::OnNcMouseMove(nHitTest, point);
}

void CFrameEditor::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// Select channel and enable edit mode
	ScaleMouse(point);

	const int Channel  = GetChannelFromPoint(point);
	const int NewFrame = GetRowFromPoint(point, false);

	m_pView->SelectFrame(NewFrame);

	if (Channel >= 0)
		m_pView->SelectChannel(Channel);

	if (m_bInputEnable)
		m_pView->SetFocus();
	else
		EnableInput();

	CWnd::OnLButtonDblClk(nFlags, point);
}

void CFrameEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	ScaleMouse(point);
/*
	int Channel	 = GetChannelFromPoint(point);
	int NewFrame = GetRowFromPoint(point, false);

	m_pView->SelectFrame(NewFrame);

	if (Channel >= 0)
		m_pView->SelectChannel(Channel);
*/
	CWnd::OnRButtonUp(nFlags, point);
}

BOOL CFrameEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta > 0) {
		// Up
		m_pView->SelectPrevFrame();
	}
	else {
		// Down
		m_pView->SelectNextFrame();
	}

	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

void CFrameEditor::AutoScroll(const CPoint &point)
{
	const int Row = GetRowFromPoint(point, false);
	const int Rows = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack());

	if (point.y <= m_iTopScrollArea && Row > 0) {
		m_pView->SelectPrevFrame();
	}
	else if (point.y >= m_iBottomScrollArea && Row < (Rows - 1)) {
		m_pView->SelectNextFrame();
	}
}

void CFrameEditor::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	// Popup menu
	CMenu *pPopupMenu, PopupMenuBar;
	PopupMenuBar.LoadMenu(IDR_FRAME_POPUP);
	pPopupMenu = PopupMenuBar.GetSubMenu(0);

	// Paste menu item
	BOOL ClipboardAvailable = IsClipboardAvailable();
	pPopupMenu->EnableMenuItem(ID_FRAME_PASTE, MF_BYCOMMAND | (ClipboardAvailable ? MF_ENABLED : MF_DISABLED));
	pPopupMenu->EnableMenuItem(ID_FRAME_PASTENEWPATTERNS, MF_BYCOMMAND | (ClipboardAvailable ? MF_ENABLED : MF_DISABLED));

	pPopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}

void CFrameEditor::EnableInput()
{
	// Set focus and enable input, input is disabled when focus is lost
	SetFocus();

	const int Track = m_pMainFrame->GetSelectedTrack();
	const int Frame = m_pView->GetSelectedFrame();
	const int Channel = m_pView->GetSelectedChannel();

	m_bInputEnable = true;
	m_bCursor = true;
	m_iCursorPos = 0;
	m_iNewPattern = m_pDocument->GetPatternAtFrame(Track, Frame, Channel);

	SetTimer(0, 500, NULL);	// Cursor timer

	InvalidateFrameData();
	Invalidate();
}

void CFrameEditor::OnEditCut()
{
	OnEditCopy();
	OnEditDelete();
}

void CFrameEditor::OnEditCopy()
{
	CClipboard Clipboard(this, m_iClipboard);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (!m_bSelecting) {
		m_iSelStartRow = m_iSelEndRow = m_pView->GetSelectedFrame();
	}

	const int SelectStart = std::min(m_iSelStartRow, m_iSelEndRow);
	const int SelectEnd	  = std::max(m_iSelStartRow, m_iSelEndRow);
	const int Channels	  = m_pDocument->GetChannelCount();
	const int Rows		  = SelectEnd - SelectStart + 1;

	CFrameClipData ClipData(Channels, Rows);

	ClipData.ClipInfo.Channels = Channels;
	ClipData.ClipInfo.Rows = Rows;
	ClipData.ClipInfo.OleInfo.SourceRowStart = SelectStart;
	ClipData.ClipInfo.OleInfo.SourceRowEnd = SelectEnd;

	for (int i = 0; i < Rows; ++i) {
		for (int j = 0; j < Channels; ++j) {
			ClipData.SetFrame(i, j, m_pDocument->GetPatternAtFrame(m_pMainFrame->GetSelectedTrack(), i + SelectStart, j));
		}
	}

	SIZE_T Size = ClipData.GetAllocSize();
	HGLOBAL hMem = Clipboard.AllocMem(Size);

	if (hMem != NULL) {
		// Copy to clipboard
		ClipData.ToMem(hMem);
		// Set clipboard for internal data, hMem may not be used after this point
		Clipboard.SetData(hMem);
	}
}

void CFrameEditor::OnEditPaste()
{
	const int Track = m_pMainFrame->GetSelectedTrack();

	CClipboard Clipboard(this, m_iClipboard);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (!Clipboard.IsDataAvailable()) {
		AfxMessageBox(IDS_CLIPBOARD_NOT_AVALIABLE);
		::CloseClipboard();
		return;
	}

	HGLOBAL hMem = Clipboard.GetData();

	if (hMem == NULL) {
		AfxMessageBox(IDS_CLIPBOARD_PASTE_ERROR);
		return;
	}

	CFrameClipData *pClipData = new CFrameClipData();
	pClipData->FromMem(hMem);

	// Check validity
	if (pClipData->ClipInfo.Channels != m_pDocument->GetChannelCount() ||
		pClipData->ClipInfo.Rows + m_pDocument->GetFrameCount(Track) > MAX_FRAMES) 
	{
		SAFE_RELEASE(pClipData);
		return;
	}

	CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_PASTE);
	pAction->SetPasteData(pClipData);
	m_pMainFrame->AddAction(pAction);
}

void CFrameEditor::OnEditPasteNewPatterns()
{
	const int Track = m_pMainFrame->GetSelectedTrack();

	CClipboard Clipboard(this, m_iClipboard);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (!Clipboard.IsDataAvailable()) {
		AfxMessageBox(IDS_CLIPBOARD_NOT_AVALIABLE);
		::CloseClipboard();
		return;
	}

	HGLOBAL hMem = Clipboard.GetData();

	if (hMem == NULL) {
		AfxMessageBox(IDS_CLIPBOARD_PASTE_ERROR);
		return;
	}

	CFrameClipData *pClipData = new CFrameClipData();
	pClipData->FromMem(hMem);

	// Check validity
	if (pClipData->ClipInfo.Channels != m_pDocument->GetChannelCount() ||
		pClipData->ClipInfo.Rows + m_pDocument->GetFrameCount(Track) > MAX_FRAMES) 
	{
		SAFE_RELEASE(pClipData);
		return;
	}

	CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_PASTE_NEW);
	pAction->SetPasteData(pClipData);
	m_pMainFrame->AddAction(pAction);
}

void CFrameEditor::OnEditDelete()
{
	if (!m_bSelecting)
		m_iSelStartRow = m_iSelEndRow = m_pView->GetSelectedFrame();

	CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_DELETE_SELECTION);
	m_pMainFrame->AddAction(pAction);
}

void CFrameEditor::Paste(unsigned int Track, CFrameClipData *pClipData)
{
	const int Rows = pClipData->ClipInfo.Rows;
	const int Channels = pClipData->ClipInfo.Channels;
	const int SelectedFrame = m_pView->GetSelectedFrame();

	for (int i = 0; i < Rows; ++i) {
		m_pDocument->InsertFrame(Track, SelectedFrame + i);
		for (int j = 0; j < Channels; ++j) {
			m_pDocument->SetPatternAtFrame(Track, SelectedFrame + i, j, pClipData->GetFrame(i, j));
		}
	}
}

void CFrameEditor::PasteNew(unsigned int Track, CFrameClipData *pClipData)
{
	const int Rows = pClipData->ClipInfo.Rows;
	const int Channels = pClipData->ClipInfo.Channels;
	const int SelectedFrame = m_pView->GetSelectedFrame();

	for (int i = 0; i < Rows; ++i) {
		m_pDocument->InsertFrame(Track, SelectedFrame + i);
		for (int j = 0; j < Channels; ++j) {
			int Source = pClipData->GetFrame(i, j);
			int Target = m_pDocument->GetPatternAtFrame(Track, SelectedFrame + i, j);
			m_pDocument->CopyPattern(Track, Target, Source, j);
		}
	}
}

bool CFrameEditor::InputEnabled() const
{
	return m_bInputEnable;
}

void CFrameEditor::OnModuleInsertFrame()
{
	m_pMainFrame->OnModuleInsertFrame();
}

void CFrameEditor::OnModuleRemoveFrame()
{
	m_pMainFrame->OnModuleRemoveFrame();
}

void CFrameEditor::OnModuleDuplicateFrame()
{
	m_pMainFrame->OnModuleDuplicateFrame();
}

void CFrameEditor::OnModuleDuplicateFramePatterns()
{
	m_pMainFrame->OnModuleDuplicateFramePatterns();
}

void CFrameEditor::OnModuleMoveFrameDown()
{
	m_pMainFrame->OnModuleMoveframedown();
}

void CFrameEditor::OnModuleMoveFrameUp()
{
	m_pMainFrame->OnModuleMoveframeup();
}

void CFrameEditor::OnModuleDuplicateCurrentPattern()		// // //
{
	m_pMainFrame->OnModuleDuplicateCurrentPattern();
}

void CFrameEditor::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	m_iWinWidth = cx;
	m_iWinHeight = cy;

	// Get number of rows visible
	int FullRowsVisible = (cy - TOP_OFFSET * 2) / ROW_HEIGHT;
	m_iRowsVisible = (cy - TOP_OFFSET * 2 + ROW_HEIGHT - TOP_OFFSET) / ROW_HEIGHT;
	m_iMiddleRow = m_iRowsVisible / 2;

	m_iTopScrollArea = ROW_HEIGHT;
	m_iBottomScrollArea = cy - ROW_HEIGHT;

	// Delete the back buffer
	m_bmpBack.DeleteObject();
	m_dcBack.DeleteDC();
}

void CFrameEditor::CancelSelection()
{
	m_bSelecting = false;
	m_bStartDrag = false;
}

void CFrameEditor::InitiateDrag()
{
	const int SelectStart = std::min(m_iSelStartRow, m_iSelEndRow);
	const int SelectEnd	  = std::max(m_iSelStartRow, m_iSelEndRow);
	const int Channels	  = m_pDocument->GetChannelCount();
	const int Rows		  = SelectEnd - SelectStart + 1;

	COleDataSource *pSrc = new COleDataSource();

	m_bDeletedRows = false;

	// Create clipboard structure
	CFrameClipData ClipData(Channels, Rows);

	ClipData.ClipInfo.Channels = Channels;
	ClipData.ClipInfo.Rows = Rows;
	ClipData.ClipInfo.OleInfo.SourceRowStart = SelectStart;
	ClipData.ClipInfo.OleInfo.SourceRowEnd = SelectEnd;

	for (int i = 0; i < Rows; ++i) {
		for (int j = 0; j < Channels; ++j) {
			ClipData.SetFrame(i, j, m_pDocument->GetPatternAtFrame(m_pMainFrame->GetSelectedTrack(), i + SelectStart, j));
		}
	}

	SIZE_T Size = ClipData.GetAllocSize();
	HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, Size);

	if (hMem != NULL) {
		// Copy to clipboard

		ClipData.ToMem(hMem);

		// Setup OLE
		pSrc->CacheGlobalData(m_iClipboard, hMem);
		DROPEFFECT res = pSrc->DoDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE);

		if (res == DROPEFFECT_MOVE) {
			if (!m_bDeletedRows) {
				// Target was another window, delete rows locally
				CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_DELETE_SELECTION);
				m_pMainFrame->AddAction(pAction);
				m_bSelecting = false;
			}
		}

		::GlobalFree(hMem);
	}
	
	SAFE_RELEASE(pSrc);

	m_bStartDrag = false;
}

bool CFrameEditor::IsCopyValid(COleDataObject* pDataObject) const
{
	// Return true if the number of pasted frames will fit
	CFrameClipData *pClipData = new CFrameClipData();
	HGLOBAL hMem = pDataObject->GetGlobalData(m_iClipboard);
	pClipData->FromMem(hMem);
	int Frames = pClipData->ClipInfo.Rows;
	SAFE_RELEASE(pClipData);
	return (m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) + Frames) <= MAX_FRAMES;
}

void CFrameEditor::UpdateDrag(const CPoint &point)
{
	CPoint newPoint(point.x, point.y + ROW_HEIGHT / 2);
	m_iDragRow = GetRowFromPoint(newPoint, true);
	AutoScroll(point);
	InvalidateFrameData();
	Invalidate();
}

BOOL CFrameEditor::DropData(COleDataObject* pDataObject, DROPEFFECT dropEffect)
{
	// Drag'n'drop operation completed

	const int Track  = m_pMainFrame->GetSelectedTrack();
	const int Frames = m_pDocument->GetFrameCount(Track);

	ASSERT(m_iDragRow >= 0 && m_iDragRow <= Frames);

	// Get frame data
	CFrameClipData *pClipData = new CFrameClipData();
	HGLOBAL hMem = pDataObject->GetGlobalData(m_iClipboard);
	pClipData->FromMem(hMem);

	const int SelectStart = pClipData->ClipInfo.OleInfo.SourceRowStart;
	const int SelectEnd	  = pClipData->ClipInfo.OleInfo.SourceRowEnd;

	if (m_bSelecting && (m_iDragRow > SelectStart && m_iDragRow < (SelectEnd + 1))) {
		SAFE_RELEASE(pClipData);
		return FALSE;
	}

	// Add action
	switch (dropEffect) {
		case DROPEFFECT_COPY:
			// Copy
			if ((pClipData->ClipInfo.Rows + m_pDocument->GetFrameCount(Track)) <= MAX_FRAMES) {
				int Action = m_DropTarget.CopyToNewPatterns() ? CFrameAction::ACT_DRAG_AND_DROP_COPY_NEW : CFrameAction::ACT_DRAG_AND_DROP_COPY;
				CFrameAction *pAction = new CFrameAction(Action);
				pAction->SetDragInfo(m_iDragRow, pClipData, false);
				m_pMainFrame->AddAction(pAction);
			}
			else {
				SAFE_RELEASE(pClipData);
				return FALSE;
			}
			break;
		case DROPEFFECT_MOVE:
			// Move
			if (m_iDragRow >= SelectStart && m_iDragRow <= (SelectEnd + 1)) {
				// Disallow dragging to the same area
				SAFE_RELEASE(pClipData);
				return FALSE;
			}
			CFrameAction *pAction = new CFrameAction(CFrameAction::ACT_DRAG_AND_DROP_MOVE);
			pAction->SetDragInfo(m_iDragRow, pClipData, m_bStartDrag);
			m_pMainFrame->AddAction(pAction);
			break;
	}

	m_bDeletedRows = true;

	InvalidateFrameData();
	Invalidate();

	return TRUE;
}

void CFrameEditor::PerformDragOperation(unsigned int Track, CFrameClipData *pClipData, int DragTarget, bool bDelete, bool bNewPatterns)
{
	const int SelStart	= pClipData->ClipInfo.OleInfo.SourceRowStart;
	const int SelEnd	= pClipData->ClipInfo.OleInfo.SourceRowEnd;
	const int Rows		= pClipData->ClipInfo.Rows;
	const int Channels	= pClipData->ClipInfo.Channels;
	
	int SelectedFrame = m_pView->GetSelectedFrame();

	if (bDelete) {
		// Target and source is same window
		if (DragTarget > SelEnd)
			DragTarget -= Rows;

		for (int i = 0; i < Rows; ++i) {
			m_pDocument->RemoveFrame(Track, SelStart);
			if (SelStart < SelectedFrame)
				--SelectedFrame;
		}
	}

	for (int i = 0; i < Rows; ++i) {
		int Frame = DragTarget + i;
		m_pDocument->InsertFrame(Track, Frame);
		for (int j = 0; j < Channels; ++j) {
			if (bNewPatterns) {
				// Copy to new pattern numbers
				int Source = pClipData->GetFrame(i, j);
				int Target = m_pDocument->GetPatternAtFrame(Track, Frame, j);
				m_pDocument->CopyPattern(Track, Target, Source, j);
			}
			else {
				// Copy to existing pattern numbers
				m_pDocument->SetPatternAtFrame(Track, Frame, j, pClipData->GetFrame(i, j));
			}
		}
		if (DragTarget <= SelectedFrame)
			++SelectedFrame;
	}

	m_iSelStartRow = DragTarget;
	m_iSelEndRow = DragTarget + Rows - 1;

	m_pView->SelectFrame(SelectedFrame);
}

void CFrameEditor::GetSelectInfo(stSelectInfo &Info) const
{
	Info.bSelecting = m_bSelecting;
	Info.iRowStart = std::min(m_iSelStartRow, m_iSelEndRow);
	Info.iRowEnd = std::max(m_iSelStartRow, m_iSelEndRow);
}

void CFrameEditor::SetSelectInfo(stSelectInfo &Info)
{
	m_bSelecting = Info.bSelecting;
	m_iSelStartRow = Info.iRowStart;
	m_iSelEndRow = Info.iRowEnd;
}

bool CFrameEditor::IsClipboardAvailable() const
{
	return ::IsClipboardFormatAvailable(m_iClipboard) == TRUE;
}

// CFrameEditorDropTarget ////////////

void CFrameEditorDropTarget::SetClipBoardFormat(UINT iClipboard)
{
	m_iClipboard = iClipboard;
}

DROPEFFECT CFrameEditorDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	CFrameWnd *pMainFrame = dynamic_cast<CFrameWnd*>(theApp.m_pMainWnd);

	if (pDataObject->IsDataAvailable(m_iClipboard)) {
		if (dwKeyState & MK_CONTROL) {
			if (m_pParent->IsCopyValid(pDataObject)) {
				m_nDropEffect = DROPEFFECT_COPY;
				m_bCopyNewPatterns = true;
				if (pMainFrame != NULL)
					pMainFrame->SetMessageText(IDS_FRAME_DROP_COPY_NEW);
			}
		}
		else if (dwKeyState & MK_SHIFT) {
			if (m_pParent->IsCopyValid(pDataObject)) {
				m_nDropEffect = DROPEFFECT_COPY;
				m_bCopyNewPatterns = false;
				if (pMainFrame != NULL)
					pMainFrame->SetMessageText(IDS_FRAME_DROP_COPY);
			}
		}
		else {
			m_nDropEffect = DROPEFFECT_MOVE;
			if (pMainFrame != NULL)
				pMainFrame->SetMessageText(IDS_FRAME_DROP_MOVE);
		}
		m_pParent->UpdateDrag(point);
	}

	return m_nDropEffect;
}

DROPEFFECT CFrameEditorDropTarget::OnDragOver(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	m_pParent->UpdateDrag(point);
	return m_nDropEffect;
}

BOOL CFrameEditorDropTarget::OnDrop(CWnd* pWnd, COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	m_nDropEffect = DROPEFFECT_NONE;
	m_pParent->UpdateDrag(point);
	return m_pParent->DropData(pDataObject, dropEffect);
}

void CFrameEditorDropTarget::OnDragLeave(CWnd* pWnd)
{
	m_nDropEffect = DROPEFFECT_NONE;
	m_pParent->Invalidate();
}

bool CFrameEditorDropTarget::IsDragging() const
{
	return m_nDropEffect != DROPEFFECT_NONE;
}

bool CFrameEditorDropTarget::CopyToNewPatterns() const
{
	return m_bCopyNewPatterns;
}

// CFrameClipData //////////////////////////////////////////////////////////////

void CFrameClipData::Alloc(int Size)
{
	iSize = Size;
	pFrames = new int[Size];
}

SIZE_T CFrameClipData::GetAllocSize() const
{
	return sizeof(ClipInfo) + sizeof(int) * iSize;
}

void CFrameClipData::ToMem(HGLOBAL hMem) 
{
	// From CPatternClipData to memory
	ASSERT(hMem != NULL);

	BYTE *pByte = (BYTE*)::GlobalLock(hMem);

	if (pByte != NULL) {
		memcpy(pByte, &ClipInfo, sizeof(ClipInfo));
		memcpy(pByte + sizeof(ClipInfo), pFrames, sizeof(int) * iSize);

		::GlobalUnlock(hMem);
	}
}

void CFrameClipData::FromMem(HGLOBAL hMem)
{
	// From memory to CPatternClipData
	ASSERT(hMem != NULL);
	ASSERT(pFrames == NULL);

	BYTE *pByte = (BYTE*)::GlobalLock(hMem);

	if (pByte != NULL) {
		memcpy(&ClipInfo, pByte, sizeof(ClipInfo));

		iSize = ClipInfo.Channels * ClipInfo.Rows;
		pFrames = new int[iSize];

		memcpy(pFrames, pByte + sizeof(ClipInfo), sizeof(int) * iSize);

		::GlobalUnlock(hMem);
	}
}

int CFrameClipData::GetFrame(int Frame, int Channel) const
{
	ASSERT(Frame >= 0 && Frame < ClipInfo.Rows);
	ASSERT(Channel >= 0 && Channel < ClipInfo.Channels);

	return *(pFrames + (Frame * ClipInfo.Channels + Channel));
}

void CFrameClipData::SetFrame(int Frame, int Channel, int Pattern)
{
	ASSERT(Frame >= 0 && Frame < ClipInfo.Rows);
	ASSERT(Channel >= 0 && Channel < ClipInfo.Channels);

	*(pFrames + (Frame * ClipInfo.Channels + Channel)) = Pattern;
}
