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

#include "stdafx.h"
#include "resource.h"

// CGotoDlg dialog

class CGotoDlg : public CDialog
{
	DECLARE_DYNAMIC(CGotoDlg)

public:
	CGotoDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CGotoDlg();

// Dialog Data
	enum { IDD = IDD_GOTO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void CheckDestination() const;
	static int GetChipFromString(const CString str);
	int GetFinalChannel() const;

	unsigned int m_iDestFrame;
	unsigned int m_iDestRow;
	unsigned int m_iDestChip;
	unsigned int m_iDestChannel;

	CComboBox *m_cChipEdit;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnEnChangeEditGotoFrame();
	afx_msg void OnEnChangeEditGotoRow();
	afx_msg void OnEnChangeEditGotoChannel();
	afx_msg void OnCbnSelchangeComboGotoChip();
	afx_msg void OnBnClickedOk();
};
