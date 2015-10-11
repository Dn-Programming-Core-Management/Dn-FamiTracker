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

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "GraphEditor.h"
#include "SequenceEditor.h"
#include "Graphics.h"
#include "SoundGen.h"

// CGraphEditor

// The graphical sequence editor

IMPLEMENT_DYNAMIC(CGraphEditor, CWnd)

BEGIN_MESSAGE_MAP(CGraphEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()


CGraphEditor::CGraphEditor(CSequence *pSequence) : 
	m_pBackDC(NULL), 
	m_pBitmap(NULL), 
	m_pSmallFont(NULL),
	m_iHighlightedItem(-1),
	m_iHighlightedValue(0),
	m_bButtonState(false)
{
	m_pSequence = pSequence;
	m_iLastPlayPos = 0;

	m_ptLineStart = m_ptLineEnd = CPoint(0, 0);
}

CGraphEditor::~CGraphEditor()
{
	SAFE_RELEASE(m_pSmallFont);
	SAFE_RELEASE(m_pBackDC);
	SAFE_RELEASE(m_pBitmap);
}

BOOL CGraphEditor::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

BOOL CGraphEditor::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, LPVOID lpParam)
{
	if (CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, lpParam) == -1)
		return -1;

	LOGFONT LogFont;

	const TCHAR *SMALL_FONT_FACE = _T("Verdana");

	m_pSmallFont = new CFont();

	memset(&LogFont, 0, sizeof LOGFONT);
	_tcscpy_s(LogFont.lfFaceName, 32, SMALL_FONT_FACE);
	LogFont.lfHeight = -10;
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	m_pSmallFont->CreateFontIndirect(&LogFont);

	// Calculate the draw areas
	GetClientRect(m_GraphRect);
	GetClientRect(m_BottomRect);
	GetClientRect(m_ClientRect);

	m_GraphRect.top += GRAPH_BOTTOM * 2 + 4;
	m_GraphRect.bottom -= GRAPH_BOTTOM;
	m_GraphRect.left += GRAPH_LEFT;
	m_GraphRect.bottom = (m_GraphRect.bottom / 15) * 15;
	m_BottomRect.left += GRAPH_LEFT;
	m_BottomRect.top = m_GraphRect.bottom;

	m_pParentWnd = pParentWnd;

	RedrawWindow();

	CDC *pDC = GetDC();

	m_pBackDC = new CDC();
	m_pBitmap = new CBitmap();
	m_pBackDC->CreateCompatibleDC(pDC);
	m_pBitmap->CreateCompatibleBitmap(pDC, m_ClientRect.Width(), m_ClientRect.Height());
	m_pBackDC->SelectObject(m_pBitmap);

	ReleaseDC(pDC);

	Initialize();

	CWnd::SetTimer(0, 30, NULL);

	return 0;
}

void CGraphEditor::Initialize()
{
	// Allow extra initialization
}

void CGraphEditor::OnTimer(UINT nIDEvent)
{
	if (m_pSequence) {
		int Pos = theApp.GetSoundGenerator()->GetSequencePlayPos(m_pSequence);
		//int Pos = m_pSequence->GetPlayPos();
		if (Pos != m_iLastPlayPos) {
			m_iCurrentPlayPos = Pos;
			RedrawWindow();
		}
		m_iLastPlayPos = Pos;
	}
}

void CGraphEditor::OnPaint()
{
	CPaintDC dc(this);
}

void CGraphEditor::PaintBuffer(CDC *pBackDC, CDC *pFrontDC)
{
	if (this == GetFocus()) {
		CRect focusRect = m_ClientRect;		
		pBackDC->SetBkColor(0);
		pBackDC->DrawFocusRect(focusRect);
	}

	pFrontDC->BitBlt(0, 0, m_ClientRect.Width(), m_ClientRect.Height(), pBackDC, 0, 0, SRCCOPY);
}

void CGraphEditor::DrawBackground(CDC *pDC, int Lines, bool DrawMarks, int MarkOffset) const
{
	const COLORREF COL_BACKGROUND	= 0x000000;
	const COLORREF COL_BOTTOM		= 0x404040;
	const COLORREF COL_HORZ_BAR		= 0x202020;

	// Fill background
	pDC->FillSolidRect(m_ClientRect, COL_BACKGROUND);
	pDC->FillSolidRect(m_GraphRect.left, m_GraphRect.top, 1, m_GraphRect.bottom, COL_BOTTOM);

	// Fill bottom area
	pDC->FillSolidRect(m_BottomRect, COL_BOTTOM);

	if (m_pSequence) {
		int ItemWidth = GetItemWidth();

		// Draw horizontal bars
		int count = m_pSequence->GetItemCount();
		for (int i = 1; i < count; i += 2) {
			int x = m_GraphRect.left + i * ItemWidth + 1;
			int y = m_GraphRect.top + 1;
			int w = ItemWidth;
			int h = m_GraphRect.Height() - 2;
			pDC->FillSolidRect(x, y, w, h, COL_HORZ_BAR);
		}
	}

	int marker = MarkOffset;

	if (Lines > 0) {
		int StepHeight = m_GraphRect.Height() / Lines;

		// Draw vertical bars
		for (int i = 0; i < Lines + 1; ++i) {
			int x = m_GraphRect.left + 1;
			int y = m_GraphRect.top + StepHeight * i;
			int w = m_GraphRect.Width() - 2;
			int h = 1;
	
			if (DrawMarks) {
				if ((++marker + 1) % 12 == 0) {
					pDC->FillSolidRect(x, y, w, StepHeight, COL_HORZ_BAR);
				}
			}

			pDC->FillSolidRect(x, y, w, h, COLOR_LINES);
		}
	}
}

