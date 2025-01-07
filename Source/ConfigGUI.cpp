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
#include "FamiTracker.h"
#include "FamiTrackerTypes.h"
#include "SoundGen.h"
#include "ConfigGUI.h"
#include "Settings.h"
#include "MainFrm.h"



// CConfigGUI dialog

IMPLEMENT_DYNAMIC(CConfigGUI, CPropertyPage)
CConfigGUI::CConfigGUI()
	: CPropertyPage(CConfigGUI::IDD)
{
}

CConfigGUI::~CConfigGUI()
{
}

void CConfigGUI::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CConfigGUI, CPropertyPage)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_PRECISEREGPITCH_CHECK, &CConfigGUI::OnBnClickedSmoothregfreqCheck)
	ON_EN_KILLFOCUS(IDC_MAXCHANVIEW_EDIT, &CConfigGUI::OnEnKillfocusMaxchanviewEdit)
	ON_EN_KILLFOCUS(IDC_IDLE_REFRESH_EDIT, &CConfigGUI::OnEnKillfocusIdleRefreshEdit)
	ON_EN_CHANGE(IDC_MAXCHANVIEW_EDIT, &CConfigGUI::OnEnChangeMaxchanviewEdit)
	ON_EN_CHANGE(IDC_IDLE_REFRESH_EDIT, &CConfigGUI::OnEnChangeIdleRefreshEdit)
END_MESSAGE_MAP()


// CConfigGUI message handlers

BOOL CConfigGUI::OnInitDialog()
{
	CSettings* pSettings = theApp.GetSettings();

	CSliderCtrl* pIdleRefSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_IDLE_REFRESH_SLIDER));
	pIdleRefSlider->SetRange(16, 1000);
	pIdleRefSlider->SetPos(pSettings->GUI.iLowRefreshRate);

	CSliderCtrl* pMaxChanSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_MAXCHANVIEW_SLIDER));
	pMaxChanSlider->SetRange(5, 28);
	pMaxChanSlider->SetPos(pSettings->GUI.iMaxChannelView);

	m_bPreciseRegPitch = pSettings->GUI.bPreciseRegPitch;
	CheckDlgButton(IDC_PRECISEREGPITCH_CHECK, pSettings->GUI.bPreciseRegPitch);

	UpdateSliderTexts();
	CPropertyPage::OnInitDialog();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CConfigGUI::OnApply()
{
	CSettings* pSettings = theApp.GetSettings();

	theApp.GetSettings()->GUI.iLowRefreshRate = static_cast<CSliderCtrl*>(GetDlgItem(IDC_IDLE_REFRESH_SLIDER))->GetPos();
	theApp.GetSettings()->GUI.iMaxChannelView = static_cast<CSliderCtrl*>(GetDlgItem(IDC_MAXCHANVIEW_SLIDER))->GetPos();
	theApp.GetSettings()->GUI.bPreciseRegPitch = IsDlgButtonChecked(IDC_PRECISEREGPITCH_CHECK);

	// trigger CMainFrame::ResizeFrameWindow()
	theApp.RefreshFrameEditor();

	return CPropertyPage::OnApply();
}

void CConfigGUI::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	UpdateSliderTexts();
	SetModified();
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CConfigGUI::UpdateSliderTexts()
{
	CString Text;

	Text.Format(_T("%i"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_IDLE_REFRESH_SLIDER))->GetPos());
	SetDlgItemText(IDC_IDLE_REFRESH_EDIT, Text);

	Text.Format(_T("%i"), static_cast<CSliderCtrl*>(GetDlgItem(IDC_MAXCHANVIEW_SLIDER))->GetPos());
	SetDlgItemText(IDC_MAXCHANVIEW_EDIT, Text);
}


void CConfigGUI::OnBnClickedSmoothregfreqCheck()
{
	m_bPreciseRegPitch = !IsDlgButtonChecked(IDC_PRECISEREGPITCH_CHECK);
	SetModified();
}

void CConfigGUI::OnEnKillfocusMaxchanviewEdit()
{
	CString Text, posText;
	GetDlgItemText(IDC_MAXCHANVIEW_EDIT, Text);
	int pos = _ttoi(Text);

	if (pos < 5) pos = 5;
	if (pos > 28) pos = 28;

	posText.Format("%i", pos);

	SetDlgItemText(IDC_MAXCHANVIEW_EDIT, posText);
	SetModified();
}


void CConfigGUI::OnEnKillfocusIdleRefreshEdit()
{
	CString Text, posText;
	GetDlgItemText(IDC_IDLE_REFRESH_EDIT, Text);
	int pos = _ttoi(Text);

	if (pos < 16) pos = 16;
	if (pos > 1000) pos = 1000;

	posText.Format("%i", pos);

	SetDlgItemText(IDC_IDLE_REFRESH_EDIT, posText);
	SetModified();
}


void CConfigGUI::OnEnChangeMaxchanviewEdit()
{
	CString Text, posText;
	GetDlgItemText(IDC_MAXCHANVIEW_EDIT, Text);
	int pos = _ttoi(Text);

	if (pos < 5) pos = 5;
	if (pos > 28) pos = 28;

	CSliderCtrl* pIdleRefSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_MAXCHANVIEW_SLIDER));
	pIdleRefSlider->SetPos(pos);

	SetModified();
}


void CConfigGUI::OnEnChangeIdleRefreshEdit()
{
	CString Text, posText;
	GetDlgItemTextA(IDC_IDLE_REFRESH_EDIT, Text);
	int pos = _ttoi(Text);

	if (pos < 16) pos = 16;
	if (pos > 1000) pos = 1000;

	CSliderCtrl* pIdleRefSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_IDLE_REFRESH_SLIDER));
	pIdleRefSlider->SetPos(pos);

	SetModified();
}
