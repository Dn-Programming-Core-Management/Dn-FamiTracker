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

#pragma once

// CVersionCheckerDlg dialog

class CVersionCheckerDlg : public CDialog
{
	DECLARE_DYNAMIC(CVersionCheckerDlg)

public:
	CVersionCheckerDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CVersionCheckerDlg();


	enum { IDD = IDD_VERSION_CHECKER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CString UpdateURL;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};