void CGraphEditor::DrawRange(CDC *pDC, int Max, int Min) const
{
	CFont *pOldFont = pDC->SelectObject(m_pSmallFont);
	CString line;

	pDC->FillSolidRect(m_ClientRect.left, m_GraphRect.top, m_GraphRect.left, m_GraphRect.bottom, 0x00);

	pDC->SetBkMode(OPAQUE);
	pDC->SetTextColor(0xFFFFFF);
	pDC->SetBkColor(pDC->GetPixel(0, 0));	// Ugly but works

	CRect textRect(2, 0, GRAPH_LEFT - 5, 10);
	CRect topRect = textRect, bottomRect = textRect;
	
	topRect.MoveToY(m_GraphRect.top - 3);
	line.Format(_T("%02i"), Max);
	pDC->DrawText(line, topRect, DT_RIGHT);

	bottomRect.MoveToY(m_GraphRect.bottom - 13);
	line.Format(_T("%02i"), Min);
	pDC->DrawText(line, bottomRect, DT_RIGHT);

	pDC->SelectObject(pOldFont);
}

void CGraphEditor::DrawLoopPoint(CDC *pDC, int StepWidth) const
{
	// Draw loop point
	int LoopPoint = m_pSequence->GetLoopPoint();

	CFont *pOldFont = pDC->SelectObject(m_pSmallFont);

	if (LoopPoint > -1) {
		int x = StepWidth * LoopPoint + GRAPH_LEFT + 1;

		GradientBar(pDC, x + 1, m_BottomRect.top, m_BottomRect.right - x, m_BottomRect.Height(), 0x808000, 0x202000);
		pDC->FillSolidRect(x, m_BottomRect.top, 1, m_BottomRect.bottom, 0xF0F000);

		pDC->SetTextColor(0xFFFFFF);
		pDC->SetBkMode(TRANSPARENT);
		pDC->TextOut(x + 4, m_BottomRect.top, _T("Loop"));
	}

	pDC->SelectObject(pOldFont);
}

void CGraphEditor::DrawReleasePoint(CDC *pDC, int StepWidth) const
{
	// Draw loop point
	int ReleasePoint = m_pSequence->GetReleasePoint();

	CFont *pOldFont = pDC->SelectObject(m_pSmallFont);

	if (ReleasePoint > -1) {
		int x = StepWidth * ReleasePoint + GRAPH_LEFT + 1;

		GradientBar(pDC, x + 1, m_BottomRect.top, m_BottomRect.right - x, m_BottomRect.Height(), 0x800080, 0x200020);
		pDC->FillSolidRect(x, m_BottomRect.top, 1, m_BottomRect.bottom, 0xF000F0);

		pDC->SetTextColor(0xFFFFFF);
		pDC->SetBkMode(TRANSPARENT);
		pDC->TextOut(x + 4, m_BottomRect.top, _T("Release"));
	}

	pDC->SelectObject(pOldFont);
}

void CGraphEditor::DrawLoopRelease(CDC *pDC, int StepWidth) const		// // //
{
	int LoopPoint = m_pSequence->GetLoopPoint();
	int ReleasePoint = m_pSequence->GetReleasePoint();

	if (ReleasePoint > LoopPoint) {
		DrawLoopPoint(pDC, StepWidth);
		DrawReleasePoint(pDC, StepWidth);
	}
	else if (ReleasePoint < LoopPoint) {
		DrawReleasePoint(pDC, StepWidth);
		DrawLoopPoint(pDC, StepWidth);
	}
	else if (LoopPoint > -1) { // LoopPoint == ReleasePoint
		CFont *pOldFont = pDC->SelectObject(m_pSmallFont);

		int x = StepWidth * LoopPoint + GRAPH_LEFT + 1;

		GradientBar(pDC, x + 1, m_BottomRect.top, m_BottomRect.right - x, m_BottomRect.Height(), 0x008080, 0x002020);
		pDC->FillSolidRect(x, m_BottomRect.top, 1, m_BottomRect.bottom, 0x00F0F0);

		pDC->SetTextColor(0xFFFFFF);
		pDC->SetBkMode(TRANSPARENT);
		pDC->TextOut(x + 4, m_BottomRect.top, _T("Loop, Release"));

		pDC->SelectObject(pOldFont);
	}
}

void CGraphEditor::DrawLine(CDC *pDC) const
{
	if (m_ptLineStart.x != 0 && m_ptLineStart.y != 0) {
		CPen Pen;
		Pen.CreatePen(1, 3, 0xFFFFFF);
		CPen *pOldPen = pDC->SelectObject(&Pen);
		pDC->MoveTo(m_ptLineStart);
		pDC->LineTo(m_ptLineEnd);
		pDC->SelectObject(pOldPen);
	}
}

template<COLORREF COL_BG1, COLORREF COL_BG2, COLORREF COL_EDGE1, COLORREF COL_EDGE2>
void CGraphEditor::DrawRect(CDC *pDC, int x, int y, int w, int h, bool flat) const
{
	if (h == 0)
		h = 1;

	if (flat) {
		GradientRect(pDC, x + 1, y + 1, w - 2, h - 2, COL_BG1, COL_BG2);
		pDC->Draw3dRect(x, y, w, h, COL_EDGE2, COL_EDGE2);
	}
	else {
		GradientRect(pDC, x + 1, y + 1, w - 2, h - 2, COL_BG1, COL_BG2);
		pDC->Draw3dRect(x, y, w, h, COL_EDGE1, COL_EDGE2);
	}
}

