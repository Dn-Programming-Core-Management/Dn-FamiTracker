/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
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
