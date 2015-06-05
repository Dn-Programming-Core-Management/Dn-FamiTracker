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
#include "MainFrm.h"
#include "PatternEditor.h"
#include "GotoDlg.h"

// CGotoDlg dialog

IMPLEMENT_DYNAMIC(CGotoDlg, CDialog)

CGotoDlg::CGotoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGotoDlg::IDD, pParent)
{
}

CGotoDlg::~CGotoDlg()
{
}

void CGotoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CGotoDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT_GOTO_FRAME, OnEnChangeEditGotoFrame)
	ON_EN_CHANGE(IDC_EDIT_GOTO_ROW, OnEnChangeEditGotoRow)
	ON_EN_CHANGE(IDC_EDIT_GOTO_CHANNEL, OnEnChangeEditGotoChannel)
	ON_CBN_SELCHANGE(IDC_COMBO_GOTO_CHIP, OnCbnSelchangeComboGotoChip)
	ON_BN_CLICKED(IDOK, &CGotoDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CGotoDlg message handlers

BOOL CGotoDlg::OnInitDialog()
{
	m_cFrameEdit   = new CEdit();
	m_cRowEdit     = new CEdit();
	m_cChipEdit    = new CComboBox();
	m_cChannelEdit = new CEdit();

	m_cFrameEdit->SubclassDlgItem(IDC_EDIT_GOTO_FRAME, this);
	m_cRowEdit->SubclassDlgItem(IDC_EDIT_GOTO_ROW, this);
	m_cChipEdit->SubclassDlgItem(IDC_COMBO_GOTO_CHIP, this);
	m_cChannelEdit->SubclassDlgItem(IDC_EDIT_GOTO_CHANNEL, this);

	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveView());
	CPatternEditor *pEditor = pView->GetPatternEditor();

	m_cChipEdit->AddString(_T("2A03"));
	if (pDoc->ExpansionEnabled(SNDCHIP_VRC6))
		m_cChipEdit->AddString(_T("VRC6"));
	if (pDoc->ExpansionEnabled(SNDCHIP_VRC7))
		m_cChipEdit->AddString(_T("VRC7"));
	if (pDoc->ExpansionEnabled(SNDCHIP_FDS))
		m_cChipEdit->AddString(_T("FDS"));
	if (pDoc->ExpansionEnabled(SNDCHIP_MMC5))
		m_cChipEdit->AddString(_T("MMC5"));
	if (pDoc->ExpansionEnabled(SNDCHIP_N163))
		m_cChipEdit->AddString(_T("N163"));
	if (pDoc->ExpansionEnabled(SNDCHIP_S5B))
		m_cChipEdit->AddString(_T("5B"));
		
	int Channel = pDoc->GetChannelType(pEditor->GetChannel());
	if (Channel >= CHANID_S5B_CH1) {
		Channel -= CHANID_S5B_CH1;
		m_cChipEdit->SelectString(-1, _T("5B"));
	}
	else if (Channel >= CHANID_VRC7_CH1) {
		Channel -= CHANID_VRC7_CH1;
		m_cChipEdit->SelectString(-1, _T("VRC7"));
	}
	else if (Channel >= CHANID_FDS) {
		Channel -= CHANID_FDS;
		m_cChipEdit->SelectString(-1, _T("FDS"));
	}
	else if (Channel >= CHANID_N163_CH1) {
		Channel -= CHANID_N163_CH1;
		m_cChipEdit->SelectString(-1, _T("N163"));
	}
	else if (Channel >= CHANID_MMC5_SQUARE1) {
		Channel -= CHANID_MMC5_SQUARE1;
		m_cChipEdit->SelectString(-1, _T("MMC5"));
	}
	else if (Channel >= CHANID_VRC6_PULSE1) {
		Channel -= CHANID_VRC6_PULSE1;
		m_cChipEdit->SelectString(-1, _T("VRC6"));
	}
	else
		m_cChipEdit->SelectString(-1, _T("2A03"));

	CString str;
	str.Format(_T("%d"), pEditor->GetFrame());
	m_cFrameEdit->SetWindowText(str);
	str.Format(_T("%d"), pEditor->GetRow());
	m_cRowEdit->SetWindowText(str);
	str.Format(_T("%d"), Channel + 1);
	m_cChannelEdit->SetWindowText(str);

	m_cFrameEdit->SetFocus();

	return CDialog::OnInitDialog();
}

void CGotoDlg::CheckDestination() const
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	int Track = static_cast<CMainFrame*>(AfxGetMainWnd())->GetSelectedTrack();

	bool Valid = true;
	if (m_iDestFrame >= pDoc->GetFrameCount(Track))
		Valid = false;
	else if (m_iDestRow >= static_cast<unsigned>(pDoc->GetFrameLength(Track, m_iDestFrame)))
		Valid = false;
	else if (GetFinalChannel() == -1)
		Valid = false;

	GetDlgItem(IDOK)->EnableWindow(Valid);
}

int CGotoDlg::GetChipFromString(const CString str)
{
	if (str == _T("2A03"))
		return SNDCHIP_NONE;
	else if (str == _T("VRC6"))
		return SNDCHIP_VRC6;
	else if (str == _T("VRC7"))
		return SNDCHIP_VRC7;
	else if (str == _T("FDS"))
		return SNDCHIP_FDS;
	else if (str == _T("MMC5"))
		return SNDCHIP_MMC5;
	else if (str == _T("N163"))
		return SNDCHIP_N163;
	else if (str == _T("5B"))
		return SNDCHIP_S5B;
	else
		return SNDCHIP_NONE;
}

int CGotoDlg::GetFinalChannel() const
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	int Channel = m_iDestChannel;
	switch (m_iDestChip) {
	case SNDCHIP_VRC6: Channel += CHANID_VRC6_PULSE1; break;
	case SNDCHIP_VRC7: Channel += CHANID_VRC7_CH1; break;
	case SNDCHIP_FDS:  Channel += CHANID_FDS; break;
	case SNDCHIP_MMC5: Channel += CHANID_MMC5_SQUARE1; break;
	case SNDCHIP_N163: Channel += CHANID_N163_CH1; break;
	case SNDCHIP_S5B:  Channel += CHANID_S5B_CH1; break;
	}

	return pDoc->GetChannelIndex(Channel);
}

void CGotoDlg::OnEnChangeEditGotoFrame()
{
	CString str;
	m_cFrameEdit->GetWindowText(str);
	m_iDestFrame = atoi(str);
	CheckDestination();
}

void CGotoDlg::OnEnChangeEditGotoRow()
{
	CString str;
	m_cRowEdit->GetWindowText(str);
	m_iDestRow = atoi(str);
	CheckDestination();
}

void CGotoDlg::OnEnChangeEditGotoChannel()
{
	CString str;
	m_cChannelEdit->GetWindowText(str);
	m_iDestChannel = atoi(str) - 1;
	CheckDestination();
}

void CGotoDlg::OnCbnSelchangeComboGotoChip()
{
	CString str;
	m_cChipEdit->GetWindowText(str);
	m_iDestChip = GetChipFromString(str);
	CheckDestination();
}

void CGotoDlg::OnBnClickedOk()
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveView());
	pView->SelectFrame(m_iDestFrame);
	pView->SelectRow(m_iDestRow);
	pView->SelectChannel(GetFinalChannel());

	CDialog::OnOK();
}