void CGraphEditor::DrawRect(CDC *pDC, int x, int y, int w, int h) const
{
	DrawRect<0xF0F0F0, 0xD0D0D0, 0xFFFFFF, 0xA0A0A0>(pDC, x, y, w, h, false);
}

void CGraphEditor::DrawPlayRect(CDC *pDC, int x, int y, int w, int h) const
{
	DrawRect<0xA0F0A0, 0x86DC86, RGB(198, 242, 198), RGB(106, 223, 106)>(pDC, x, y, w, h, false);
}

void CGraphEditor::DrawCursorRect(CDC *pDC, int x, int y, int w, int h) const
{
	DrawRect<0xFFE0C0, 0xE5C699, RGB(215, 235, 253), RGB(120, 182, 226)>(pDC, x, y, w, h, m_bButtonState);
}

void CGraphEditor::DrawShadowRect(CDC *pDC, int x, int y, int w, int h) const
{	
	if (h == 0)
		h = 1;

	GradientBar(pDC, x, y, w, h, 0x484848, 0x444444);
}

int CGraphEditor::GetItemWidth() const
{
	if (!m_pSequence || m_pSequence->GetItemCount() == 0)
		return 0;

	int Width = m_GraphRect.Width();
	Width = (Width / m_pSequence->GetItemCount()) * m_pSequence->GetItemCount();

	Width = Width / m_pSequence->GetItemCount();

	if (Width > ITEM_MAX_WIDTH)
		Width = ITEM_MAX_WIDTH;

	return Width;
}

void CGraphEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_bButtonState = true;

	SetCapture();
	SetFocus();

	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	if (point.y < m_GraphRect.bottom) {
		m_iEditing = EDIT_POINT;
		ModifyItem(point, true);
		if (point.x > GRAPH_LEFT)
			CursorChanged(point.x - GRAPH_LEFT);
	}
	else if (point.y > m_GraphRect.bottom) {
		m_iEditing = EDIT_LOOP;
		ModifyLoopPoint(point, true);
	}
}

void CGraphEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bButtonState = false;

	ModifyReleased();
	ReleaseCapture();
	Invalidate();
}

void CGraphEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	if (nFlags & MK_LBUTTON) {
		switch (m_iEditing) {
			case EDIT_POINT:
				ModifyItem(point, true);
				break;
			case EDIT_LOOP:
				ModifyLoopPoint(point, true);
				break;
		}
	}
	else if (nFlags & MK_RBUTTON) {
		switch (m_iEditing) {
			case EDIT_LINE:	{
				int PosX = point.x;
				int PosY = point.y;

				if (m_iEditing == EDIT_LINE) {
					m_ptLineEnd = CPoint(PosX, PosY);

					int StartX = (m_ptLineStart.x < m_ptLineEnd.x ? m_ptLineStart.x : m_ptLineEnd.x);
					int StartY = (m_ptLineStart.x < m_ptLineEnd.x ? m_ptLineStart.y : m_ptLineEnd.y);
					int EndX = (m_ptLineStart.x > m_ptLineEnd.x ? m_ptLineStart.x : m_ptLineEnd.x);
					int EndY = (m_ptLineStart.x > m_ptLineEnd.x ? m_ptLineStart.y : m_ptLineEnd.y);

					float DeltaY = float(EndY - StartY) / float((EndX - StartX) + 1);
					float fY = float(StartY);

					for (int x = StartX; x < EndX; x++) {
						ModifyItem(CPoint(x, (int)fY), false);
						fY += DeltaY;
					}

					RedrawWindow();
					m_pParentWnd->PostMessage(WM_SEQUENCE_CHANGED, 1);
				}
				}
				break;
			case EDIT_RELEASE:
				ModifyReleasePoint(point, true);
				break;
		}
	}
	else {
		// No button
		HighlightItem(point);
	}

	// Notify parent
	if (m_pSequence->GetItemCount() > 0 && point.x > GRAPH_LEFT) {
		CursorChanged(point.x - GRAPH_LEFT);
	}
	else
		m_pParentWnd->PostMessage(WM_CURSOR_CHANGE, 0, 0);
}

void CGraphEditor::OnRButtonDown(UINT nFlags, CPoint point)
{
	SetCapture();

	if (point.y < m_GraphRect.bottom) {
		m_ptLineStart = m_ptLineEnd = point;
		m_iEditing = EDIT_LINE;
	}
	else if (point.y > m_GraphRect.bottom) {
		m_iEditing = EDIT_RELEASE;
		ModifyReleasePoint(point, true);
	}

	CWnd::OnRButtonDown(nFlags, point);
}

void CGraphEditor::OnRButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();

	m_ptLineStart = CPoint(0, 0);
	m_iEditing = EDIT_NONE;
	RedrawWindow();
	CWnd::OnRButtonUp(nFlags, point);
}

void CGraphEditor::ModifyReleased()
{
}

void CGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	if (Redraw) {
		RedrawWindow(NULL);
		m_pParentWnd->PostMessage(WM_SEQUENCE_CHANGED, 1);
	}
}

