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

/*
 * The sequence length editor
 */

#include "stdafx.h"
#include "Sequence.h"		// // // MAX_SEQUENCE_ITEMS
#include "SizeEditor.h"
#include "SequenceEditorMessage.h"		// // //

const int CHANGE_DELAY = 400;
const int CHANGE_SPEED = 50;

// CSizeEditor

IMPLEMENT_DYNAMIC(CSizeEditor, CWnd)

BEGIN_MESSAGE_MAP(CSizeEditor, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
	ON_WM_TIMER()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

CSizeEditor::CSizeEditor(CWnd *pParent) : 
	CWnd(),
	m_iValue(0),
	m_iButtonPressed(0),
	m_pParentWnd(pParent),
	m_bSizeCursor(false)
{
}

CSizeEditor::~CSizeEditor()
{
}

void CSizeEditor::OnPaint()
{
	CPaintDC dc(this);

	CFont Font;
	Font.CreateFont(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, _T("Tahoma"));

	CFont *pOldFont = dc.SelectObject(&Font);

	CRect rect;
	GetClientRect(rect);

	COLORREF BUTTON_FACE = GetSysColor(COLOR_BTNFACE);
	COLORREF BUTTON_HILIGHT = GetSysColor(COLOR_BTNHILIGHT);
	COLORREF BUTTON_SHADOW = GetSysColor(COLOR_BTNSHADOW);

	// Background
	dc.FillSolidRect(rect, 0x00);
	dc.Draw3dRect(rect, BUTTON_SHADOW, BUTTON_HILIGHT);

	if (this == GetFocus()) {
		CRect focusRect = rect;
		focusRect.DeflateRect(rect.Height() - 1, 2, rect.Height() + 1, 2);
		dc.SetBkColor(0xFFFFFF);
		dc.DrawFocusRect(focusRect);
	}

	CRect buttonRect;

	// The '-'-button
	buttonRect = CRect(2, 2, rect.bottom - 2, rect.bottom - 2);
	dc.FillSolidRect(buttonRect, BUTTON_FACE);
	if (m_iButtonPressed == 1)
		dc.Draw3dRect(buttonRect, BUTTON_SHADOW, BUTTON_HILIGHT);
	else
		dc.Draw3dRect(buttonRect, BUTTON_HILIGHT, BUTTON_SHADOW);

	// The '+'-button
	buttonRect = CRect(rect.right - rect.bottom, 2, rect.right - 2, rect.bottom - 2);
	dc.FillSolidRect(buttonRect, BUTTON_FACE);
	if (m_iButtonPressed == 2)
		dc.Draw3dRect(buttonRect, BUTTON_SHADOW, BUTTON_HILIGHT);
	else
		dc.Draw3dRect(buttonRect, BUTTON_HILIGHT, BUTTON_SHADOW);

	CRect textRect;
	textRect = rect;
	textRect.DeflateRect(rect.bottom + 6, 1, rect.bottom + 6, 0);

	// Text
	CString str;
	str.Format(_T("%i"), m_iValue);		// // //
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(0xFFFFFF);
	dc.DrawText(str, &textRect, DT_RIGHT);
	dc.SetTextColor(0);
	dc.TextOut(6, 1 + ((m_iButtonPressed == 1) ? 1 : 0), _T("-"));
	dc.TextOut(rect.right - 14, 1 + ((m_iButtonPressed == 2) ? 1 : 0), _T("+"));

	dc.SelectObject(pOldFont);
}

BOOL CSizeEditor::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (m_bSizeCursor) {
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZENS));
		return TRUE;
	}
	return CWnd::OnSetCursor(pWnd, nHitTest, message);
}

void CSizeEditor::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetClientRect(rect);

	SetCapture();
	SetFocus();

	if ((point.x > rect.bottom) && (point.x < (rect.right - rect.bottom))) {
		m_iButtonPressed = 3;
	}
	else
		MouseAction(nFlags, point);
}

void CSizeEditor::OnLButtonUp(UINT nFlags, CPoint point)
{
	ReleaseCapture();
	m_iButtonPressed = 0;
	KillTimer(0);
	RedrawWindow(NULL);
}

void CSizeEditor::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	MouseAction(nFlags, point);
}

void CSizeEditor::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect rect;
	GetClientRect(rect);

	if ((nFlags & MK_LBUTTON) && (m_iButtonPressed == 3)) {
		// Drag
		static int LastY;
		if (LastY - point.y > 0)
			IncreaseValue();
		else if (LastY - point.y < 0)
			DecreaseValue();
		LastY = point.y;
		RedrawWindow(NULL);
	}
	else {
		MouseAction(nFlags, point);
		if (!(nFlags & MK_LBUTTON)) {
			if ((point.x > rect.bottom) && (point.x < (rect.right - rect.bottom)))
				m_bSizeCursor = true;
			else
				m_bSizeCursor = false;
		}
	}
}

void CSizeEditor::MouseAction(UINT nFlags, CPoint point)
{
	CRect rect;
	GetClientRect(rect);

	if (point.x < rect.bottom && (nFlags & MK_LBUTTON)) {
		if (m_iButtonPressed == 0) {
			DecreaseValue();
			SetTimer(0, CHANGE_DELAY, NULL);
		}
		m_iButtonPressed = 1;
	}
	else if (point.x > (rect.right - rect.bottom) && (nFlags & MK_LBUTTON)) {
		if (m_iButtonPressed == 0) {
			IncreaseValue();
			SetTimer(0, CHANGE_DELAY, NULL);
		}
		m_iButtonPressed = 2;
	}
	else
		m_iButtonPressed = 0;

	RedrawWindow(NULL);
}

void CSizeEditor::IncreaseValue()
{
	if (m_iValue < MAX_SEQUENCE_ITEMS)
		m_iValue++;

	m_pParentWnd->PostMessage(WM_SIZE_CHANGE, (LPARAM)m_iValue);
}

void CSizeEditor::DecreaseValue()
{
	if (m_iValue > 0)
		m_iValue--;

	m_pParentWnd->PostMessage(WM_SIZE_CHANGE, (LPARAM)m_iValue);
}

void CSizeEditor::SetValue(int Value)
{
	m_iValue = Value;
	RedrawWindow(NULL);
}

int CSizeEditor::GetValue() const
{
	return m_iValue;
}

void CSizeEditor::OnTimer(UINT_PTR nIDEvent)
{
	SetTimer(0, CHANGE_SPEED, NULL);

	if (m_iButtonPressed == 1)
		DecreaseValue();
	else if (m_iButtonPressed == 2)
		IncreaseValue();

	CWnd::OnTimer(nIDEvent);
}

void CSizeEditor::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);
	Invalidate();
}

void CSizeEditor::OnKillFocus(CWnd* pNewWnd)
{
	CWnd::OnKillFocus(pNewWnd);
	Invalidate();
}
