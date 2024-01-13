/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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

#include "stdafx.h"
#include "../resource.h"
#include "DSample.h"
#include "SampleEditorView.h"
#include "SampleEditorDlg.h"

// CSampleEditorView control

const float CSampleEditorView::MAX_FACTOR = 1.0f;		// // // 1x
const float CSampleEditorView::MIN_FACTOR = 0.01f;		// // // 100x

IMPLEMENT_DYNAMIC(CSampleEditorView, CStatic)

BEGIN_MESSAGE_MAP(CSampleEditorView, CStatic)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

CSampleEditorView::CSampleEditorView() : 
	m_iSelStart(-1), 
	m_iSelEnd(-1), 
	m_iStartCursor(0),
	m_pSamples(NULL),
	m_bClicked(false),
	m_pScrollBar(NULL),
	m_fZoom(1.0f),
	m_iViewStart(0),
	m_iViewEnd(0)
{
	m_pSolidPen = new CPen(PS_SOLID, 1, (COLORREF)0x00);
	m_pDashedPen = new CPen(PS_DASH, 1, (COLORREF)0x00);
	m_pGrayDashedPen = new CPen(PS_DASHDOT, 1, (COLORREF)0xF0F0F0);
	m_pDarkGrayDashedPen = new CPen(PS_DASHDOT, 1, (COLORREF)0xE0E0E0);
}

CSampleEditorView::~CSampleEditorView()
{
	SAFE_RELEASE_ARRAY(m_pSamples);

	SAFE_RELEASE(m_pSolidPen);
	SAFE_RELEASE(m_pDashedPen);
	SAFE_RELEASE(m_pGrayDashedPen);
	SAFE_RELEASE(m_pDarkGrayDashedPen);
	SAFE_RELEASE(m_pScrollBar);
}

void CSampleEditorView::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect sbRect;
	m_pScrollBar->GetClientRect(&sbRect);
	int ScrollBarHeight = sbRect.bottom - sbRect.top;

	GetClientRect(&m_clientRect);
	m_clientRect.bottom -= ScrollBarHeight;

	int Width = m_clientRect.right - ::GetSystemMetrics(SM_CXEDGE);
	int Height = m_clientRect.bottom - ::GetSystemMetrics(SM_CYEDGE);

	if (m_dcCopy.m_hDC != NULL)
		m_dcCopy.DeleteDC();

	if (m_bmpCopy.m_hObject != NULL)
		m_bmpCopy.DeleteObject();

	m_bmpCopy.CreateCompatibleBitmap(&dc, m_clientRect.Width(), m_clientRect.Height());
	m_dcCopy.CreateCompatibleDC(&dc);
	m_dcCopy.SelectObject(&m_bmpCopy);
	m_dcCopy.FillSolidRect(m_clientRect, 0xFFFFFF);
	m_dcCopy.SetViewportOrg(1, 1);

	if (m_iSize == 0) {
		m_dcCopy.TextOut(10, 10, CString(_T("No sample")));
		dc.BitBlt(0, 0, m_clientRect.Width(), m_clientRect.Height(), &m_dcCopy, 0, 0, SRCCOPY);
		return;
	}

	// Size of visible area
	int Size = m_iViewEnd - m_iViewStart;

	if (Size == 0)
		Size = 1;

	double Step = double(Size) / double(Width);	// Samples / pixel

	m_dSampleStep = Step;
	m_iBlockSize = (Width * (8 * 16)) / Size;

	CPen *oldPen = m_dcCopy.SelectObject(m_pSolidPen);

	// Block markers
	m_dcCopy.SelectObject(m_pGrayDashedPen);
	m_dcCopy.SetBkMode(TRANSPARENT);
	int StartBlock = (m_iViewStart / (8 * 16));
	int Blocks = (Size / (8 * 16));
	int Offset = m_iViewStart % (8 * 16);
	if (Blocks < (Width / 2)) {
		for (int i = 1; i < Blocks + 2; ++i) {
			int x = int((i * 128 - Offset) / m_dSampleStep) - 1;
			if ((i + StartBlock) % 4 == 0)
				m_dcCopy.SelectObject(m_pDarkGrayDashedPen);
			else
				m_dcCopy.SelectObject(m_pGrayDashedPen);
			m_dcCopy.MoveTo(x, 0);
			m_dcCopy.LineTo(x, Height);
		}
	}

	// Selection, each step is 16 bytes, or 128 samples
	if (m_iSelStart != m_iSelEnd) {
		const COLORREF SEL_COLOR = 0xFF80A0;
		int Offset = int(m_iViewStart / m_dSampleStep);
		int StartPixel = GetPixel(m_iSelStart) - Offset;
		int EndPixel = GetPixel(m_iSelEnd) - Offset;
		m_dcCopy.FillSolidRect(StartPixel, 0, EndPixel - StartPixel, Height, SEL_COLOR);
	}

	// Draw the sample
	int y = (m_pSamples[m_iViewStart] * Height) / 127;
	m_dcCopy.MoveTo(0, y);
	m_dcCopy.SelectObject(m_pSolidPen);

	float Step2 = float(Width) / float(Size);
	float Pos = 0.0f;
	int LastPos = -1;
	int Steps = 0;
	int Min = 255, Max = 0;
	int LastValue = y;

	for (int i = m_iViewStart + 1; i < m_iViewEnd; ++i) {
		if (m_pSamples[i] < Min)
			Min = m_pSamples[i];
		if (m_pSamples[i] > Max)
			Max = m_pSamples[i];

		Pos += Step2;
		++Steps;

		int x = int(Pos);
		if (x != LastPos) {
			if (Steps == 1) {
				int y = (Min * Height) / 127;
				m_dcCopy.LineTo(x, LastValue);
				m_dcCopy.LineTo(x, y);
				LastValue = y;
			}
			else {
				m_dcCopy.LineTo(x, (Min * Height) / 127);
				m_dcCopy.LineTo(x, (Max * Height) / 127);
			}
			Min = 255;
			Max = 0;
			Steps = 0;
			LastPos = x;
		}
	}

	y = (m_pSamples[m_iViewEnd - 1] * Height) / 127;
	m_dcCopy.LineTo(Width - 1, y);

	m_dcCopy.SetViewportOrg(0, 0);
	m_dcCopy.SelectObject(oldPen);

	dc.BitBlt(0, 0, m_clientRect.Width(), m_clientRect.Height(), &m_dcCopy, 0, 0, SRCCOPY);
}