void CGraphEditor::ModifyLoopPoint(CPoint point, bool Redraw)
{
	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	int ItemWidth = GetItemWidth();
	int LoopPoint = (point.x - GRAPH_LEFT + (ItemWidth / 2)) / ItemWidth;

	// Disable loop point by dragging it to far right
	if (LoopPoint >= (int)m_pSequence->GetItemCount())
		LoopPoint = -1;

	m_pSequence->SetLoopPoint(LoopPoint);

	if (Redraw) {
		RedrawWindow(NULL);
		m_pParentWnd->PostMessage(WM_SEQUENCE_CHANGED, 1);
	}
}

void CGraphEditor::ModifyReleasePoint(CPoint point, bool Redraw)
{
	if (!m_pSequence || !m_pSequence->GetItemCount())
		return;

	int ItemWidth = GetItemWidth();
	int ReleasePoint = (point.x - GRAPH_LEFT + (ItemWidth / 2)) / ItemWidth;

	// Disable loop point by dragging it to far right
	if (ReleasePoint >= (int)m_pSequence->GetItemCount())
		ReleasePoint = -1;

	m_pSequence->SetReleasePoint(ReleasePoint);

	if (Redraw) {
		RedrawWindow(NULL);
		m_pParentWnd->PostMessage(WM_SEQUENCE_CHANGED, 1);
	}
}

void CGraphEditor::CursorChanged(int x)
{
	if (m_pSequence->GetItemCount() == 0)
		return;

	int Width = GetItemWidth();

	if (Width == 0)
		return;

	int Pos = x / Width;

	if (Pos >= (signed)m_pSequence->GetItemCount())
		return;

	int Value = m_pSequence->GetItem(Pos);
	m_pParentWnd->PostMessage(WM_CURSOR_CHANGE, Pos, Value);
}

bool CGraphEditor::IsEditLine() const
{
	return m_iEditing == EDIT_LINE;
}

void CGraphEditor::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	RedrawWindow();
}

void CGraphEditor::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	RedrawWindow();
}

// Bar graph editor (volume and duty setting)

void CBarGraphEditor::OnPaint()
{
	ASSERT(m_pBackDC != NULL);

	CPaintDC dc(this);
	
	DrawBackground(m_pBackDC, m_iLevels, false, 0);
	DrawRange(m_pBackDC, m_iLevels, 0);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = m_GraphRect.Height() / m_iLevels;

	if (m_iHighlightedValue > 0 && m_iHighlightedItem >= 0 && m_iHighlightedItem < Count) {
		int x = m_GraphRect.left + m_iHighlightedItem * StepWidth + 1;
		int y = m_GraphRect.top + StepHeight * (m_iLevels - m_iHighlightedValue);
		int w = StepWidth;
		int h = StepHeight * m_iHighlightedValue;
		DrawShadowRect(m_pBackDC, x, y, w, h);
	}

	// Draw items
	for (int i = 0; i < Count; i++) {
		int x = m_GraphRect.left + i * StepWidth + 1;
		int y = m_GraphRect.top + StepHeight * (m_iLevels - m_pSequence->GetItem(i));
		int w = StepWidth;
		int h = StepHeight * m_pSequence->GetItem(i);

		if (m_iCurrentPlayPos == i)
			DrawPlayRect(m_pBackDC, x, y, w, h);
		else if ((m_iHighlightedItem == i) && (m_pSequence->GetItem(i) >= m_iHighlightedValue) && !IsEditLine())
			DrawCursorRect(m_pBackDC, x, y, w, h);
		else
			DrawRect(m_pBackDC, x, y, w, h);
	}
	
	DrawLoopRelease(m_pBackDC, StepWidth);		// // //
	DrawLine(m_pBackDC);

	PaintBuffer(m_pBackDC, &dc);
}

void CBarGraphEditor::HighlightItem(CPoint point)
{
	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = m_iLevels - (((point.y - m_GraphRect.top) + (ItemHeight / 2)) / ItemHeight);
	int LastItem = m_iHighlightedItem;
	int LastValue = m_iHighlightedValue;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	if (m_GraphRect.PtInRect(point) == 0 || unsigned(ItemIndex) >= m_pSequence->GetItemCount()) {
		m_iHighlightedItem = -1;
		m_iHighlightedValue = 0;
	}
	
	if (LastItem != m_iHighlightedItem || LastValue != m_iHighlightedValue)
		RedrawWindow(NULL);
}

void CBarGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = m_iLevels - (((point.y - m_GraphRect.top) + (ItemHeight / 2)) / ItemHeight);

	if (ItemValue < 0)
		ItemValue = 0;
	if (ItemValue > m_iLevels)
		ItemValue = m_iLevels;

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

int CBarGraphEditor::GetItemHeight() const
{
	return m_GraphRect.Height() / m_iLevels;
}

// Arpeggio graph editor

IMPLEMENT_DYNAMIC(CArpeggioGraphEditor, CGraphEditor)

BEGIN_MESSAGE_MAP(CArpeggioGraphEditor, CGraphEditor)
	ON_WM_VSCROLL()
	ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()

CArpeggioGraphEditor::CArpeggioGraphEditor(CSequence *pSequence) : 
	CGraphEditor(pSequence), 
	m_iScrollOffset(0), 
	m_pScrollBar(NULL)
{
}

CArpeggioGraphEditor::~CArpeggioGraphEditor()
{
	SAFE_RELEASE(m_pScrollBar);
}

