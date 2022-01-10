/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
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
#include "CompoundAction.h"		// // //
#include "Accelerator.h"
#include "PatternEditor.h"
#include "FrameEditor.h"
#include "SoundGen.h"
#include "Settings.h"
#include "Graphics.h"
#include "Clipboard.h"
#include "Bookmark.h"		// // //
#include "BookmarkCollection.h"		// // //
#include "BookmarkManager.h"		// // //
#include "DPI.h"		// // //
#include "TrackerChannel.h"

/*
 * CFrameEditor
 * This is the frame(order) editor to the left in the control panel
 *
 */

const TCHAR CFrameEditor::CLIPBOARD_ID[] = _T("FamiTracker Frames");

IMPLEMENT_DYNAMIC(CFrameEditor, CWnd)

CFrameEditor::CFrameEditor(CMainFrame *pMainFrm) :
	m_pMainFrame(pMainFrm),
	m_pDocument(NULL),
	m_pView(NULL),
	mClipboardFormat(0),
	m_iChannelView(theApp.GetSettings()->ChannelViewCount),		// // !!
	m_iMaxChannelView(theApp.GetSettings()->GUI.iMaxChannelView),		// // !!
	m_iWinWidth(0),
	m_iWinHeight(0),
	m_iHiglightLine(0),
	m_iFirstChannel(0),
	m_iCursorPos(0),
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
	m_iDragRow(0),
	m_bLastRow(false),		// // //
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
	ON_COMMAND(ID_FRAME_PASTEOVERWRITE, OnEditPasteOverwrite)