BOOL CSampleEditorView::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CSampleEditorView::OnMouseMove(UINT nFlags, CPoint point)
{
	double Sample = double(point.x) * m_dSampleStep;
	int Offset = int(Sample / (8.0 * 64.0));
	int Pos = int(Sample / 8.0);		// // //

	if (!m_iSize)
		return;

	if (point.y > m_clientRect.bottom)
		return;

	if (nFlags & MK_LBUTTON && m_iSelStart != -1) {
		int Block = GetBlock(point.x + (m_iBlockSize / 2) + int(m_iViewStart / m_dSampleStep));
		m_iSelEnd = Block;
		Invalidate();
		RedrawWindow();
		static_cast<CSampleEditorDlg*>(GetParent())->SelectionChanged();
	}

	CString Text, num1, num2;		// // //
	num1.Format(_T("0x%02X"), Offset);
	num2.Format(_T("%d"), Pos);
	AfxFormatString2(Text, ID_INDICATOR_DPCM_SEGMENT, num1, num2);
	static_cast<CSampleEditorDlg*>(GetParent())->UpdateStatus(0, Text);

	CStatic::OnMouseMove(nFlags, point);
}

void CSampleEditorView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (!m_iSize)
		return;

	if (point.y > m_clientRect.bottom)
		return;

	int Block = GetBlock(point.x + int(m_iViewStart / m_dSampleStep));
	m_iSelStart = Block;
	m_iSelEnd = Block;

	Invalidate();
	RedrawWindow();
	m_bClicked = true;
	CStatic::OnLButtonDown(nFlags, point);
}

void CSampleEditorView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// Sets the start cursor

	if (!m_iSize)
		return;

	if (!m_bClicked)
		return;

	m_bClicked = false;

	double Sample = double(point.x + int(m_iViewStart / m_dSampleStep)) * m_dSampleStep;
	int Offset = int(Sample / (8.0 * 64.0));

	if (m_iSelEnd == m_iSelStart) {
		m_iStartCursor = Offset;
		m_iSelStart = m_iSelEnd = -1;
		DrawStartCursor();
	}
	else {
		if (m_iSelEnd < m_iSelStart) {
			int Temp = m_iSelEnd;
			m_iSelEnd = m_iSelStart;
			m_iSelStart = Temp;
		}
	}

	static_cast<CSampleEditorDlg*>(GetParent())->SelectionChanged();

	CStatic::OnLButtonUp(nFlags, point);
}