void CArpeggioGraphEditor::Initialize()
{
	// Setup scrollbar
	const int SCROLLBAR_WIDTH = 18;
	SCROLLINFO info;

	m_pScrollBar = new CScrollBar();

	m_GraphRect.right -= SCROLLBAR_WIDTH;
	m_pScrollBar->Create(SBS_VERT | SBS_LEFTALIGN | WS_CHILD | WS_VISIBLE, CRect(m_GraphRect.right, m_GraphRect.top, m_GraphRect.right + SCROLLBAR_WIDTH, m_GraphRect.bottom), this, 0);

	info.cbSize = sizeof(SCROLLINFO);
	info.fMask = SIF_ALL;

	switch (m_pSequence->GetSetting()) {		// // //
	case SETTING_ARP_FIXED:
		info.nMax = 84;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = 84;
		m_iScrollMax = 84; break;
	case SETTING_ARP_SCHEME:
		info.nMax = 72;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = 36;
		m_iScrollMax = 36; break;
	default:
		info.nMax = 192;
		info.nMin = 0;
		info.nPage = 10;
		info.nPos = 96;
		m_iScrollMax = 96;
	}

	if (m_pSequence != NULL && m_pSequence->GetItemCount() > 0) {
		m_iScrollOffset = m_pSequence->GetItem(0);
		if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME){			// // //
			m_iScrollOffset = (m_iScrollOffset + 0x80) % 0x40;
			m_iScrollOffset -= 0x40 * (m_iScrollOffset % 0x40 > 0x24);
		}

		info.nPos = m_iScrollMax - m_iScrollOffset;		// // //
		if (info.nPos < info.nMin)
			info.nPos = info.nMin;
		if (info.nPos > info.nMax)
			info.nPos = info.nMax;

	}
	else
		m_iScrollOffset = 0;

	m_pScrollBar->SetScrollInfo(&info);
	m_pScrollBar->ShowScrollBar(TRUE);
	m_pScrollBar->EnableScrollBar(ESB_ENABLE_BOTH);

	m_ClientRect.right -= SCROLLBAR_WIDTH;

	CGraphEditor::Initialize();
}

void CArpeggioGraphEditor::ChangeSetting()
{
	// Reset the scrollbar
	SCROLLINFO info;

	switch (m_pSequence->GetSetting()) {		// // //
	case SETTING_ARP_FIXED:
		info.nMax = 84;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = 84;
		m_iScrollMax = 84; break;
	case SETTING_ARP_SCHEME:
		info.nMax = 72;
		info.nMin = 0;
		info.nPage = 1;
		info.nPos = 36;
		m_iScrollMax = 36; break;
	default:
		info.nMax = 192;
		info.nMin = 0;
		info.nPage = 10;
		info.nPos = 96;
		m_iScrollMax = 96;
	}

	m_iScrollOffset = 0;
	m_pScrollBar->SetScrollInfo(&info);
}

CString CArpeggioGraphEditor::GetNoteString(int Value)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

	int Octave = Value / NOTE_RANGE;
	int Note = Value % NOTE_RANGE;

	CString line;
	line.Format(_T("%c%c%c"), NOTES_A[Note], NOTES_B[Note], NOTES_C[Octave]);

	return line;
}

CString CArpeggioGraphEditor::GetArpSchemeString(int Value)
{
	CString line = _T("");
	int trsp = Value & 0x3F;

	if (!trsp) {
		static const CString SINGLE[] = {_T("0"), _T("x"), _T("y"), _T("-y")};
		return SINGLE[(Value & 0xC0) >> 6];
	}

	line.Format(_T("%i"), trsp > 0x24 ? trsp - 64 : trsp);
	switch (Value & 0xC0) {
	case ARPSCHEME_MODE_X: line.Append(_T("+x")); break;
	case ARPSCHEME_MODE_Y: line.Append(_T("+y")); break;
	case ARPSCHEME_MODE_NEG_Y: line.Append(_T("-y")); break;
	}

	return line;
}

