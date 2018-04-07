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
#include "resource.h"
#include "FamiTrackerDoc.h"
#include "SwapDlg.h"

// CSwapDlg dialog

IMPLEMENT_DYNAMIC(CSwapDlg, CDialog)

CSwapDlg::CSwapDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SWAP, pParent)
{
}

void CSwapDlg::SetTrack(unsigned int Track)
{
	m_iTrack = Track;
}

CSwapDlg::~CSwapDlg()
{
	SAFE_RELEASE(m_cChannelFirst);
	SAFE_RELEASE(m_cChannelSecond);
	SAFE_RELEASE(m_cChipFirst);
	SAFE_RELEASE(m_cChipSecond);
}

void CSwapDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSwapDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT_SWAP_CHAN1, OnEnChangeEditSwapChan1)
	ON_EN_CHANGE(IDC_EDIT_SWAP_CHAN2, OnEnChangeEditSwapChan2)
	ON_CBN_SELCHANGE(IDC_COMBO_SWAP_CHIP1, OnCbnSelchangeComboSwapChip1)
	ON_CBN_SELCHANGE(IDC_COMBO_SWAP_CHIP2, OnCbnSelchangeComboSwapChip2)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CSwapDlg message handlers

BOOL CSwapDlg::OnInitDialog()
{
	m_cChannelFirst = new CEdit();
	m_cChannelSecond = new CEdit();
	m_cChipFirst = new CComboBox();
	m_cChipSecond = new CComboBox();
	
	m_cChannelFirst->SubclassDlgItem(IDC_EDIT_SWAP_CHAN1, this);
	m_cChannelSecond->SubclassDlgItem(IDC_EDIT_SWAP_CHAN2, this);
	m_cChipFirst->SubclassDlgItem(IDC_COMBO_SWAP_CHIP1, this);
	m_cChipSecond->SubclassDlgItem(IDC_COMBO_SWAP_CHIP2, this);
	
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	m_cChipFirst->AddString(_T("2A03"));
	if (pDoc->ExpansionEnabled(SNDCHIP_VRC6))
		m_cChipFirst->AddString(_T("VRC6"));
	if (pDoc->ExpansionEnabled(SNDCHIP_VRC7))
		m_cChipFirst->AddString(_T("VRC7"));
	if (pDoc->ExpansionEnabled(SNDCHIP_FDS))
		m_cChipFirst->AddString(_T("FDS"));
	if (pDoc->ExpansionEnabled(SNDCHIP_MMC5))
		m_cChipFirst->AddString(_T("MMC5"));
	if (pDoc->ExpansionEnabled(SNDCHIP_N163))
		m_cChipFirst->AddString(_T("N163"));
	if (pDoc->ExpansionEnabled(SNDCHIP_S5B))
		m_cChipFirst->AddString(_T("5B"));

	CString str;
	for (int i = 0; i < m_cChipFirst->GetCount(); i++)
	{
	   m_cChipFirst->GetLBText(i, str);
	   m_cChipSecond->AddString(str);
	}
	m_cChannelFirst->SetWindowText(_T("1"));
	m_cChannelSecond->SetWindowText(_T("2"));
	m_cChipFirst->SetCurSel(0);
	m_cChipSecond->SetCurSel(0);
	CheckDlgButton(IDC_CHECK_SWAP_ALL, BST_UNCHECKED);

	m_cChannelFirst->SetFocus();

	return CDialog::OnInitDialog();
}

void CSwapDlg::CheckDestination() const
{
	GetDlgItem(IDOK)->EnableWindow(GetFinalChannel(m_iDestChannel1, m_iDestChip1) != -1 &&
								   GetFinalChannel(m_iDestChannel2, m_iDestChip2) != -1 &&
								   (m_iDestChannel1 != m_iDestChannel2 || m_iDestChip1 != m_iDestChip2));
}

int CSwapDlg::GetChipFromString(const CString str)
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

int CSwapDlg::GetFinalChannel(unsigned int Channel, unsigned int Chip) const
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	switch (Chip) {
	case SNDCHIP_VRC6: Channel += CHANID_VRC6_PULSE1; break;
	case SNDCHIP_VRC7: Channel += CHANID_VRC7_CH1; break;
	case SNDCHIP_FDS:  Channel += CHANID_FDS; break;
	case SNDCHIP_MMC5: Channel += CHANID_MMC5_SQUARE1; break;
	case SNDCHIP_N163: Channel += CHANID_N163_CH1; break;
	case SNDCHIP_S5B:  Channel += CHANID_S5B_CH1; break;
	}

	return pDoc->GetChannelIndex(Channel);
}

void CSwapDlg::OnEnChangeEditSwapChan1()
{
	CString str;
	m_cChannelFirst->GetWindowText(str);
	m_iDestChannel1 = atoi(str) - 1;
	CheckDestination();
}

void CSwapDlg::OnEnChangeEditSwapChan2()
{
	CString str;
	m_cChannelSecond->GetWindowText(str);
	m_iDestChannel2 = atoi(str) - 1;
	CheckDestination();
}

void CSwapDlg::OnCbnSelchangeComboSwapChip1()
{
	CString str;
	m_cChipFirst->GetWindowText(str);
	m_iDestChip1 = GetChipFromString(str);
	CheckDestination();
}

void CSwapDlg::OnCbnSelchangeComboSwapChip2()
{
	CString str;
	m_cChipSecond->GetWindowText(str);
	m_iDestChip2 = GetChipFromString(str);
	CheckDestination();
}

void CSwapDlg::OnBnClickedOk()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	if (IsDlgButtonChecked(IDC_CHECK_SWAP_ALL) == BST_CHECKED)
		for (unsigned int i = 0; i < pDoc->GetTrackCount(); i++)
			pDoc->SwapChannels(i, GetFinalChannel(m_iDestChannel1, m_iDestChip1),
								  GetFinalChannel(m_iDestChannel2, m_iDestChip2));
	else
		pDoc->SwapChannels(m_iTrack,
						   GetFinalChannel(m_iDestChannel1, m_iDestChip1),
						   GetFinalChannel(m_iDestChannel2, m_iDestChip2));
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
	pDoc->UpdateAllViews(NULL, UPDATE_FRAME);
	pDoc->UpdateAllViews(NULL, UPDATE_COLUMNS);

	CDialog::OnOK();
}