//	ON_UPDATE_COMMAND_UI(ID_FRAME_PASTEOVERWRITE, OnUpdateEditPasteOverwrite)
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

	m_Font.CreateFont(DPI::SY(14), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
		theApp.GetSettings()->Appearance.strFrameFont);		// // // 050B

	m_ChanFont.CreateFont((DPI::SY(14) - DPI::SY(14 / 4)), 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,	
		theApp.GetSettings()->Appearance.strFrameFont);		// // !!


	mClipboardFormat = ::RegisterClipboardFormat(CLIPBOARD_ID);

	m_DropTarget.Register(this);
	m_DropTarget.SetClipBoardFormat(mClipboardFormat);

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

	CPoint ArrowPoints[3];
	CBrush HoverBrush(ColText);
	CBrush BlackBrush(ColBackground);
	CPen HoverPen(PS_SOLID, 1, ColText);
	CPen BlackPen(PS_SOLID, 1, ColTextDimmed);

	const bool bHexRows				= theApp.GetSettings()->General.bRowInHex;

	LPCTSTR ROW_FORMAT = bHexRows ? _T("%02X") : _T("%03i");


	const int Track			= m_pMainFrame->GetSelectedTrack();
	const int FrameCount	= m_pDocument->GetFrameCount(Track);
	const int ChannelCount	= m_pDocument->GetChannelCount();
	int ActiveFrame			= GetEditFrame();		// // //
	int ActiveChannel		= m_pView->GetSelectedChannel();
	const int SelectStart	= m_selection.GetFrameStart();		// // //
	const int SelectEnd		= m_selection.GetFrameEnd();
	const int CBegin		= m_selection.GetChanStart();
	const int CEnd			= m_selection.GetChanEnd();
	int PatternAreaWidth	= m_iWinWidth - ROW_COLUMN_WIDTH;

	if (ActiveChannel < m_iFirstChannel)
		m_iFirstChannel = ActiveChannel;
	if (ActiveChannel >= (m_iFirstChannel + (ChannelCount - 1)))
		m_iFirstChannel = ActiveChannel - (ChannelCount - 1);

	if (m_bLastRow)		// // //
		++ActiveFrame;
	if (ActiveFrame > FrameCount)		// // //
		ActiveFrame = FrameCount;
	if (ActiveFrame < 0)
		ActiveFrame = 0;

	CFont *pOldFont = m_dcBack.SelectObject(&m_Font);

	m_dcBack.SetBkMode(TRANSPARENT);
	m_dcBack.SetTextAlign(TA_CENTER);		// // //

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
	GradientBar(&m_dcBack, DPI::Rect(0, m_iMiddleRow * ROW_HEIGHT + 3, m_iWinWidth, ROW_HEIGHT + 1), RowColor, ColBackground);	

	int FirstVisibleFrame = ActiveFrame - m_iMiddleRow;
	int Frame = 0;
	int Start = 0;
	int End = m_iRowsVisible;
	int ChannelOffset = GetChannelOffset();

	int PlayFrame = theApp.GetSoundGenerator()->GetPlayerFrame();

	if (ActiveFrame > m_iMiddleRow)
		Frame = ActiveFrame - m_iMiddleRow;
	if (FirstVisibleFrame + Start < 0)
		Start = -FirstVisibleFrame;
	if (FirstVisibleFrame + End >= FrameCount)
		End = Start + FrameCount - ((FirstVisibleFrame > 0) ? FirstVisibleFrame : 0);

	// Draw rows
	CBookmarkCollection *pCol = m_pDocument->GetBookmarkManager()->GetCollection(Track);		// // //
	
	for (int i = Start; i <= End; ++i) {
		CRect RowRect = DPI::Rect(0, i * ROW_HEIGHT + 4, m_iWinWidth, ROW_HEIGHT - 1);		// // //

		// // // Highlight by bookmarks
		if (i != m_iMiddleRow) if (const unsigned Count = pCol->GetCount()) for (unsigned j = 0; j < Count; ++j)
			if (pCol->GetBookmark(j)->m_iFrame == Frame) {
				GradientBar(&m_dcBack, RowRect, theApp.GetSettings()->Appearance.iColBackgroundHilite, ColBackground);
				break;
			}

		// Play cursor
		if (PlayFrame == Frame && !m_pView->GetFollowMode() && theApp.IsPlaying())
			GradientBar(&m_dcBack, RowRect, theApp.GetSettings()->Appearance.iColCurrentRowPlaying, ColBackground);		// // //

		// Queue cursor
		if (theApp.GetSoundGenerator()->GetQueueFrame() == Frame)
			GradientBar(&m_dcBack, RowRect, QUEUE_COLOR, ColBackground);

		bool bSelectedRow = m_bSelecting && (Frame >= SelectStart) && (Frame <= SelectEnd);

		// Selection
		if (bSelectedRow) {//28 + (j * FRAME_ITEM_WIDTH + FRAME_ITEM_WIDTH / 2) 
			CRect RowRect = DPI::Rect(ROW_COLUMN_WIDTH + FRAME_ITEM_WIDTH * CBegin - ChannelOffset * FRAME_ITEM_WIDTH,
										i * ROW_HEIGHT + 3, FRAME_ITEM_WIDTH * (CEnd - CBegin + 1), ROW_HEIGHT);		// // !!
			RowRect.OffsetRect(2, 0);
			RowRect.InflateRect(1, 0);
			m_dcBack.FillSolidRect(RowRect, ColSelect);
			if (Frame == SelectStart)
				m_dcBack.FillSolidRect(RowRect.left, RowRect.top, RowRect.Width(), 1, ColSelectEdge);
			if (Frame == SelectEnd) 
				m_dcBack.FillSolidRect(RowRect.left, RowRect.bottom - 1, RowRect.Width(), 1, ColSelectEdge);
			m_dcBack.FillSolidRect(RowRect.left, RowRect.top, 1, RowRect.Height(), ColSelectEdge);		// // //
			m_dcBack.FillSolidRect(RowRect.right - 1, RowRect.top, 1, RowRect.Height(), ColSelectEdge);		// // //
		}

		if (i == m_iMiddleRow) {
			// Cursor box w and h
			int x = ((ActiveChannel - m_iFirstChannel) * FRAME_ITEM_WIDTH) - ChannelOffset * FRAME_ITEM_WIDTH;
			int y = m_iMiddleRow * ROW_HEIGHT + 3;

			GradientBar(&m_dcBack, DPI::Rect(ROW_COLUMN_WIDTH + 2 + x, y, FRAME_ITEM_WIDTH, ROW_HEIGHT + 1), ColCursor, ColBackground);
			m_dcBack.Draw3dRect(DPI::Rect(ROW_COLUMN_WIDTH + 2 + x, y, FRAME_ITEM_WIDTH, ROW_HEIGHT + 1), BLEND(ColCursor, 0xFFFFFF, 90), BLEND(ColCursor, ColBackground, 60));

			if (m_bInputEnable && m_bCursor) {
				// Flashing black box indicating that input is active
				m_dcBack.FillSolidRect(DPI::Rect(ROW_COLUMN_WIDTH + 4 + x + CURSOR_WIDTH * m_iCursorPos, y + 2, CURSOR_WIDTH, ROW_HEIGHT - 3), ColBackground);
			}
		}

		if (i == End) {
			m_dcBack.SetTextColor(ColTextHilite);
			m_dcBack.TextOut(DPI::SX(3 + FRAME_ITEM_WIDTH / 2), DPI::SY(i * ROW_HEIGHT + 3), _T(">>"));

			COLORREF CurrentColor = (i == m_iHiglightLine || m_iHiglightLine == -1) ? ColText : ColTextDimmed;

			for (int j = 0; j < ChannelCount; ++j) {
				int Chan = j + m_iFirstChannel;

				//m_dcBack.SetTextColor(CurrentColor);
				m_dcBack.SetTextColor(DIM(CurrentColor, 70));

				m_dcBack.DrawText(_T("--"), DPI::Rect(28 + (j * FRAME_ITEM_WIDTH + FRAME_ITEM_WIDTH / 2) - ChannelOffset * FRAME_ITEM_WIDTH,
					i * ROW_HEIGHT + 3, FRAME_ITEM_WIDTH - 2, 20), DT_LEFT | DT_TOP | DT_NOCLIP);
			}
		}
		else {

			COLORREF CurrentColor = (i == m_iHiglightLine || m_iHiglightLine == -1) ? ColText : ColTextDimmed;

			for (int j = 0; j < ChannelCount; ++j) {
				int Chan = j + m_iFirstChannel;
				// Dim patterns that are different from current
				if (!m_bLastRow && m_pDocument->GetPatternAtFrame(Track, Frame, Chan) == m_pDocument->GetPatternAtFrame(Track, ActiveFrame, Chan)
								|| bSelectedRow)
					m_dcBack.SetTextColor(CurrentColor);
				else
					m_dcBack.SetTextColor(DIM(CurrentColor, 70));

				// Pattern number
				m_dcBack.DrawText(MakeIntString(m_pDocument->GetPatternAtFrame(Track, Frame, Chan), _T("%02X")),		// // //
					DPI::Rect(28 + (j * FRAME_ITEM_WIDTH + FRAME_ITEM_WIDTH / 2) - ChannelOffset * FRAME_ITEM_WIDTH,
					i * ROW_HEIGHT + 3, FRAME_ITEM_WIDTH - 2, 20), DT_LEFT | DT_TOP | DT_NOCLIP);
			}
		}
		++Frame;
	}
	// Channel name bg
	m_dcBack.FillSolidRect(0, 0, m_iWinWidth, DPI::SY(TOP_OFFSET * 2 + (14 - 14 / 4)), ColBackground);
	// Row number bg
	m_dcBack.FillSolidRect(0, 0, DPI::SX(ROW_COLUMN_WIDTH - 1), m_iWinHeight, ColBackground);	// // !!
	
	// reset some variables for drawing row numbers/channel names
	FirstVisibleFrame = ActiveFrame - m_iMiddleRow;
	Frame = 0;
	Start = 0;
	End = m_iRowsVisible;
	if (ActiveFrame > m_iMiddleRow)
		Frame = ActiveFrame - m_iMiddleRow;
	if (FirstVisibleFrame + Start < 0)
		Start = -FirstVisibleFrame;
	if (FirstVisibleFrame + End >= FrameCount)
		End = Start + FrameCount - ((FirstVisibleFrame > 0) ? FirstVisibleFrame : 0);

	// Row numbers and Channel names
	for (int i = Start; i <= End; ++i) {
		m_dcBack.SelectObject(m_Font);

		// // !! Row highlight by bookmarks
		if (const unsigned Count = pCol->GetCount()) for (unsigned j = 0; j < Count; ++j)
			if (pCol->GetBookmark(j)->m_iFrame == Frame) {
				GradientBar(&m_dcBack, DPI::Rect(0, i* ROW_HEIGHT + 4, ROW_COLUMN_WIDTH - 1, ROW_HEIGHT - 1), theApp.GetSettings()->Appearance.iColBackgroundHilite, ColBackground);
				break;
			}

		// // // 050B row marker
		if (Frame == m_pView->GetMarkerFrame())
			GradientBar(&m_dcBack, DPI::Rect(2, i * ROW_HEIGHT + 4, ROW_COLUMN_WIDTH - 5, ROW_HEIGHT - 1), ColCursor, DIM(ColCursor, 30));

		if (i == End) {
			m_dcBack.SetTextColor(ColTextHilite);
			m_dcBack.TextOut(DPI::SX(3 + FRAME_ITEM_WIDTH / 2), DPI::SY(i* ROW_HEIGHT + 3), _T(">>"));
			for (int j = 0; j < ChannelCount; ++j) {
				int Chan = j + m_iFirstChannel;
				COLORREF CurrentColor = (ActiveChannel == Chan) ? ColText : ColTextHilite;

				// Dim channels not in cursor
				m_dcBack.SetTextColor(CurrentColor);

				// Channel names
				m_dcBack.SelectObject(m_ChanFont);
				CTrackerChannel* pChannel = m_pDocument->GetChannel(Chan);
				m_dcBack.DrawText(pChannel->GetShortName(),			// // !!
					DPI::Rect(28 + (j * FRAME_ITEM_WIDTH + FRAME_ITEM_WIDTH / 2) - ChannelOffset * FRAME_ITEM_WIDTH,
						TOP_OFFSET,
						FRAME_ITEM_WIDTH - 2,
						20),
					DT_LEFT | DT_TOP | DT_NOCLIP);
				m_dcBack.SelectObject(m_Font);
			}
		}
		else {
			// Row number, number format determined by ROW_FORMAT
			m_dcBack.SetTextColor(ColTextHilite);
			m_dcBack.TextOut(DPI::SX(3 + FRAME_ITEM_WIDTH / 2), DPI::SY(i* ROW_HEIGHT + 3), MakeIntString(Frame, ROW_FORMAT));
		}
		++Frame;
	}
	// corner square
	m_dcBack.FillSolidRect(0, 0, DPI::SX(ROW_COLUMN_WIDTH - 1), DPI::SY(TOP_OFFSET * 2 + (14 - 14 / 4)), ColBackground);

	// arrows to adjust visible channels
	if (m_iChannelView < ChannelCount && m_iChannelView < m_iMaxChannelView) {
		ArrowPoints[0].SetPoint(DPI::SX(((ROW_COLUMN_WIDTH - 1) / 2) + 4 - 2), DPI::SY(TOP_OFFSET));
		ArrowPoints[1].SetPoint(DPI::SX(((ROW_COLUMN_WIDTH - 1) / 2) + 4 - 2), DPI::SY(TOP_OFFSET + 10));
		ArrowPoints[2].SetPoint(DPI::SX(((ROW_COLUMN_WIDTH - 1) / 2) + 4 + 3), DPI::SY(TOP_OFFSET + 5));

		CObject *pOldBrush = m_dcBack.SelectObject(m_bChannelViewArrow1 ? &HoverBrush : &BlackBrush);
		CObject *pOldPen = m_dcBack.SelectObject(m_bChannelViewArrow1 ? &HoverPen : &BlackPen);

		m_dcBack.Polygon(ArrowPoints, 3);
		m_dcBack.SelectObject(pOldBrush);
		m_dcBack.SelectObject(pOldPen);
	}
	if (m_iChannelView > 5) {
		ArrowPoints[0].SetPoint(DPI::SX(((ROW_COLUMN_WIDTH - 1) / 2) - 4 + 2), DPI::SY(TOP_OFFSET));
		ArrowPoints[1].SetPoint(DPI::SX(((ROW_COLUMN_WIDTH - 1) / 2) - 4 + 2), DPI::SY(TOP_OFFSET + 10));
		ArrowPoints[2].SetPoint(DPI::SX(((ROW_COLUMN_WIDTH - 1) / 2) - 4 - 3), DPI::SY(TOP_OFFSET + 5));

		CObject* pOldBrush = m_dcBack.SelectObject(m_bChannelViewArrow2 ? &HoverBrush : &BlackBrush);
		CObject* pOldPen = m_dcBack.SelectObject(m_bChannelViewArrow2 ? &HoverPen : &BlackPen);

		m_dcBack.Polygon(ArrowPoints, 3);
		m_dcBack.SelectObject(pOldBrush);
		m_dcBack.SelectObject(pOldPen);
	}

	if (m_DropTarget.IsDragging()) {
		// Draw cursor when dragging
		if (!m_bSelecting || (m_iDragRow <= SelectStart || m_iDragRow >= (SelectEnd + 1))) {
			if (m_iDragRow >= FirstVisibleFrame && m_iDragRow <= FirstVisibleFrame + m_iRowsVisible) {
				int x = ROW_COLUMN_WIDTH + CBegin * FRAME_ITEM_WIDTH + 2;		// // //
				int y = m_iDragRow - FirstVisibleFrame;
				m_dcBack.FillSolidRect(DPI::Rect(x, y * ROW_HEIGHT + 3, FRAME_ITEM_WIDTH * (CEnd - CBegin + 1) + 1, 2), ColDragCursor);
			}
		}
	}

	COLORREF colSeparator = BLEND(ColBackground, (ColBackground ^ 0xFFFFFF), 75);

	// Row number separator
	m_dcBack.FillSolidRect(DPI::SX(ROW_COLUMN_WIDTH - 1), 0, DPI::SY(1), m_iWinHeight, colSeparator);
	// Column number separator
	m_dcBack.FillSolidRect(0, DPI::SX(TOP_OFFSET * 2 + (14 - 14 / 4)), m_iWinWidth, DPI::SX(1), colSeparator);

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

	const int ActiveFrame	= GetEditFrame();		// // //
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
				SetEditFrame(nPos);		// // //
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
//	CancelSelection();		// // //
	m_bLastRow = false;		// // //
	InvalidateFrameData();
	Invalidate();
	theApp.GetAccelerator()->SetAccelerator(NULL);
}

