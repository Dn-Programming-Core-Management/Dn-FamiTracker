/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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


// CSpeedDlg dialog

class CSpeedDlg : public CDialog
{
	DECLARE_DYNAMIC(CSpeedDlg)

public:
	CSpeedDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSpeedDlg();

// Dialog Data
	enum { IDD = IDD_SPEED };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	int m_iInitSpeed;
	int m_iSpeed;

	DECLARE_MESSAGE_MAP()
public:
	int GetSpeedFromDlg(int InitialSpeed);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
//	virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCancel();
};