void CSampleEditorView::DrawPlayCursor(int Pos)
{
	CDC *pDC = GetDC();
	int x = int((double(Pos + m_iStartCursor) * 8.0 * 64.0 - m_iViewStart) / m_dSampleStep);

	pDC->BitBlt(0, 0, m_clientRect.Width(), m_clientRect.Height(), &m_dcCopy, 0, 0, SRCCOPY);

	if (Pos != -1) {
		CPen *oldPen = pDC->SelectObject(m_pDashedPen);
		pDC->MoveTo(x, 0);
		pDC->LineTo(x, m_clientRect.bottom);
		pDC->SelectObject(oldPen);
	}

	ReleaseDC(pDC);
}

void CSampleEditorView::DrawStartCursor()
{
	CDC *pDC = GetDC();
	int x = int((double(m_iStartCursor) * 8.0 * 64.0 - m_iViewStart) / m_dSampleStep);

	pDC->BitBlt(0, 0, m_clientRect.Width(), m_clientRect.Height(), &m_dcCopy, 0, 0, SRCCOPY);

	if (m_iStartCursor != -1) {
		CPen *oldPen = pDC->SelectObject(m_pDashedPen);
		pDC->MoveTo(x, 0);
		pDC->LineTo(x, m_clientRect.bottom);
		pDC->SelectObject(oldPen);
	}

	ReleaseDC(pDC);
}

void CSampleEditorView::ExpandSample(CDSample *pSample, int Start)
{
	// Expand DPCM to PCM
	//

	int Size = pSample->GetSize() * 8;

	SAFE_RELEASE_ARRAY(m_pSamples);

	if (pSample->GetSize() == 0) {
		m_iSize = 0;
		m_iStartCursor = 0;
		m_iSelStart = m_iSelEnd = -1;
		return;
	}

	SAFE_RELEASE_ARRAY(m_pSamples);
	m_pSamples = new int[Size];
	m_iSize = Size;

	const char *pData = pSample->GetData();
	int Delta = Start;

	for (int i = 0; i < Size; ++i) {
		int BitPos = (i & 0x07);
		if (pData[i >> 3] & (1 << BitPos)) {
			if (Delta < 126)
				Delta += 2;
		}
		else {
			if (Delta > 1)
				Delta -= 2;
		}
		m_pSamples[i] = Delta;
	}	

	m_iSelStart = m_iSelEnd = -1;
	m_iStartCursor = 0;

	if (m_iViewEnd == 0) {
		m_iViewStart = 0;
		m_iViewEnd = m_iSize;
		SetZoom(1.0f);
	}

	int ViewSize = m_iViewEnd - m_iViewStart;
	if (m_iViewEnd > m_iSize) {
		m_iViewEnd = m_iSize;
		m_iViewStart = m_iSize - ViewSize;
		if (m_iViewStart < 0) {
			m_iViewStart = 0;
			SetZoom(1.0f);
		}
	}
}

int CSampleEditorView::GetBlock(int Pixel) const
{
	// Convert pixel to selection block
	double Sample = double(Pixel) * m_dSampleStep;
	int Pos = int(Sample / (8.0 * 16.0));
	if (Pos > 255)
		Pos = 255;
	return Pos;
}

int CSampleEditorView::GetPixel(int Block) const
{
	return int((double(Block) * 128.0) / m_dSampleStep);
}

void CSampleEditorView::UpdateInfo()
{
	if (!m_iSize)
		return;

	CString Text, num;		// // //
	num.Format(_T("%i"), m_iSize / 8);
	AfxFormatString1(Text, ID_INDICATOR_DPCM_SIZE, num);
	static_cast<CSampleEditorDlg*>(GetParent())->UpdateStatus(1, Text);
	num.Format(_T("%i"), m_pSamples[m_iSize - 1]);
	AfxFormatString1(Text, ID_INDICATOR_DPCM_ENDPOS, num);
	static_cast<CSampleEditorDlg*>(GetParent())->UpdateStatus(2, Text);
}

void CSampleEditorView::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);

	if (m_pScrollBar->m_hWnd != NULL) {
		CRect clientRect, scrollRect;
		GetClientRect(&clientRect);
		m_pScrollBar->GetClientRect(&scrollRect);
		scrollRect.right = clientRect.right;
		int height = scrollRect.Height();
		scrollRect.top = clientRect.bottom - height;
		scrollRect.bottom = scrollRect.top + height;
		m_pScrollBar->MoveWindow(&scrollRect);
	}
}