void CArpeggioGraphEditor::DrawRange(CDC *pDC, int Max, int Min)
{
	const char NOTES_A[] = {'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'};
	const char NOTES_B[] = {'-', '#', '-', '#', '-', '-', '#', '-', '#', '-', '#', '-'};
	const char NOTES_C[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

	if (m_pSequence->GetSetting() != SETTING_ARP_FIXED) {
		// Absolute, relative
		CGraphEditor::DrawRange(pDC, Max, Min);
	}
	else {
		// Fixed
		pDC->FillSolidRect(m_ClientRect.left, m_GraphRect.top, m_GraphRect.left, m_GraphRect.bottom, 0x00);

		CFont *pOldFont = pDC->SelectObject(m_pSmallFont);

		pDC->SetTextColor(0xFFFFFF);
		pDC->SetBkColor(0);

		// Top
		int NoteValue = m_iScrollOffset + 20;
		int Octave = NoteValue / NOTE_RANGE;
		int Note = NoteValue % NOTE_RANGE;

		CString line;
		line = GetNoteString(NoteValue);
		pDC->TextOut(2, m_GraphRect.top - 3, line);

		// Bottom
		NoteValue = m_iScrollOffset;
		Octave = NoteValue / NOTE_RANGE;
		Note = NoteValue % NOTE_RANGE;

		line.Format(_T("%c%c%c"), NOTES_A[Note], NOTES_B[Note], NOTES_C[Octave]);
		pDC->TextOut(2, m_GraphRect.bottom - 13, line);
		pDC->SelectObject(pOldFont);
	}
}

void CArpeggioGraphEditor::OnPaint()
{
	ASSERT(m_pBackDC != NULL);

	CPaintDC dc(this);

	DrawBackground(m_pBackDC, ITEMS, true,		// // //
		m_pSequence->GetSetting() == SETTING_ARP_FIXED ? 2 - m_iScrollOffset : -m_iScrollOffset);
	DrawRange(m_pBackDC, m_iScrollOffset + 10, m_iScrollOffset - 10);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = m_GraphRect.Height() / ITEMS;

	// One last line
	m_pBackDC->FillSolidRect(m_GraphRect.left + 1, m_GraphRect.top + 20 * StepHeight, m_GraphRect.Width() - 2, 1, COLOR_LINES);

	if (m_iHighlightedItem >= 0 && m_iHighlightedItem < Count) {
		int item;			// // //
		if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {
			int value = (m_iHighlightedValue + 0x100) % 0x40;
			if (value > 36) value -= 0x40;
			item = (ITEMS / 2) - value + m_iScrollOffset;
		}
		else
			item = (ITEMS / 2) - m_iHighlightedValue + m_iScrollOffset;
		if (m_pSequence->GetSetting() == SETTING_ARP_FIXED)
			item += (ITEMS / 2);
		if (item >= 0 && item <= 20) {
			int x = m_GraphRect.left + m_iHighlightedItem * StepWidth + 1;
			int y = m_GraphRect.top + StepHeight * item + 1;
			int w = StepWidth;
			int h = StepHeight - 1;
			DrawShadowRect(m_pBackDC, x, y, w, h);
		}
	}

	// Draw items
	CFont *pOldFont = m_pBackDC->SelectObject(m_pSmallFont);		// // //
	m_pBackDC->SetTextAlign(TA_CENTER);
	m_pBackDC->SetTextColor(0xFFFFFF);
	m_pBackDC->SetBkMode(TRANSPARENT);
	for (int i = 0; i < Count; i++) {
		int item;			// // //
		if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {
			int value = (m_pSequence->GetItem(i) + 0x100) % 0x40;
			if (value > 36) value -= 0x40;
			item = (ITEMS / 2) - value + m_iScrollOffset;
		}
		else
			item = (ITEMS / 2) - m_pSequence->GetItem(i) + m_iScrollOffset;
		if (m_pSequence->GetSetting() == SETTING_ARP_FIXED)
			item += (ITEMS / 2);
		if (item >= 0 && item <= 20) {
			int x = m_GraphRect.left + i * StepWidth + 1;
			int y = m_GraphRect.top + StepHeight * item;
			int w = StepWidth;
			int h = StepHeight;

			if (m_iCurrentPlayPos == i)
				DrawPlayRect(m_pBackDC, x, y, w, h);
			else if (m_iHighlightedItem == i && (m_pSequence->GetItem(i) == m_iHighlightedValue) && !IsEditLine())
				DrawCursorRect(m_pBackDC, x, y, w, h);
			else
				DrawRect(m_pBackDC, x, y, w, h);
			
			if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {
				static const CString HEAD[] = {_T(""), _T("x"), _T("y"), _T("-y")};
				m_pBackDC->TextOut(x + w / 2, y - 2 * h, HEAD[(m_pSequence->GetItem(i) & 0xFF) >> 6]);
			}
		}
	}
	m_pBackDC->SetTextAlign(TA_LEFT);
	m_pBackDC->SelectObject(pOldFont);
	
	DrawLoopRelease(m_pBackDC, StepWidth);		// // //
	DrawLine(m_pBackDC);

	PaintBuffer(m_pBackDC, &dc);
}

void CArpeggioGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	int ItemWidth = GetItemWidth();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = GetItemValue(point.y);

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;
	
	if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME) {		// // //
		int value = m_pSequence->GetItem(ItemIndex);
		if (value < 0) value += 0x100;
		if (::GetKeyState(VK_NUMPAD0) & 0x80)
			value = 0;
		else if (::GetKeyState(VK_NUMPAD1) & 0x80)
			value = ARPSCHEME_MODE_X;
		else if (::GetKeyState(VK_NUMPAD2) & 0x80)
			value = ARPSCHEME_MODE_Y;
		else if (::GetKeyState(VK_NUMPAD3) & 0x80)
			value = ARPSCHEME_MODE_NEG_Y;
		ItemValue = (ItemValue & 0x3F) | (value & 0xC0);
	}
	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

void CArpeggioGraphEditor::HighlightItem(CPoint point)
{
	int ItemWidth = GetItemWidth();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = GetItemValue(point.y);
	int LastItem = m_iHighlightedItem;
	int LastValue = m_iHighlightedValue;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	if (m_GraphRect.PtInRect(point) == 0 || unsigned(ItemIndex) >= m_pSequence->GetItemCount()) { 
		m_iHighlightedItem = -1;
		m_iHighlightedValue = 0;
	}

	if (m_iHighlightedItem != LastItem || m_iHighlightedValue != LastValue) {
		RedrawWindow(NULL);
	}
}

int CArpeggioGraphEditor::GetItemValue(int pos)
{
	int ItemValue;
	int ItemHeight = GetItemHeight();

	switch (m_pSequence->GetSetting()) {		// // //
	case SETTING_ARP_FIXED:
		ItemValue = ITEMS - ((pos - m_GraphRect.top) / ItemHeight) + m_iScrollOffset;
		if (ItemValue < 0)
			ItemValue = 0;
		if (ItemValue > 95)
			ItemValue = 95;
		break;
	case SETTING_ARP_SCHEME:
		ItemValue = (ITEMS / 2) - ((pos - m_GraphRect.top) / ItemHeight) + m_iScrollOffset;
		if (ItemValue < -27)
			ItemValue = -27;
		if (ItemValue > 36)
			ItemValue = 36;
		break;
	default:
		ItemValue = (ITEMS / 2) - ((pos - m_GraphRect.top) / ItemHeight) + m_iScrollOffset;
		if (ItemValue < -96)
			ItemValue = -96;
		if (ItemValue > 96)
			ItemValue = 96;
	}

	return ItemValue;
}