void CFrameEditor::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	const int PAGE_SIZE = 4;

	int Track = m_pMainFrame->GetSelectedTrack();
	
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
		unsigned int Frame = GetEditFrame();		// // //

		switch (nChar) {
			case VK_UP:
			case VK_DOWN:
			case VK_LEFT:
			case VK_RIGHT:
			case VK_NEXT:
			case VK_PRIOR:
			case VK_HOME:
			case VK_END:
				if (bShift && !m_bLastRow) {		// // //
					if (!m_bSelecting) {
						m_bSelecting = true;
						m_selection.m_cpStart.m_iFrame = m_selection.m_cpEnd.m_iFrame = Frame;		// // //
						m_selection.m_cpStart.m_iChannel = m_selection.m_cpEnd.m_iChannel = Channel;		// // //
					}
				}
				else
					CancelSelection();
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
				break;
			case VK_RIGHT:
				if (Channel == ChannelCount - 1)
					Channel = 0;
				else
					Channel += 1;
				m_pView->SelectChannel(Channel);
				m_iCursorPos = 0;
				break;
			case VK_UP:
				if (Frame == 0)
					Frame = FrameCount - (m_bSelecting ? 1 : 0);
				else
					Frame -= 1;
				SetEditFrame(Frame);		// // //
				m_iCursorPos = 0;
				break;
			case VK_DOWN:
				if (Frame == FrameCount - (m_bSelecting ? 1 : 0))
					Frame = 0;
				else 
					Frame += 1;
				SetEditFrame(Frame);		// // //
				m_iCursorPos = 0;
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
					Frame = FrameCount - (m_bSelecting ? 1 : 0);
				else
					Frame += PAGE_SIZE;
				SetEditFrame(Frame);		// // //
				m_iCursorPos = 0;
				break;
			case VK_PRIOR:
				if ((signed)Frame - PAGE_SIZE < 0)
					Frame = 0;
				else
					Frame -= PAGE_SIZE;
				SetEditFrame(Frame);		// // //
				m_iCursorPos = 0;
				break;
			case VK_HOME:
				Frame = 0;
				SetEditFrame(Frame);		// // //
				m_iCursorPos = 0;
				break;
			case VK_END:
				Frame = FrameCount - (m_bSelecting ? 1 : 0);
				SetEditFrame(Frame);		// // //
				m_iCursorPos = 0;
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
					m_selection.m_cpEnd.m_iFrame = Frame;
					m_selection.m_cpEnd.m_iChannel = Channel;		// // //
				}
				// // //
				InvalidateFrameData();
				Invalidate();
				break;
		}

		if (Num != -1) {
			if (IsSelecting()) {		// // //
				int Pattern;
				if (m_iCursorPos == 0) {
					Pattern = Num << 4;
					m_iCursorPos = 1;
				}
				else if (m_iCursorPos == 1) {
					Pattern = m_pDocument->GetPatternAtFrame(Track, GetSelection().GetFrameStart(), GetSelection().GetChanStart()) | Num;
					m_iCursorPos = 0;
				}
				m_pMainFrame->AddAction(new CFActionSetPattern {Pattern});
				m_pDocument->SetModifiedFlag();
			}
			else if (!m_bLastRow || FrameCount < MAX_FRAMES) {		// // //
				int Pattern = m_bLastRow ? 0 : m_pDocument->GetPatternAtFrame(Track, Frame, Channel);		// // //
				if (m_iCursorPos == 0)
					Pattern = (Pattern & 0x0F) | (Num << 4);
				else if (m_iCursorPos == 1)
					Pattern = (Pattern & 0xF0) | Num;
				Pattern = std::min(Pattern, MAX_PATTERN - 1);

				if (m_bLastRow)
					m_pMainFrame->AddAction(new CFActionFrameCount {static_cast<int>(FrameCount) + 1});
				Action *pAction = m_pMainFrame->ChangeAllPatterns() ?
									   (Action*)new CFActionSetPatternAll {Pattern} : new CFActionSetPattern {Pattern};
				m_pMainFrame->AddAction(pAction);
				m_pDocument->SetModifiedFlag();

				const int SelectedChannel = (m_pView->GetSelectedChannel() + 1) % m_pDocument->GetAvailableChannels();		// // //
				const int SelectedFrame = m_pView->GetSelectedFrame();

				if (m_iCursorPos == 1) {
					m_iCursorPos = 0;
					m_bCursor = true;
					m_pView->SelectChannel(SelectedChannel);		// // //
				}
				else
					m_iCursorPos = 1;
			}
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

int CFrameEditor::GetChannelOffset() const
{
	// offset the patterns when out of bounds
	int ChannelOffset = 0;
	int ChanLim = m_iChannelView / 2;	// rounded up
	int ActiveChannel = m_pView->GetSelectedChannel();
	int ChannelCount = m_pDocument->GetChannelCount();
	if (ActiveChannel <= ChanLim)
		ChannelOffset = 0;
	else if (ActiveChannel + ChanLim >= ChannelCount)
		ChannelOffset = ChannelCount - m_iChannelView;
	else
		ChannelOffset = ActiveChannel - ChanLim;
	return ChannelOffset;
}

int CFrameEditor::GetArrowFromPoint(const CPoint& point) const
{
	// translate point value to one of the arrows
	int X = point.x;
	int Y = point.y;
	if (X >= 0 && X <= DPI::SX((ROW_COLUMN_WIDTH - 1) / 2) && Y >= 0 && Y<= DPI::SY(TOP_OFFSET * 2 + (14 - 14 / 4)))
		return 1;
	else if (X >= DPI::SX((ROW_COLUMN_WIDTH - 1) / 2) && X <= DPI::SX(ROW_COLUMN_WIDTH - 1) && Y >= 0 && Y <= DPI::SY(TOP_OFFSET * 2 + (14 - 14 / 4)))
		return 2;
	else
		return 0;
}

int CFrameEditor::GetRowFromPoint(const CPoint &point, bool DropTarget) const
{
	// Translate a point value to a row
	int Delta = ((point.y - TOP_OFFSET) / DPI::SY(ROW_HEIGHT)) - m_iMiddleRow;		// // //
	int NewFrame = GetEditFrame() + Delta;
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
	int Offs = point.x - DPI::SX(ROW_COLUMN_WIDTH) - 2;		// // //
	if (Offs < 0) return -1;
	int Channels = m_pDocument->GetChannelCount();		// // //
	Offs /= DPI::SX(FRAME_ITEM_WIDTH);
	if (Offs >= Channels)
		Offs = Channels - 1;
	return Offs + GetChannelOffset();
}

bool CFrameEditor::IsOverFrameColumn(const CPoint &point) const		// // //
{
	return point.x < DPI::SX(ROW_COLUMN_WIDTH);
}

unsigned int CFrameEditor::CalcWidth(int Channels) const
{
	// return the smallest width between input and current channel view count
	if (Channels < m_iChannelView)
		return ROW_COLUMN_WIDTH + FRAME_ITEM_WIDTH * Channels + 25;
	else
		return ROW_COLUMN_WIDTH + FRAME_ITEM_WIDTH * m_iChannelView + 25;
}

//// Mouse ////////////////////////////////////////////////////////////////////////////////////////

void CFrameEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	int Row = GetRowFromPoint(point, false);
	int Chan = GetChannelFromPoint(point);		// // //
	int Arrow = GetArrowFromPoint(point);		// // !!

	m_ButtonPoint = point;
	if (Arrow == 0) {
		if (m_bSelecting) {
			int SelectStart = m_selection.GetFrameStart(), SelectEnd = m_selection.GetFrameEnd();		// // //
			int ChanStart = m_selection.GetChanStart(), ChanEnd = m_selection.GetChanEnd();		// // //

			if (Row < SelectStart || Row > SelectEnd || Chan < ChanStart || Chan > ChanEnd) {		// // //
				if (nFlags & MK_SHIFT) {
					m_selection.m_cpEnd.m_iFrame = Row;
					m_selection.m_cpEnd.m_iChannel = Chan;		// // //
					InvalidateFrameData();
					Invalidate();
				}
				else {
					m_selection.m_cpStart.m_iFrame = m_selection.m_cpEnd.m_iFrame = Row;
					m_bFullFrameSelect = Chan < 0;		// // //
					if (m_bFullFrameSelect) {
						m_selection.m_cpStart.m_iChannel = 0;
						m_selection.m_cpEnd.m_iChannel = m_pDocument->GetChannelCount() - 1;
					}
					else
						m_selection.m_cpStart.m_iChannel = m_selection.m_cpEnd.m_iChannel = Chan;
					m_bSelecting = false;
					// m_pView->SetFocus();
				}
			}
			else {
				m_bStartDrag = true;
			}
		}
		else {
			if (nFlags & MK_SHIFT) {
				m_selection.m_cpStart = GetFrameCursor();		// // //
				m_selection.m_cpEnd.m_iFrame = Row;
				m_selection.m_cpEnd.m_iChannel = Chan;		// // //
				m_bFullFrameSelect = false;		// // //
				m_bSelecting = true;
			}
			else {
				m_selection.m_cpStart.m_iFrame = m_selection.m_cpEnd.m_iFrame = Row;
				m_bFullFrameSelect = Chan < 0;		// // //
				if (m_bFullFrameSelect) {
					m_selection.m_cpStart.m_iChannel = 0;
					m_selection.m_cpEnd.m_iChannel = m_pDocument->GetChannelCount() - 1;
				}
				else
					m_selection.m_cpStart.m_iChannel = m_selection.m_cpEnd.m_iChannel = Chan;
			}
		}
	}
	CWnd::OnLButtonDown(nFlags, point);
}

void CFrameEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	int Channel	 = GetChannelFromPoint(point);
	int NewFrame = GetRowFromPoint(point, false);
	int Arrow = GetArrowFromPoint(point);		// // !!

	if (m_bSelecting) {
		if (Arrow == 0) {
			if (m_bStartDrag) {
				m_bSelecting = false;
				m_bStartDrag = false;
				m_pView->SetFocus();
			}
			InvalidateFrameData();
			Invalidate();
		}
	}
	else {
		if (Arrow == 0) {
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
				if (Channel >= m_pDocument->GetChannelCount()) Channel = m_pDocument->GetChannelCount() - 1;
					m_pView->SelectChannel(Channel);		// // //
			}
		}
		else if (Arrow == 1) {
			if (m_iChannelView > 5) {
				m_iChannelView -= 1;
				theApp.GetSettings()->ChannelViewCount = m_iChannelView;
				// refresh the window
				m_pMainFrame->ResizeFrameWindow();
			}
		}
		else if (Arrow == 2) {
			if (m_iChannelView < m_pDocument->GetChannelCount() && m_iChannelView < m_iMaxChannelView) {
				m_iChannelView += 1;
				theApp.GetSettings()->ChannelViewCount = m_iChannelView;
				// refresh the window
				m_pMainFrame->ResizeFrameWindow();
			}
		}
	}

	CWnd::OnLButtonUp(nFlags, point);
}

void CFrameEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	int Arrow = GetArrowFromPoint(point);		// // !!
	if (Arrow == 0) {
		if (nFlags & MK_LBUTTON) {
			if (!m_bSelecting) {
				if (abs(m_ButtonPoint.x - point.x) > m_iDragThresholdX || abs(m_ButtonPoint.y - point.y) > m_iDragThresholdY) {
					m_bSelecting = true;
					EnableInput();		// // //
				}
			}
			if (m_bStartDrag) {
				if (abs(m_ButtonPoint.x - point.x) > m_iDragThresholdX || abs(m_ButtonPoint.y - point.y) > m_iDragThresholdY) {
					InitiateDrag();
				}
			}
			else if (m_bSelecting) {
				AutoScroll(point);
				m_selection.m_cpEnd.m_iFrame = GetRowFromPoint(point, false);
				if (m_bFullFrameSelect) {		// // //
					m_selection.m_cpStart.m_iChannel = 0;
					m_selection.m_cpEnd.m_iChannel = m_pDocument->GetChannelCount() - 1;
				}
				else {
					m_selection.m_cpEnd.m_iChannel = GetChannelFromPoint(point);
					if (m_selection.m_cpEnd.m_iChannel < 0) m_selection.m_cpEnd.m_iChannel = 0;
				}
				InvalidateFrameData();
				Invalidate();
			}
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

	const int Channel  = GetChannelFromPoint(point);
	const int NewFrame = GetRowFromPoint(point, true);		// // // allow one-past-the-end
	int Arrow = GetArrowFromPoint(point);		// // !!


	if (Arrow == 0) {
		SetEditFrame(NewFrame);		// // //

		if (Channel >= 0)
			m_pView->SelectChannel(Channel);

		if (m_bInputEnable)
			m_pView->SetFocus();
		else
			EnableInput();
	}

	CWnd::OnLButtonDblClk(nFlags, point);
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
	const int Row = GetRowFromPoint(point, true);		// // //
	const int Rows = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) + 1;

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
	m_pMainFrame->UpdateMenu(&PopupMenuBar);
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

	m_bInputEnable = true;
	m_bCursor = true;
	m_iCursorPos = 0;

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
	std::unique_ptr<CFrameClipData> ClipData {Copy()};		// // //

	CClipboard Clipboard(this, mClipboardFormat);

	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}

	if (HGLOBAL hMem = Clipboard.AllocMem(ClipData->GetAllocSize())) {
		// Copy to clipboard
		ClipData->ToMem(hMem);
		// Set clipboard for internal data, hMem may not be used after this point
		Clipboard.SetData(hMem);
	}
}

