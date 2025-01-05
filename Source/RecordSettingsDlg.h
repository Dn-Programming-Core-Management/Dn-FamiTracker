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


// CRecordSettingsDlg dialog

struct stRecordSetting;

class CRecordSettingsDlg : public CDialog
{
	DECLARE_DYNAMIC(CRecordSettingsDlg)

public:
	CRecordSettingsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CRecordSettingsDlg();

	stRecordSetting *GetRecordSetting();

// Dialog Data
	enum { IDD = IDD_RECORD_SETTINGS };

protected:
	int m_iInterval;
	int m_iCount;
	bool m_bReset;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEditRecorderInterval();
	afx_msg void OnEnChangeEditRecorderCount();
	afx_msg void OnBnClickedCheckRecorderReset();
	afx_msg void OnEnKillfocusEditRecorderInterval();
	afx_msg void OnEnKillfocusEditRecorderCount();
};
