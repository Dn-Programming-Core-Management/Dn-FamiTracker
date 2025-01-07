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


// CGrooveDlg dialog

class CGroove;

class CGrooveDlg : public CDialog
{
	DECLARE_DYNAMIC(CGrooveDlg)

public:
	CGrooveDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGrooveDlg();
	
	void SetGrooveIndex(int Index);

// Dialog Data
	enum { IDD = IDD_GROOVE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CFamiTrackerDoc *m_pDocument;

	CGroove* GrooveTable[MAX_GROOVE] = { };
	CGroove* Groove;
	int m_iGrooveIndex, m_iGroovePos;

	CListBox *m_cGrooveTable, *m_cCurrentGroove;

	void ReloadGrooves();
	void UpdateCurrentGroove();
	void UpdateIndicators();
	void ParseGrooveField();

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnLbnSelchangeListGrooveTable();
	afx_msg void OnLbnSelchangeListCurrentGroove();
	afx_msg void OnBnClickedButtonGroovelUp();
	afx_msg void OnBnClickedButtonGroovelDown();
	afx_msg void OnBnClickedButtonGroovelClear();
	afx_msg void OnBnClickedButtonGroovelClearall();
	afx_msg void OnBnClickedButtonGrooveUp();
	afx_msg void OnBnClickedButtonGrooveDown();
	afx_msg void OnBnClickedButtonGrooveCopyFxx();
	afx_msg void OnBnClickedButtonGrooveExpand();
	afx_msg void OnBnClickedButtonGrooveShrink();
	afx_msg void OnBnClickedButtonGrooveGenerate();
	afx_msg void OnBnClickedButtonGroovePad();
	afx_msg void OnBnClickedApply();
};