void CFrameEditor::OnEditPaste()
{
	CClipboard Clipboard(this, mClipboardFormat);
	HGLOBAL hMem;		// // //
	if (!Clipboard.GetData(hMem))
		return;

	CFrameClipData *pClipData = new CFrameClipData();
	pClipData->FromMem(hMem);

	m_pMainFrame->AddAction(new CFActionPaste {pClipData, GetEditFrame(), false});		// // //
}

void CFrameEditor::OnEditPasteOverwrite()		// // //
{
	CClipboard Clipboard(this, mClipboardFormat);
	HGLOBAL hMem;		// // //
	if (!Clipboard.GetData(hMem))
		return;

	CFrameClipData *pClipData = new CFrameClipData();
	pClipData->FromMem(hMem);

	m_pMainFrame->AddAction(new CFActionPasteOverwrite {pClipData});		// // //
}

void CFrameEditor::OnUpdateEditPasteOverwrite(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->Enable(IsClipboardAvailable() ? 1 : 0);
}

void CFrameEditor::OnEditPasteNewPatterns()
{
	CClipboard Clipboard(this, mClipboardFormat);
	HGLOBAL hMem;		// // //
	if (!Clipboard.GetData(hMem))
		return;

	CFrameClipData *pClipData = new CFrameClipData();
	pClipData->FromMem(hMem);

	m_pMainFrame->AddAction(new CFActionPaste {pClipData, GetEditFrame(), true});		// // //
}

void CFrameEditor::OnEditDelete()
{
	if (!m_bSelecting) {
		m_selection.m_cpStart.m_iFrame = m_selection.m_cpEnd.m_iFrame = m_pView->GetSelectedFrame();		// // //
		m_selection.m_cpStart.m_iChannel = 0;		// // //
		m_selection.m_cpEnd.m_iChannel = m_pDocument->GetChannelCount() - 1;		// // //
	}

	m_pMainFrame->AddAction(new CFActionDeleteSel { });		// // //
}

CFrameCursorPos CFrameEditor::GetFrameCursor() const		// // //
{
	return CFrameCursorPos {
		static_cast<int>(m_pView->GetSelectedFrame()),
		static_cast<int>(m_pView->GetSelectedChannel())
	};
}

std::pair<CFrameIterator, CFrameIterator> CFrameEditor::GetIterators() const		// // //
{
	int Track = m_pMainFrame->GetSelectedTrack();
	return m_bSelecting ?
		CFrameIterator::FromSelection(m_selection, m_pDocument, Track) :
		CFrameIterator::FromCursor(GetFrameCursor(), m_pDocument, Track);
}

CFrameClipData *CFrameEditor::Copy() const		// // //
{
	return m_bSelecting ? Copy(m_selection) : CopyFrame(m_pView->GetSelectedFrame());
}

CFrameClipData *CFrameEditor::Copy(const CFrameSelection &Sel) const		// // //
{
	const int Track = m_pMainFrame->GetSelectedTrack();
	auto it = CFrameIterator::FromSelection(Sel, m_pDocument, Track);
	const int Frames = it.second.m_iFrame - it.first.m_iFrame + 1;
	const int Channels = it.second.m_iChannel - it.first.m_iChannel + 1;		// // //
	const int ChanStart = it.first.m_iChannel;

	auto pData = new CFrameClipData {Channels, Frames};
	pData->ClipInfo.FirstChannel = ChanStart;		// // //
	pData->ClipInfo.OleInfo.SourceRowStart = it.first.m_iFrame;
	pData->ClipInfo.OleInfo.SourceRowEnd = it.second.m_iFrame;

	for (int f = 0; f < Frames; ++f) {
		for (int c = 0; c < Channels; ++c)
			pData->SetFrame(f, c, it.first.Get(c + ChanStart));
		++it.first;
	}

	return pData;
}

CFrameClipData *CFrameEditor::CopyFrame(int Frame) const		// // //
{
	CFrameSelection Sel;
	Sel.m_cpStart.m_iFrame = Sel.m_cpEnd.m_iFrame = Frame;
	Sel.m_cpStart.m_iChannel = 0;		// // //
	Sel.m_cpEnd.m_iChannel = m_pDocument->GetChannelCount() - 1;		// // //
	return Copy(Sel);
}

CFrameClipData *CFrameEditor::CopyEntire(int Track) const		// // //
{
	CFrameSelection Sel;
	Sel.m_cpEnd.m_iFrame = m_pDocument->GetFrameCount(Track) - 1;
	Sel.m_cpEnd.m_iChannel = m_pDocument->GetChannelCount() - 1;
	return Copy(Sel);
}

void CFrameEditor::PasteInsert(unsigned int Track, int Frame, const CFrameClipData *pClipData)		// // //
{
	const int Frames = pClipData->ClipInfo.Frames;
	const int Channels = pClipData->ClipInfo.Channels;

	CFrameSelection Sel;		// // //
	Sel.m_cpStart.m_iFrame = Frame;
	Sel.m_cpEnd.m_iFrame = Frame + Frames - 1;
	Sel.m_cpStart.m_iChannel = pClipData->ClipInfo.FirstChannel;		// // //
	Sel.m_cpEnd.m_iChannel = Sel.m_cpStart.m_iChannel + Channels - 1;

	for (int f = 0; f < Frames; ++f)
		m_pDocument->InsertFrame(Track, Frame);
	CFrameIterator it {m_pDocument, static_cast<int>(Track), Sel.m_cpStart};
	for (int f = 0; f < Frames; ++f) {
		for (int c = 0; c < it.m_iChannel; ++c)
			it.Set(c, 0);
		for (int c = 0; c < Channels; ++c)
			it.Set(c + it.m_iChannel, pClipData->GetFrame(f, c));
		for (int c = it.m_iChannel + Channels, Count = m_pDocument->GetChannelCount(); c < Count; ++c)
			it.Set(c, 0);
		++it;
	}

	SetSelection(Sel);
}

void CFrameEditor::PasteAt(unsigned int Track, const CFrameClipData *pClipData, const CFrameCursorPos &Pos)		// // //
{
	CFrameIterator it {m_pDocument, static_cast<int>(Track), Pos};
	for (int f = 0; f < pClipData->ClipInfo.Frames; ++f) {
		for (int c = 0; c < pClipData->ClipInfo.Channels; ++c)
			it.Set(c + /*it.m_iChannel*/ pClipData->ClipInfo.FirstChannel, pClipData->GetFrame(f, c));
		++it;
		if (it.m_iFrame == 0) break;
	}
}

