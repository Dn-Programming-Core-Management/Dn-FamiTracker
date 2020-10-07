/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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


// CAboutDlg, about dialog

class CLinkLabel : public CStatic
{
public:
	CLinkLabel(CString address);
protected:
	DECLARE_MESSAGE_MAP()
	CString m_strAddress;
	bool m_bHover;
public:
	afx_msg HBRUSH CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};


class CHead : public CStatic
{
public:
	CHead();
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void CHead::DrawItem(LPDRAWITEMSTRUCT);
};


class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	~CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CLinkLabel *m_pWeb, *m_pBug;		// // //
	CToolTipCtrl m_wndToolTip;
	
	CFont *m_pLinkFont, *m_pBoldFont, *m_pTitleFont;
	CHead *m_pHead;

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
