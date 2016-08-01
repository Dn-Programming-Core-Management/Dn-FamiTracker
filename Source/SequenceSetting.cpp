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

#include "stdafx.h"
#include "Sequence.h"
#include "SequenceSetting.h"
#include "SequenceEditorMessage.h"		// // //
#include "Instrument.h"		// // // inst_type_t

// Sequence setting menu

static LPCTSTR SEQ_SETTING_TEXT[][SEQ_COUNT] = {		// // // 050B
	{_T("16 steps"), _T("Absolute"), _T("Relative"), nullptr, nullptr},
	{_T("64 steps"),    _T("Fixed"), _T("Absolute"), nullptr, nullptr},
#ifdef _DEBUG
	{       nullptr, _T("Relative"),    _T("Sweep"), nullptr, nullptr},
#else
	{       nullptr, _T("Relative"),        nullptr, nullptr, nullptr},
#endif
	{       nullptr,   _T("Scheme"),        nullptr, nullptr, nullptr},
};

const UINT CSequenceSetting::MENU_ID_BASE = 0x1000U;		// // //
const UINT CSequenceSetting::MENU_ID_MAX  = 0x100FU;		// // //

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
	ON_COMMAND_RANGE(MENU_ID_BASE, MENU_ID_MAX, OnMenuSettingChanged)		// // //
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()


void CSequenceSetting::Setup(CFont *pFont)
{
	m_pFont = pFont;
}

void CSequenceSetting::OnPaint()
{
	CPaintDC dc {this};
	CRect rect;
	GetClientRect(&rect);

	unsigned mode = m_pSequence->GetSetting();		// // //
	if (mode > SEQ_SETTING_COUNT[m_iType] || SEQ_SETTING_TEXT[mode][m_iType] == nullptr) {
		dc.FillSolidRect(rect, 0xFFFFFF); return;
	}
	LPCTSTR str = SEQ_SETTING_TEXT[mode][m_iType];

	int BgColor = m_bMouseOver ? 0x303030 : 0x101010;

	dc.FillSolidRect(rect, BgColor);
	dc.DrawEdge(rect, EDGE_SUNKEN, BF_RECT);
	dc.SelectObject(m_pFont);
	dc.SetTextColor(0xFFFFFF);
	dc.SetBkColor(BgColor);

	rect.top += 2;
	dc.DrawText(str, _tcslen(str), rect, DT_CENTER);
}

void CSequenceSetting::OnLButtonDown(UINT nFlags, CPoint point)
{
	CRect rect;
	GetWindowRect(rect);

	unsigned Setting = m_pSequence->GetSetting();		// // //
	if (SEQ_SETTING_COUNT[m_iType] < 2) return;

	m_menuPopup.CreatePopupMenu();
	for (unsigned i = 0; i < SEQ_SETTING_COUNT[m_iType]; ++i)
		m_menuPopup.AppendMenu(MF_STRING, MENU_ID_BASE + i, SEQ_SETTING_TEXT[i][m_iType]);
	m_menuPopup.CheckMenuRadioItem(MENU_ID_BASE, MENU_ID_MAX, MENU_ID_BASE + Setting, MF_BYCOMMAND);
#ifndef _DEBUG
	if (m_iType == SEQ_VOLUME && m_iInstType != INST_VRC6)
		m_menuPopup.EnableMenuItem(MENU_ID_BASE + SETTING_VOL_64_STEPS, MF_DISABLED);		// // // 050B
	else if (m_iType == SEQ_PITCH && m_iInstType != INST_2A03)
		m_menuPopup.EnableMenuItem(MENU_ID_BASE + SETTING_PITCH_SWEEP, MF_DISABLED);
#endif
	m_menuPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x + rect.left, point.y + rect.top, this);
	m_menuPopup.DestroyMenu();		// // //

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

void CSequenceSetting::OnMenuSettingChanged(UINT ID)		// // //
{
	unsigned Setting = m_pSequence->GetSetting();
	unsigned New = ID - MENU_ID_BASE;
	ASSERT(New < SEQ_SETTING_COUNT[m_iType]);

	if (m_iType == SEQ_VOLUME) {		// // // 050B
		if (Setting == SETTING_VOL_16_STEPS && New == SETTING_VOL_64_STEPS)
			for (unsigned int i = 0; i < m_pSequence->GetItemCount(); ++i)
				m_pSequence->SetItem(i, m_pSequence->GetItem(i) * 4);
		else if (Setting == SETTING_VOL_64_STEPS && New == SETTING_VOL_16_STEPS)
			for (unsigned int i = 0; i < m_pSequence->GetItemCount(); ++i)
				m_pSequence->SetItem(i, m_pSequence->GetItem(i) / 4);
	}

	if (m_iType == SEQ_ARPEGGIO && Setting != SETTING_ARP_SCHEME) {
		for (unsigned int i = 0; i < m_pSequence->GetItemCount(); ++i) {
			int Item = m_pSequence->GetItem(i) & 0x3F;
			if (Item > 0x24) Item -= 0x40;
			m_pSequence->SetItem(i, Item);
		}
	}
	
	m_pSequence->SetSetting(static_cast<seq_setting_t>(New));
	m_pParent->PostMessage(WM_SETTING_CHANGED);		// // //

	if (m_iType == SEQ_ARPEGGIO && Setting == SETTING_ARP_FIXED) {
		// Prevent invalid sequence items
		for (unsigned int i = 0; i < m_pSequence->GetItemCount(); ++i) {
			int Item = m_pSequence->GetItem(i);
			if (Item < 0)
				Item = 0;
			m_pSequence->SetItem(i, Item);
		}
	}
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