void CFrameEditor::PasteNew(unsigned int Track, int Frame, const CFrameClipData *pClipData)		// // //
{
	int Count = m_pDocument->GetChannelCount();
	m_pDocument->AddFrames(Track, Frame, pClipData->ClipInfo.Frames);

	CFrameSelection Sel;
	Sel.m_cpStart.m_iFrame = Frame;
	Sel.m_cpEnd.m_iFrame = Frame + pClipData->ClipInfo.Frames - 1;
	Sel.m_cpStart.m_iChannel = pClipData->ClipInfo.FirstChannel;
	Sel.m_cpEnd.m_iChannel = Sel.m_cpStart.m_iChannel + pClipData->ClipInfo.Channels - 1;

	PasteAt(Track, pClipData, Sel.m_cpStart);
	ClonePatterns(Track, Sel);
	SetSelection(Sel);
}

void CFrameEditor::ClonePatterns(unsigned int Track, const CFrameSelection &_Sel)		// // //
{
	CFrameSelection Sel = _Sel.GetNormalized();
	auto it = CFrameIterator::FromSelection(Sel, m_pDocument, Track);
	std::unordered_map<std::pair<int, int>, int, pairhash> NewPatterns;

	while (true) {
		for (int c = it.first.m_iChannel; c <= it.second.m_iChannel; ++c) {
			int OldPattern = it.first.Get(c);
			auto Index = std::make_pair(c, OldPattern);
			auto p = NewPatterns.find(Index);		// // // share common patterns
			if (p == NewPatterns.end()) {
				NewPatterns[Index] = m_pDocument->GetFirstFreePattern(Track, c);
				m_pDocument->CopyPattern(Track, NewPatterns[Index], OldPattern, c);
			}
			m_pDocument->SetPatternAtFrame(Track, it.first.m_iFrame, c, NewPatterns[Index]);
		}
		if (it.first == it.second) break;
		++it.first;
	}
}

void CFrameEditor::ClearPatterns(unsigned int Track, const CFrameSelection &Sel)		// // //
{
	auto it = CFrameIterator::FromSelection(Sel, m_pDocument, Track);
	
	while (true) {
		for (int c = it.first.m_iChannel; c <= it.second.m_iChannel; ++c)
			m_pDocument->ClearPattern(Track, it.first.m_iFrame, c);
		if (it.first == it.second) break;
		++it.first;
	}
}

bool CFrameEditor::InputEnabled() const
{
	return m_bInputEnable;
}

void CFrameEditor::ResetCursor()		// // //
{
	m_pView->SelectFirstFrame();
	m_pView->SelectChannel(0);
	m_bLastRow = false;
	CancelSelection();
	InvalidateFrameData();
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

void CFrameEditor::OnEditSelectpattern()		// // //
{
	CFrameSelection Sel;
	Sel.m_cpStart = Sel.m_cpEnd = GetFrameCursor();
	SetSelection(Sel);
}

void CFrameEditor::OnEditSelectframe()		// // //
{
	CFrameSelection Sel;
	Sel.m_cpStart.m_iFrame = Sel.m_cpEnd.m_iFrame = m_pView->GetSelectedFrame();
	Sel.m_cpEnd.m_iChannel = m_pDocument->GetChannelCount() - 1;
	SetSelection(Sel);
}

void CFrameEditor::OnEditSelectchannel()		// // //
{
	CFrameSelection Sel;
	Sel.m_cpEnd.m_iFrame = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) - 1;
	Sel.m_cpStart.m_iChannel = Sel.m_cpEnd.m_iChannel = m_pView->GetSelectedChannel();
	SetSelection(Sel);
}

void CFrameEditor::OnEditSelectall()		// // //
{
	CFrameSelection Sel;
	Sel.m_cpEnd.m_iFrame = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) - 1;
	Sel.m_cpEnd.m_iChannel = m_pDocument->GetChannelCount() - 1;
	SetSelection(Sel);
}

void CFrameEditor::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	m_iWinWidth = cx;
	m_iWinHeight = cy;

	int Offset = DPI::SY(TOP_OFFSET);		// // // 050B
	int Height = DPI::SY(ROW_HEIGHT);		// // // 050B

	// Get number of rows visible
	m_iRowsVisible = (cy - Offset) / Height;		// // //
	m_iMiddleRow = m_iRowsVisible / 2;

	m_iTopScrollArea = ROW_HEIGHT * 2;	// offset by channel header
	m_iBottomScrollArea = cy - ROW_HEIGHT * 2;

	// Delete the back buffer
	m_bmpBack.DeleteObject();
	m_dcBack.DeleteDC();
}

void CFrameEditor::CancelSelection()
{
	if (m_bSelecting) {		// // //
		InvalidateFrameData();
		Invalidate();
	}
	m_bSelecting = false;
	m_bStartDrag = false;
}

void CFrameEditor::InitiateDrag()
{
	const int SelectStart = m_selection.GetFrameStart();		// // //
	const int SelectEnd	  = m_selection.GetFrameEnd();
	const int ChanStart	  = m_selection.GetChanStart();		// // //
	const int Channels	  = m_selection.GetChanEnd() - ChanStart + 1;		// // // m_pDocument->GetChannelCount();
	const int Rows		  = SelectEnd - SelectStart + 1;

	COleDataSource *pSrc = new COleDataSource();

	m_bDeletedRows = false;

	// Create clipboard structure
	CFrameClipData ClipData(Channels, Rows);
	ClipData.ClipInfo.FirstChannel = ChanStart;		// // //
	ClipData.ClipInfo.OleInfo.SourceRowStart = SelectStart;
	ClipData.ClipInfo.OleInfo.SourceRowEnd = SelectEnd;

	const int Track = m_pMainFrame->GetSelectedTrack();		// // //
	for (int i = 0; i < Rows; ++i)
		for (int j = 0; j < Channels; ++j)
			ClipData.SetFrame(i, j, m_pDocument->GetPatternAtFrame(Track, i + SelectStart, j + ChanStart));

	SIZE_T Size = ClipData.GetAllocSize();
	HGLOBAL hMem = ::GlobalAlloc(GMEM_MOVEABLE, Size);

	if (hMem != NULL) {
		// Copy to clipboard

		ClipData.ToMem(hMem);

		// Setup OLE
		pSrc->CacheGlobalData(mClipboardFormat, hMem);
		DROPEFFECT res = pSrc->DoDragDrop(DROPEFFECT_COPY | DROPEFFECT_MOVE); // calls DropData

		if (res == DROPEFFECT_MOVE) {
			if (!m_bDeletedRows) {
				// Target was another window, delete rows locally
				m_pMainFrame->AddAction(new CFActionDeleteSel { });		// // //
			}
		}

		::GlobalFree(hMem);
	}
	
	SAFE_RELEASE(pSrc);

	m_bStartDrag = false;
}