int CArpeggioGraphEditor::GetItemHeight() const
{
	return m_GraphRect.Height() / ITEMS;
}

void CArpeggioGraphEditor::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
		case SB_LINEDOWN: 
			m_iScrollOffset--;
			break;
		case SB_LINEUP:
			m_iScrollOffset++;
			break;
		case SB_PAGEDOWN:
			m_iScrollOffset -= 10;
			break;
		case SB_PAGEUP:
			m_iScrollOffset += 10;
			break;
		case SB_TOP:
			m_iScrollOffset = m_iScrollMax;
			break;
		case SB_BOTTOM:	
			m_iScrollOffset = (m_pSequence->GetSetting() != SETTING_ARP_FIXED) ? -m_iScrollMax : 0;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_iScrollOffset = m_iScrollMax - (int)nPos;
			break;
		case SB_ENDSCROLL:
		default:
			return;
	}

	if (m_iScrollOffset > m_iScrollMax)
		m_iScrollOffset = m_iScrollMax;

	if (m_pSequence->GetSetting() != SETTING_ARP_FIXED) {
		if (m_iScrollOffset < -m_iScrollMax)
			m_iScrollOffset = -m_iScrollMax;
	}
	else {
		if (m_iScrollOffset < 0)
			m_iScrollOffset = 0;
	}

	pScrollBar->SetScrollPos(m_iScrollMax - m_iScrollOffset);

	RedrawWindow();

	CWnd::OnVScroll(nSBCode, nPos, pScrollBar);
}

BOOL CArpeggioGraphEditor::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (zDelta < 0)
		OnVScroll(SB_LINEDOWN, 0, m_pScrollBar);
	else
		OnVScroll(SB_LINEUP, 0, m_pScrollBar);
	return CWnd::OnMouseWheel(nFlags, zDelta, pt);
}

// Pitch graph editor

void CPitchGraphEditor::OnPaint()
{
	ASSERT(m_pBackDC != NULL);

	CPaintDC dc(this);

	DrawBackground(m_pBackDC, 0, false, 0);
	DrawRange(m_pBackDC, 127, -128);

	m_pBackDC->FillSolidRect(m_GraphRect.left, m_GraphRect.top + m_GraphRect.Height() / 2, m_GraphRect.Width(), 1, COLOR_LINES);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = m_GraphRect.Height() / ITEMS;

	// One last line
	m_pBackDC->FillSolidRect(m_GraphRect.left + 1, m_GraphRect.top + 20 * StepHeight, m_GraphRect.Width() - 2, 1, COLOR_LINES);

	if (m_iHighlightedItem != -1) {
		int x = m_GraphRect.left + m_iHighlightedItem * StepWidth + 1;
		int y = m_GraphRect.top + m_GraphRect.Height() / 2;
		int w = StepWidth;
		int h = -(m_iHighlightedValue * m_GraphRect.Height()) / 255 ;
		DrawShadowRect(m_pBackDC, x, y, w, h);
	}

	int Value = m_pSequence->GetItem(m_iHighlightedItem);
	bool bHighlight = !IsEditLine();
	if (m_iHighlightedValue > 0) {
		if (m_iHighlightedValue > Value || Value < 0) {
			bHighlight = false;
		}
	}
	else {
		if (m_iHighlightedValue < Value || Value > 0) {
			bHighlight = false;
		}
	}

	// Draw items
	for (int i = 0; i < Count; i++) {
		int item = m_pSequence->GetItem(i);
		int x = m_GraphRect.left + i * StepWidth + 1;
		int y = m_GraphRect.top + m_GraphRect.Height() / 2;
		int w = StepWidth;
		int h = -(item * m_GraphRect.Height()) / 255 ;
		if (h < 0) {
			y += h;
			h = -h;
		}
		if (m_iCurrentPlayPos == i)
			DrawPlayRect(m_pBackDC, x, y, w, h);
		else if (m_iHighlightedItem == i && bHighlight)
			DrawCursorRect(m_pBackDC, x, y, w, h);
		else
			DrawRect(m_pBackDC, x, y, w, h);
	}
	
	DrawLoopRelease(m_pBackDC, StepWidth);		// // //
	DrawLine(m_pBackDC);

	PaintBuffer(m_pBackDC, &dc);
}

int CPitchGraphEditor::GetItemHeight() const
{
	return m_GraphRect.Height() / ITEMS;
}

