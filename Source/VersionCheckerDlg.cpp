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
	if (theApp.m_bNewVersion) {
		SetDlgItemText(IDC_VERSION_STATIC, "A new version of Dn-FamiTracker is now available.");
		SetDlgItemText(IDC_VERSIONDATE_MSG, conv::to_t(theApp.m_pVerInfo).data());
		SetDlgItemText(IDC_RELEASE_MSG, conv::to_t(theApp.m_pVerDesc).data());
		SetDlgItemText(IDC_VERSION_STATIC2, "Release notes:");
		SetDlgItemText(IDC_VERSION_STATIC3, "Pressing \"Update\" will open the GitHub page for this release.");
	}
	else {
		SetDlgItemText(IDC_VERSION_STATIC, "A new version of Dn-FamiTracker has not been found.");
		SetDlgItemText(IDC_VERSIONDATE_MSG, "You might be on the latest version.");
		SetDlgItemText(IDC_VERSION_STATIC2, "If not, go to the latest release to download the latest version.");
		SetDlgItemText(IDC_VERSION_STATIC3, "Pressing \"Update\" will open the GitHub page for the latest release.");
		SetDlgItemText(IDC_RELEASE_MSG, "");
		GetDlgItem(IDC_RELEASE_MSG)->EnableWindow(false);
	}
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