int CFrameEditor::GetEditFrame() const		// // //
{
	int Frame = m_pView->GetSelectedFrame();
	if (m_bLastRow)
		if (Frame != m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack()) - 1) {
			m_bLastRow = false;
			SetEditFrame(++Frame);
		}
	return Frame + (m_bLastRow ? 1 : 0);
}

void CFrameEditor::SetEditFrame(int Frame) const		// // //
{
	int Frames = m_pDocument->GetFrameCount(m_pMainFrame->GetSelectedTrack());
//	if (m_bLastRow != (Frame >= Frames))
//		InvalidateFrameData();
	if (m_bLastRow = (Frame >= Frames))
		Frame = Frames - 1;
	m_pView->SelectFrame(Frame);
}

bool CFrameEditor::IsCopyValid(COleDataObject* pDataObject) const
{
	// Return true if the number of pasted frames will fit
	CFrameClipData *pClipData = new CFrameClipData();
	HGLOBAL hMem = pDataObject->GetGlobalData(mClipboardFormat);
	pClipData->FromMem(hMem);
	int Frames = pClipData->ClipInfo.Frames;
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
	HGLOBAL hMem = pDataObject->GetGlobalData(mClipboardFormat);
	pClipData->FromMem(hMem);

	const int SelectStart = pClipData->ClipInfo.OleInfo.SourceRowStart;
	const int SelectEnd	  = pClipData->ClipInfo.OleInfo.SourceRowEnd;

	if (m_bSelecting && (m_iDragRow > SelectStart && m_iDragRow < (SelectEnd + 1))) {
		SAFE_RELEASE(pClipData);
		return FALSE;
	}

	// Add action
	switch (dropEffect) {
		case DROPEFFECT_MOVE:
			// Move
			if (m_bStartDrag) {		// // // same window
				if (m_iDragRow >= SelectStart && m_iDragRow <= (SelectEnd + 1)) {		// // //
					// Disallow dragging to the same area
					SAFE_RELEASE(pClipData);
					return FALSE;
				}
				m_pMainFrame->AddAction(new CFActionDropMove {pClipData, m_iDragRow});		// // //
				break;
			}
			// [[fallthrough]]
		case DROPEFFECT_COPY:
			// Copy
			if (!m_pMainFrame->AddAction(new CFActionPaste {pClipData, m_iDragRow, m_DropTarget.CopyToNewPatterns()})) {		// // //
				SAFE_RELEASE(pClipData);
				return FALSE;
			}
			break;
	}

	m_bDeletedRows = true;

	InvalidateFrameData();
	Invalidate();

	return TRUE;
}

void CFrameEditor::MoveSelection(unsigned int Track, const CFrameSelection &Sel, const CFrameCursorPos &Target)		// // //
{
	if (Target.m_iFrame == Sel.GetFrameStart()) return;
	CFrameSelection Normal = Sel.GetNormalized();
	auto pData = std::unique_ptr<CFrameClipData>(Copy(Normal));
	const int Frames = Normal.m_cpEnd.m_iFrame - Normal.m_cpStart.m_iFrame + 1;

	int Delta = Target.m_iFrame - Normal.m_cpStart.m_iFrame;
	if (Delta > 0) {
		CFrameSelection Tail(Normal);
		Tail.m_cpStart.m_iFrame += Frames;
		Tail.m_cpEnd.m_iFrame = Target.m_iFrame - 1;
		auto pRest = std::unique_ptr<CFrameClipData>(Copy(Tail));
		PasteAt(Track, pRest.get(), Normal.m_cpStart);
		Delta -= Frames;
		PasteAt(Track, pData.get(), {Normal.m_cpStart.m_iFrame + Delta, Normal.m_cpStart.m_iChannel});
	}
	else {
		CFrameSelection Head(Normal);
		Head.m_cpEnd.m_iFrame -= Frames;
		Head.m_cpStart.m_iFrame = Target.m_iFrame;
		auto pRest = std::unique_ptr<CFrameClipData>(Copy(Head));
		PasteAt(Track, pData.get(), Head.m_cpStart);
		Head.m_cpStart.m_iFrame += Frames;
		PasteAt(Track, pRest.get(), Head.m_cpStart);
	}
	Normal.m_cpStart.m_iFrame += Delta;
	Normal.m_cpEnd.m_iFrame += Delta;
	SetSelection(Normal);
}

CFrameSelection CFrameEditor::GetSelection() const		// // //
{
	if (!m_bSelecting) {
		CFrameSelection Sel;
		Sel.m_cpStart.m_iFrame = Sel.m_cpEnd.m_iFrame = m_pView->GetSelectedFrame();
		Sel.m_cpStart.m_iChannel = 0;
		Sel.m_cpEnd.m_iChannel = m_pDocument->GetChannelCount() - 1;
		return Sel;
	}
	return m_selection;
}

bool CFrameEditor::IsSelecting() const		// // //
{
	return m_bSelecting;
}

void CFrameEditor::SetSelection(const CFrameSelection &Sel)		// // //
{
	m_selection = Sel;
	m_bSelecting = true;
	InvalidateFrameData();
	Invalidate();
}

bool CFrameEditor::IsClipboardAvailable() const
{
	return ::IsClipboardFormatAvailable(mClipboardFormat) == TRUE;
}

// CFrameEditorDropTarget ////////////

void CFrameEditorDropTarget::SetClipBoardFormat(UINT iClipboard)
{
	mClipboardFormat = iClipboard;
}

DROPEFFECT CFrameEditorDropTarget::OnDragEnter(CWnd* pWnd, COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	CFrameWnd *pMainFrame = dynamic_cast<CFrameWnd*>(theApp.m_pMainWnd);

	if (pDataObject->IsDataAvailable(mClipboardFormat)) {
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
