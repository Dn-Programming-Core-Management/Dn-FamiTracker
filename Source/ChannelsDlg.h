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

const int ROOT_ITEM_COUNT = 7;

// CChannelsDlg dialog

class CChannelsDlg : public CDialog
{
	DECLARE_DYNAMIC(CChannelsDlg)

public:
	CChannelsDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CChannelsDlg();

// Dialog Data
	enum { IDD = IDD_CHANNELS };

protected:
	CTreeCtrl *m_pAvailableTree;
	CListCtrl *m_pAddedChannels;

	HTREEITEM m_hRootItems[ROOT_ITEM_COUNT];

	void AddChannel(int ChanID);
	void InsertChannel(HTREEITEM hItem);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnClickAvailable(NMHDR *pNMHDR, LRESULT *result);
	afx_msg void OnDblClickAvailable(NMHDR *pNMHDR, LRESULT *result);
	afx_msg void OnDblClickAdded(NMHDR *pNMHDR, LRESULT *result);
	afx_msg void OnBnClickedMoveDown();
	afx_msg void OnNMRClickAvailableTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedMoveUp();
};
