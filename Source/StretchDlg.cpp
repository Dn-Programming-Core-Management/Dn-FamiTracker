/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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

#include <vector>
#include "stdafx.h"
#include "../resource.h"
#include "StretchDlg.h"


// CStretchDlg dialog

IMPLEMENT_DYNAMIC(CStretchDlg, CDialog)

CStretchDlg::CStretchDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CStretchDlg::IDD, pParent), m_iStretchMap(), m_bValid(true)
{

}

CStretchDlg::~CStretchDlg()
{
}

void CStretchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CStretchDlg, CDialog)
	ON_EN_CHANGE(IDC_EDIT_STRETCH_MAP, OnEnChangeEditStretchMap)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_STRETCH_EXPAND, OnBnClickedButtonStretchExpand)
	ON_BN_CLICKED(IDC_BUTTON_STRETCH_SHRINK, OnBnClickedButtonStretchShrink)
	ON_BN_CLICKED(IDC_BUTTON_STRETCH_RESET, OnBnClickedButtonStretchReset)
	ON_BN_CLICKED(IDC_BUTTON_STRETCH_INVERT, OnBnClickedButtonStretchInvert)
END_MESSAGE_MAP()


// CStretchDlg message handlers


std::vector<int> CStretchDlg::GetStretchMap()
{
	CDialog::DoModal();
	return m_iStretchMap;
}

BOOL CStretchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	OnBnClickedButtonStretchReset();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CStretchDlg::UpdateStretch()
{
	CString str, token;
	int pos = 0;
	GetDlgItemText(IDC_EDIT_STRETCH_MAP, str);
	m_iStretchMap.clear();
	if (str.SpanIncluding(_T("0123456789 ")) != str) {
		m_bValid = false;
		return;
	}

	token = str.Tokenize(_T(" "), pos);
	while (!token.IsEmpty()) {
		m_iStretchMap.push_back(atoi(token));
		token = str.Tokenize(_T(" "), pos);
	}

	if (m_iStretchMap.empty()) m_bValid = false;
	else if (m_iStretchMap[0] == 0) m_bValid = false;
	else {
		m_bValid = true;
		UpdateTest();
	}
}

void CStretchDlg::UpdateTest()
{
	CString str = _T("Test:");
	unsigned int count = 0, mapPos = 0;
	for (int i = 0; i < STRETCH_MAP_TEST_LEN; i++) {
		if (count < STRETCH_MAP_TEST_LEN && m_iStretchMap[mapPos] != 0)
			str.AppendFormat(_T(" %d"), count);
		else
			str += _T(" -");
		count += m_iStretchMap[mapPos];
		if (++mapPos == m_iStretchMap.size())
			mapPos = 0;
	}
	SetDlgItemText(IDC_STRETCH_TEST, str);
}

void CStretchDlg::OnEnChangeEditStretchMap()
{
	CString str;
	UpdateStretch();

	if (!m_bValid) SetDlgItemText(IDC_STRETCH_TEST, _T(" Invalid stretch map"));
	GetDlgItem(IDOK)->EnableWindow(m_bValid);
	GetDlgItem(IDC_BUTTON_STRETCH_INVERT)->EnableWindow(m_bValid);
}

void CStretchDlg::OnBnClickedCancel()
{
	m_iStretchMap.clear();
	CDialog::OnCancel();
}

void CStretchDlg::OnBnClickedButtonStretchExpand()
{
	SetDlgItemText(IDC_EDIT_STRETCH_MAP, _T("1 0"));
}

void CStretchDlg::OnBnClickedButtonStretchShrink()
{
	SetDlgItemText(IDC_EDIT_STRETCH_MAP, _T("2"));
}

void CStretchDlg::OnBnClickedButtonStretchReset()
{
	SetDlgItemText(IDC_EDIT_STRETCH_MAP, _T("1"));
}

void CStretchDlg::OnBnClickedButtonStretchInvert()
{
	if (!m_bValid) return;
	CString str;
	unsigned int pos = 0;

	while (pos < m_iStretchMap.size()) {
		int x = m_iStretchMap[pos++];
		int y = 0;
		while (m_iStretchMap[pos] == 0) {
			if (pos >= m_iStretchMap.size()) break;
			pos++;
			y++;
		}
		str.AppendFormat(_T(" %d"), y + 1);
		for (int i = 0; i < x - 1; i++)
			str.AppendFormat(_T(" %d"), 0);
	}

	str.Delete(0);
	SetDlgItemText(IDC_EDIT_STRETCH_MAP, str);
}