void CSampleEditorView::OnHome()
{
	m_iStartCursor = 0;

	DrawStartCursor();
}

void CSampleEditorView::OnEnd()
{
	m_iStartCursor = m_iSize / (64 * 8);

	DrawStartCursor();
}

void CSampleEditorView::OnRight()
{
	m_iStartCursor++;

	if (m_iStartCursor > (m_iSize / (64 * 8)))
		m_iStartCursor = (m_iSize / (64 * 8));

	DrawStartCursor();
}

void CSampleEditorView::OnLeft()
{
	m_iStartCursor--;
	
	if (m_iStartCursor < 0)
		m_iStartCursor = 0;

	DrawStartCursor();
}

void CSampleEditorView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	CMenu contextMenu;
	contextMenu.LoadMenu(IDR_SAMPLE_EDITOR_POPUP);
	CMenu *pPopupMenu = contextMenu.GetSubMenu(0);
	pPopupMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd->GetParent());
}

bool CSampleEditorView::HasSelection() const
{
	return m_iSelStart != -1;
}

void CSampleEditorView::PreSubclassWindow()
{
	if (m_pScrollBar == NULL) {
		m_pScrollBar = new CScrollBar();
	}

	// Create scroll bar
	if (m_pScrollBar->m_hWnd == NULL) {
		CRect rect;
		GetClientRect(&rect);
		m_pScrollBar->Create(SBS_HORZ | SBS_BOTTOMALIGN | WS_CHILD | WS_VISIBLE, rect, this, 1);
		
	}

	CStatic::PreSubclassWindow();
}

void CSampleEditorView::SetZoom(float Factor)
{
	// Set zoom, 1.0f = no zoom, 0.0f = max zoom
	//

	int FullViewSize = m_iSize;
	int ViewSize = m_iViewEnd - m_iViewStart;
	int ViewMiddle = ViewSize / 2 + m_iViewStart;

	m_fZoom = ((MAX_FACTOR - MIN_FACTOR) * (Factor * Factor)) + MIN_FACTOR;		// // //

	int NewViewSize = int(FullViewSize * m_fZoom);

	m_iViewStart = ViewMiddle - NewViewSize / 2;
	m_iViewEnd = ViewMiddle + NewViewSize / 2;

	if (m_iViewEnd > m_iSize)
		m_iViewEnd = m_iSize;

	if (m_iViewStart < 0)
		m_iViewStart = 0;

	if (Factor == 1.0f) {
		m_pScrollBar->EnableWindow(FALSE);
	}
	else {
		m_pScrollBar->EnableWindow(TRUE);
		m_iScrollMax = int(1000.0f * (1.0f - m_fZoom));
		m_pScrollBar->SetScrollRange(0, m_iScrollMax);
		m_pScrollBar->SetScrollPos((m_iViewStart * m_iScrollMax) / (m_iSize - NewViewSize));

		SCROLLINFO ScrollInfo;
		ScrollInfo.cbSize = sizeof(SCROLLINFO);
		ScrollInfo.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
		ScrollInfo.nPage = UINT(1.0f + Factor * 100.0f);
		ScrollInfo.nMin = 0;
		ScrollInfo.nMax = m_iScrollMax;
		ScrollInfo.nPos = (m_iViewStart * m_iScrollMax) / (m_iSize - NewViewSize);
		m_pScrollBar->SetScrollInfo(&ScrollInfo);
	}
}

float CSampleEditorView::GetZoom() const		// // //
{
	return m_fZoom;
}

float CSampleEditorView::GetMaxZoomFactor() const
{
	return float(m_clientRect.Width() / 2) / float(m_iSize);
}

void CSampleEditorView::SetScroll(UINT nPos)
{
	if (nPos < 0 || nPos > (unsigned)m_iScrollMax)
		return;

	m_pScrollBar->SetScrollPos(nPos);

	int ViewSize = m_iViewEnd - m_iViewStart;
	m_iViewStart = ((m_iSize - ViewSize) * nPos) / m_iScrollMax;
	m_iViewEnd = m_iViewStart + ViewSize;

	Invalidate();
}

void CSampleEditorView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	switch (nSBCode) {
		case SB_LINERIGHT:
			SetScroll(pScrollBar->GetScrollPos() + 1);
			break;
		case SB_LINELEFT: 
			SetScroll(pScrollBar->GetScrollPos() - 1);
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			SetScroll(nPos);
			break;
	}

	CStatic::OnHScroll(nSBCode, nPos, pScrollBar);
}
