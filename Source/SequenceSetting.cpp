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

/*
 * The sequence setting button. Used only by arpeggio at the moment
 */

#include <string>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "Sequence.h"
#include "SequenceEditor.h"
#include "GraphEditor.h"
#include "InstrumentEditPanel.h"
#include "SequenceSetting.h"

// Arpeggio menu
enum {
	MENU_ARP_ABSOLUTE = 500, 
	MENU_ARP_RELATIVE, 
	MENU_ARP_FIXED,
	MENU_ARP_SCHEME		// // //
};

IMPLEMENT_DYNAMIC(CSequenceSetting, CWnd)

CSequenceSetting::CSequenceSetting(CWnd *pParent) 
	: CWnd(), m_pParent(pParent), m_pSequence(NULL), m_bMouseOver(false)
{
}

CSequenceSetting::~CSequenceSetting()
{
}

BEGIN_MESSAGE_MAP(CSequenceSetting, CWnd)
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(MENU_ARP_ABSOLUTE, OnMenuArpAbsolute)
	ON_COMMAND(MENU_ARP_RELATIVE, OnMenuArpRelative)
	ON_COMMAND(MENU_ARP_FIXED, OnMenuArpFixed)
	ON_COMMAND(MENU_ARP_SCHEME, OnMenuArpScheme) // // //
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


void CSequenceSetting::Setup(CFont *pFont)
{
	m_menuPopup.CreatePopupMenu();

	m_menuPopup.AppendMenu(MF_STRING, MENU_ARP_ABSOLUTE, _T("Absolute"));
	m_menuPopup.AppendMenu(MF_STRING, MENU_ARP_RELATIVE, _T("Relative"));
	m_menuPopup.AppendMenu(MF_STRING, MENU_ARP_FIXED, _T("Fixed"));
	m_menuPopup.AppendMenu(MF_STRING, MENU_ARP_SCHEME, _T("Scheme")); // // //

	m_pFont = pFont;
}

void CSequenceSetting::OnPaint()
{
	CPaintDC dc(this);
	bool bDraw(false);

	CRect rect;
	GetClientRect(&rect);

	if (m_iType == SEQ_ARPEGGIO)
		bDraw = true;

	int mode = m_pSequence->GetSetting();

	if (bDraw) {

		int BgColor = m_bMouseOver ? 0x303030 : 0x101010;

		dc.FillSolidRect(rect, BgColor);
		dc.DrawEdge(rect, EDGE_SUNKEN, BF_RECT);
		dc.SelectObject(m_pFont);
		dc.SetTextColor(0xFFFFFF);
		dc.SetBkColor(BgColor);

		LPCTSTR MODES[] = {_T("Absolute"), _T("Fixed"), _T("Relative"), _T("Scheme")}; // // //

		rect.top += 2;
		dc.DrawText(MODES[mode], _tcslen(MODES[mode]), rect, DT_CENTER);
	}
	else {
		dc.FillSolidRect(rect, 0xFFFFFF);
	}
}

void CSequenceSetting::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetWindowRect(rect);

	if (m_iType == SEQ_ARPEGGIO) {

		switch (m_pSequence->GetSetting()) {		// // //
			case ARP_SETTING_ABSOLUTE:
				m_menuPopup.CheckMenuRadioItem(MENU_ARP_ABSOLUTE, MENU_ARP_SCHEME, MENU_ARP_ABSOLUTE, MF_BYCOMMAND);
				break;
			case ARP_SETTING_RELATIVE:
				m_menuPopup.CheckMenuRadioItem(MENU_ARP_ABSOLUTE, MENU_ARP_SCHEME, MENU_ARP_RELATIVE, MF_BYCOMMAND);
				break;
			case ARP_SETTING_FIXED:
				m_menuPopup.CheckMenuRadioItem(MENU_ARP_ABSOLUTE, MENU_ARP_SCHEME, MENU_ARP_FIXED, MF_BYCOMMAND);
				break;
			case ARP_SETTING_SCHEME:
				m_menuPopup.CheckMenuRadioItem(MENU_ARP_ABSOLUTE, MENU_ARP_SCHEME, MENU_ARP_SCHEME, MF_BYCOMMAND);
				break;
		}

		m_menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x + rect.left, point.y + rect.top, this);
	}

	CWnd::OnLButtonDown(nFlags, point);
}

void CSequenceSetting::SelectSequence(CSequence *pSequence, int Type, int InstrumentType)
{
	m_pSequence = pSequence;
	m_iType		= Type;
	m_iInstType = InstrumentType;

	UpdateWindow();
	RedrawWindow();
}

void CSequenceSetting::OnMenuArpAbsolute()
{
	m_pSequence->SetSetting(ARP_SETTING_ABSOLUTE);
	static_cast<CSequenceEditor*>(m_pParent)->ChangedSetting();
}

void CSequenceSetting::OnMenuArpRelative()
{
	m_pSequence->SetSetting(ARP_SETTING_RELATIVE);
	static_cast<CSequenceEditor*>(m_pParent)->ChangedSetting();
}

void CSequenceSetting::OnMenuArpFixed()
{
	m_pSequence->SetSetting(ARP_SETTING_FIXED);
	static_cast<CSequenceEditor*>(m_pParent)->ChangedSetting();

	// Prevent invalid sequence items
	for (unsigned int i = 0; i < m_pSequence->GetItemCount(); ++i) {
		int Item = m_pSequence->GetItem(i);
		if (Item < 0)
			Item = 0;
		m_pSequence->SetItem(i, Item);
	}
}

void CSequenceSetting::OnMenuArpScheme()		// // //
{
	m_pSequence->SetSetting(ARP_SETTING_SCHEME);
	static_cast<CSequenceEditor*>(m_pParent)->ChangedSetting();
}

void CSequenceSetting::OnMouseMove(UINT nFlags, CPoint point)
{
	bool bOldMouseOver = m_bMouseOver;
	m_bMouseOver = true;
	if (bOldMouseOver != m_bMouseOver) {
		TRACKMOUSEEVENT mouseEvent;
		mouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
		mouseEvent.dwFlags = TME_LEAVE;
		mouseEvent.hwndTrack = m_hWnd;
		TrackMouseEvent(&mouseEvent);
		RedrawWindow();
	}

	CWnd::OnMouseMove(nFlags, point);
}

void CSequenceSetting::OnMouseLeave()
{
	m_bMouseOver = false;
	RedrawWindow();

	TRACKMOUSEEVENT mouseEvent;
	mouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
	mouseEvent.dwFlags = TME_CANCEL;
	mouseEvent.hwndTrack = m_hWnd;
	TrackMouseEvent(&mouseEvent);

	CWnd::OnMouseLeave();
}
