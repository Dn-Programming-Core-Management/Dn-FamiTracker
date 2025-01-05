/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/

#include "stdafx.h"
#include "CustomControls.h"
#include "DPI.h"		// // //

/*

 Contains some custom GUI controls.

 * CInstrumentList
   - The instrument list (moved to separate file)
 
 * CBannerEdit
   - An edit box that displays a banner when no text is present
 
 * CLockedEdit
   - An edit box that is locked for editing, unlocked with double-click

*/

///
/// CBannerEdit
///

const TCHAR CBannerEdit::BANNER_FONT[]	 = _T("Tahoma");
const COLORREF CBannerEdit::BANNER_COLOR = 0x808080;

// Used to display a banner in edit boxes

IMPLEMENT_DYNAMIC(CBannerEdit, CEdit)

BEGIN_MESSAGE_MAP(CBannerEdit, CEdit)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CBannerEdit::OnSetFocus(CWnd* pOldWnd)
{
	CEdit::OnSetFocus(pOldWnd);
	Invalidate();
	RedrawWindow(NULL, NULL, RDW_ERASE);
}

void CBannerEdit::OnPaint()
{
	CEdit::OnPaint();

	// Overlay some text
	CString str;
	GetWindowText(str);

	// only if empty, enabled, and not in focus
	if (str.GetLength() > 0 || GetFocus() == this || !IsWindowEnabled())
		return;

	CDC *pDC = GetDC();
	if (pDC != NULL) {
		CFont font;
		font.CreateFont(DPI::SY(13), 0, 0, 0, 0, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, BANNER_FONT);
		CFont *pOldFont = pDC->SelectObject(&font);

		pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
		pDC->SetTextColor(BANNER_COLOR);
		pDC->TextOut(2, 1, m_strText);
		pDC->SelectObject(pOldFont);

		ReleaseDC(pDC);
	}
}

void CBannerEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	// Limit string size to 31 chars
	CString text;
	GetWindowText(text);
	if (text.GetLength() > 31)
		text = text.Left(31);
	SetWindowText(text);
}

///
/// CLockedEdit
///

// This class takes care of locking/unlocking edit boxes by double clicking

IMPLEMENT_DYNAMIC(CLockedEdit, CEdit)

BEGIN_MESSAGE_MAP(CLockedEdit, CEdit)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

bool CLockedEdit::IsEditable() const
{
	return !((GetWindowLong(m_hWnd, GWL_STYLE) & ES_READONLY) == ES_READONLY);
}

bool CLockedEdit::Update()
{
	bool ret_val(m_bUpdate);
	m_bUpdate = false;
	return ret_val;
}

int CLockedEdit::GetValue() const
{
	return m_iValue;
}

void CLockedEdit::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	m_bUpdate = false;
	if (IsEditable())
		SetSel(0, -1);	// select all
	else {
		SendMessage(EM_SETREADONLY, FALSE);
		SetFocus();
		SetSel(0, -1);
	}
}

void CLockedEdit::OnSetFocus(CWnd* pOldWnd)
{
	CEdit::OnSetFocus(pOldWnd);

	if (!IsEditable())
		static_cast<CFrameWnd*>(AfxGetMainWnd())->GetActiveView()->SetFocus();		// // //
}

void CLockedEdit::OnKillFocus(CWnd* pNewWnd)
{
	CEdit::OnKillFocus(pNewWnd);
	CString Text;
	if (!IsEditable())
		return;
	GetWindowText(Text);
	m_iValue = _ttoi(Text);
	m_bUpdate = true;
	SendMessage(EM_SETREADONLY, TRUE);
}

BOOL CLockedEdit::PreTranslateMessage(MSG* pMsg)
{
	// For some reason OnKeyDown won't work
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) {
		static_cast<CFrameWnd*>(AfxGetMainWnd())->GetActiveView()->SetFocus();		// // //
		return TRUE;
	}

	return CEdit::PreTranslateMessage(pMsg);
}