void CPitchGraphEditor::ModifyItem(CPoint point, bool Redraw)
{
	int MouseY = (point.y - m_GraphRect.top) - (m_GraphRect.Height() / 2);
	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = -(MouseY * 255) / m_GraphRect.Height();

	if (ItemValue < -128)
		ItemValue = -128;
	if (ItemValue > 127)
		ItemValue = 127;

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

void CPitchGraphEditor::HighlightItem(CPoint point)
{
	int MouseY = (point.y - m_GraphRect.top) - (m_GraphRect.Height() / 2);
	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();
	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;
	int ItemValue = -(MouseY * 255) / m_GraphRect.Height();
	int LastItem = m_iHighlightedItem;
	int LastValue = m_iHighlightedValue;

	if (ItemValue < -128)
		ItemValue = -128;
	if (ItemValue > 127)
		ItemValue = 127;

	m_iHighlightedItem = ItemIndex;
	m_iHighlightedValue = ItemValue;

	if (m_GraphRect.PtInRect(point) == 0 || ItemIndex < 0 || unsigned(ItemIndex) >= m_pSequence->GetItemCount()) {
		m_iHighlightedItem = -1;
		m_iHighlightedValue = 0;
	}
	
	if (m_iHighlightedItem != LastItem || m_iHighlightedValue != LastValue) {
		RedrawWindow(NULL);
	}
}

// Sunsoft noise editor

void CNoiseEditor::OnPaint()
{
	ASSERT(m_pBackDC != NULL);

	CPaintDC dc(this);
	
	DrawBackground(m_pBackDC, m_iItems, false, 0);
	DrawRange(m_pBackDC, m_iItems, 0);

	// Return now if no sequence is selected
	if (!m_pSequence) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	// Draw items
	int Count = m_pSequence->GetItemCount();

	if (!Count) {
		PaintBuffer(m_pBackDC, &dc);
		return;
	}

	int StepWidth = GetItemWidth();
	int StepHeight = m_GraphRect.Height() / m_iItems;

	// Draw items
	for (int i = 0; i < Count; i++) {
		// Draw noise frequency
		int item = m_pSequence->GetItem(i) & 0x1F;
		int x = m_GraphRect.left + i * StepWidth + 1;
		int y = m_GraphRect.top + StepHeight * (m_iItems - item);
		int w = StepWidth;
		int h = StepHeight;//* item;
		
		if (m_iCurrentPlayPos == i)
			DrawPlayRect(m_pBackDC, x, y, w, h);
		else
			DrawRect(m_pBackDC, x, y, w, h);

		// Draw switches
		item = m_pSequence->GetItem(i);

		int Offset = h * 36 - 1;

		if (item & S5B_MODE_SQUARE) {
			static const COLORREF BUTTON_COL = COMBINE(0, 160, 160);
			int y = Offset;
			int h = 9;
			m_pBackDC->FillSolidRect(x, y, w, h, BUTTON_COL);
			m_pBackDC->Draw3dRect(x, y, w, h, BLEND(BUTTON_COL, 0xFFFFFF, 80), BLEND(BUTTON_COL, 0x000000, 80));
		}
		else {
			static const COLORREF BUTTON_COL = COMBINE(50, 50, 50);
			int y = Offset;
			int h = 9;
			m_pBackDC->FillSolidRect(x, y, w, h, BUTTON_COL);
			m_pBackDC->Draw3dRect(x, y, w, h, BLEND(BUTTON_COL, 0xFFFFFF, 80), BLEND(BUTTON_COL, 0x000000, 80));
		}

		if (item & S5B_MODE_NOISE) {
			static const COLORREF BUTTON_COL = COMBINE(160, 0, 160);
			int y = Offset + 11;
			int h = 9;
			m_pBackDC->FillSolidRect(x, y, w, h, BUTTON_COL);
			m_pBackDC->Draw3dRect(x, y, w, h, BLEND(BUTTON_COL, 0xFFFFFF, 80), BLEND(BUTTON_COL, 0x000000, 80));
		}
		else {
			static const COLORREF BUTTON_COL = COMBINE(50, 50, 50);
			int y = Offset + 11;
			int h = 9;
			m_pBackDC->FillSolidRect(x, y, w, h, BUTTON_COL);
			m_pBackDC->Draw3dRect(x, y, w, h, BLEND(BUTTON_COL, 0xFFFFFF, 80), BLEND(BUTTON_COL, 0x000000, 80));
		}
	}
	
	DrawLoopRelease(m_pBackDC, StepWidth);		// // //
	DrawLine(m_pBackDC);

	PaintBuffer(m_pBackDC, &dc);
}

void CNoiseEditor::ModifyItem(CPoint point, bool Redraw)
{
	int ItemValue;
	int ItemWidth = GetItemWidth();
	int ItemHeight = GetItemHeight();

	int ItemIndex = (point.x - GRAPH_LEFT) / ItemWidth;

	int Offset = 36 * ItemHeight - 1;

	if (point.y >= Offset) {

		if (m_iLastIndex == ItemIndex)
			return;

		m_iLastIndex = ItemIndex;

		if (point.y >= Offset && point.y < Offset + 10) {
			// Square
			ItemValue = m_pSequence->GetItem(ItemIndex) ^ S5B_MODE_SQUARE;
		}
		else if (point.y >= Offset + 11 && point.y < Offset + 21) {
			// Noise
			ItemValue = m_pSequence->GetItem(ItemIndex) ^ S5B_MODE_NOISE;
		}
		else
			return;
	}
	else {

		ItemValue = m_iItems - (((point.y - m_GraphRect.top) + (ItemHeight / 2)) / ItemHeight);

		if (ItemValue < 0)
			ItemValue = 0;
		if (ItemValue > m_iItems)
			ItemValue = m_iItems;

		ItemValue |= m_pSequence->GetItem(ItemIndex) & 0xC0;
	}

	if (ItemIndex < 0 || ItemIndex >= (int)m_pSequence->GetItemCount())
		return;

	m_pSequence->SetItem(ItemIndex, ItemValue);

	CGraphEditor::ModifyItem(point, Redraw);
}

void CNoiseEditor::HighlightItem(CPoint point)
{
	// TODO
}

int CNoiseEditor::GetItemHeight() const
{
	return m_GraphRect.Height() / m_iItems;
}

void CNoiseEditor::ModifyReleased()
{
	m_iLastIndex = -1;
}
