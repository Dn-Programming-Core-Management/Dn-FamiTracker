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


class CFamiTrackerDoc;

// CTransposeDlg dialog

class CTransposeDlg : public CDialog
{
	DECLARE_DYNAMIC(CTransposeDlg)

public:
	CTransposeDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTransposeDlg();

	void SetTrack(unsigned int Track);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TRANSPOSE };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

private:
	void Transpose(int Trsp, unsigned int Track);
	
	CFamiTrackerDoc *m_pDocument;
	int m_iTrack;

	static bool s_bDisableInst[MAX_INSTRUMENTS];
	static const UINT BUTTON_ID;

	CButton **m_cInstButton;
	CFont *m_pFont;
	
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedInst(UINT nID);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButtonTrspReverse();
	afx_msg void OnBnClickedButtonTrspClear();
};
