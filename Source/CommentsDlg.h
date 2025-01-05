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


// CCommentsDlg dialog

class CCommentsDlg : public CDialog
{
	DECLARE_DYNAMIC(CCommentsDlg)

public:
	CCommentsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCommentsDlg();
	// // //
	CString GetComment() const;
	void SetComment(CString Str);
	bool GetShowOnLoad() const;
	void SetShowOnLoad(bool Enable);
	bool IsChanged() const;

// Dialog Data
	enum { IDD = IDD_COMMENTS };

	static LPCTSTR FONT_FACE;
	static int FONT_SIZE;

protected:
	static RECT WinRect;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void SaveComment();

protected:
	bool m_bChanged;
	CFont *m_pFont;
	// // //
	CString m_sComment;
	bool m_bShowOnLoad;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnInitDialog();
	virtual BOOL DestroyWindow();
	afx_msg void OnEnChangeComments();
	afx_msg void OnBnClickedShowonopen();
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
};
