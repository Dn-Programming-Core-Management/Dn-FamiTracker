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

class CFamiTrackerDoc;
class CBookmark;
class CBookmarkCollection;
class CBookmarkManager;


// CBookmarkDlg dialog

class CBookmarkDlg : public CDialog
{
	DECLARE_DYNAMIC(CBookmarkDlg)

public:
	CBookmarkDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CBookmarkDlg();

	void LoadBookmarks(int Track);
	void SelectBookmark(int Pos);
	void SetManager(CBookmarkManager *const pManager);

// Dialog Data
	enum { IDD = IDD_BOOKMARKS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CBookmark *MakeBookmark() const;
	void UpdateBookmarkList();

	CListBox *m_cListBookmark;
	CSpinButtonCtrl *m_cSpinFrame;
	CSpinButtonCtrl *m_cSpinRow;
	CSpinButtonCtrl *m_cSpinHighlight1;
	CSpinButtonCtrl *m_cSpinHighlight2;

	CFamiTrackerDoc *m_pDocument;
	CBookmarkManager *m_pManager;
	unsigned m_iTrack;

	CBookmarkCollection *m_pCollection;
	bool m_bEnableHighlight1;
	bool m_bEnableHighlight2;
	bool m_bPersist;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnBnClickedButtonBookmarkAdd();
	afx_msg void OnBnClickedButtonBookmarkUpdate();
	afx_msg void OnBnClickedButtonBookmarkRemove();
	afx_msg void OnBnClickedButtonBookmarkMoveup();
	afx_msg void OnBnClickedButtonBookmarkMovedown();
	afx_msg void OnBnClickedButtonBookmarkClearall();
	afx_msg void OnBnClickedButtonBookmarkSortp();
	afx_msg void OnBnClickedButtonBookmarkSortn();
	afx_msg void OnLbnSelchangeListBookmarks();
	afx_msg void OnLbnDblclkListBookmarks();
	afx_msg void OnBnClickedCheckBookmarkHigh1();
	afx_msg void OnBnClickedCheckBookmarkHigh2();
	afx_msg void OnBnClickedCheckBookmarkPersist();
};
