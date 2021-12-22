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

// VersionCheckerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "Settings.h"
#include "VersionCheckerDlg.h"
#include "VersionChecker.h"
#include <vector>
#include "str_conv/str_conv.hpp"		// // //

// CVersionCheckerDlg dialog

IMPLEMENT_DYNAMIC(CVersionCheckerDlg, CDialog)

CVersionCheckerDlg::CVersionCheckerDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_VERSION_CHECKER, pParent)
{
}

CVersionCheckerDlg::~CVersionCheckerDlg()
{
}

void CVersionCheckerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CVersionCheckerDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CVersionCheckerDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CVersionCheckerDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CVersionCheckerDlg message handlers

BOOL CVersionCheckerDlg::OnInitDialog()
{
	SetDlgItemText(IDC_VERSIONDATE_MSG, conv::to_t(theApp.m_pVerInfo).data());
	SetDlgItemText(IDC_RELEASE_MSG, conv::to_t(theApp.m_pVerDesc).data());
	if (theApp.m_bStartUp)
		SetDlgItemText(IDC_UPDATE_REMIND, _T("Do not remind me again. (may be re-enabled in config menu)"));
		//set visible the message
	return TRUE;
};

void CVersionCheckerDlg::OnBnClickedCancel()
{
	if (IsDlgButtonChecked(IDC_UPDATE_REMIND) != 0)
		theApp.GetSettings()->General.bCheckVersion = false;
	theApp.m_bStartUp = false;
	CDialog::OnCancel();
}

void CVersionCheckerDlg::OnBnClickedOk()
{
	ShellExecuteW(NULL, L"open", conv::to_wide(theApp.m_pVersionURL).data(), NULL, NULL, SW_SHOWNORMAL);
	if (IsDlgButtonChecked(IDC_UPDATE_REMIND) != 0)
		theApp.GetSettings()->General.bCheckVersion = false;
	theApp.m_bStartUp = false;
	CDialog::OnOK();
}