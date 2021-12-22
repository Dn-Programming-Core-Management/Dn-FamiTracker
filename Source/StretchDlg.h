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

#pragma once

#define STRETCH_MAP_TEST_LEN 16


// CStretchDlg dialog

class CStretchDlg : public CDialog
{
	DECLARE_DYNAMIC(CStretchDlg)

public:
	CStretchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CStretchDlg();

// Dialog Data
	enum { IDD = IDD_STRETCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void UpdateStretch();
	void UpdateTest();

	std::vector<int> m_iStretchMap;
	bool m_bValid;

	DECLARE_MESSAGE_MAP()
public:
	std::vector<int> GetStretchMap();
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEditStretchMap();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButtonStretchExpand();
	afx_msg void OnBnClickedButtonStretchShrink();
	afx_msg void OnBnClickedButtonStretchReset();
	afx_msg void OnBnClickedButtonStretchInvert();
};
