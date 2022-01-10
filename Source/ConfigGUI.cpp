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
	ON_BN_CLICKED(IDC_SMOOTHREGFREQ_CHECK, &CConfigGUI::OnBnClickedSmoothregfreqCheck)
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

	m_bSmoothRegFreq = pSettings->GUI.bSmoothRegFreq;
	CheckDlgButton(IDC_SMOOTHREGFREQ_CHECK, pSettings->GUI.bSmoothRegFreq);

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
	theApp.GetSettings()->GUI.bSmoothRegFreq = IsDlgButtonChecked(IDC_SMOOTHREGFREQ_CHECK);

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
	m_bSmoothRegFreq = !IsDlgButtonChecked(IDC_SMOOTHREGFREQ_CHECK);
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
