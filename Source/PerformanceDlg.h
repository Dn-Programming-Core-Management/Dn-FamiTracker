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


// CPerformanceDlg dialog

class CPerformanceDlg : public CDialog
{
	DECLARE_DYNAMIC(CPerformanceDlg)

public:
	CPerformanceDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPerformanceDlg();

// Dialog Data
	enum { IDD = IDD_PERFORMANCE };

private:
	void UpdateBar();
	void UpdateInfo();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	unsigned int PerRefreshRate;		// Refresh rate for performance meter updates

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedOk();
	virtual BOOL DestroyWindow();
	afx_msg void OnClose();
};
