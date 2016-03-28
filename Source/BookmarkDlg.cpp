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

#include "stdafx.h"
#include <string>
#include "FamiTrackerDoc.h"
#include "FamiTracker.h"
#include "MainFrm.h"
#include "Bookmark.h"		// // //
#include "BookmarkCollection.h"		// // //
#include "BookmarkManager.h"		// // // TODO: night not need this
#include "BookmarkDlg.h"

// CBookmarkDlg dialog

IMPLEMENT_DYNAMIC(CBookmarkDlg, CDialog)

CBookmarkDlg::CBookmarkDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBookmarkDlg::IDD, pParent)
{

}

CBookmarkDlg::~CBookmarkDlg()
{
	SAFE_RELEASE(m_cListBookmark);
	SAFE_RELEASE(m_cSpinFrame);
	SAFE_RELEASE(m_cSpinRow);
	SAFE_RELEASE(m_cSpinHighlight1);
	SAFE_RELEASE(m_cSpinHighlight2);
}

void CBookmarkDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CBookmarkDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_BOOKMARK_ADD, OnBnClickedButtonBookmarkAdd)
	ON_BN_CLICKED(IDC_BUTTON_BOOKMARK_UPDATE, OnBnClickedButtonBookmarkUpdate)
	ON_BN_CLICKED(IDC_BUTTON_BOOKMARK_REMOVE, OnBnClickedButtonBookmarkRemove)
	ON_BN_CLICKED(IDC_BUTTON_BOOKMARK_MOVEUP, OnBnClickedButtonBookmarkMoveup)
	ON_BN_CLICKED(IDC_BUTTON_BOOKMARK_MOVEDOWN, OnBnClickedButtonBookmarkMovedown)
	ON_BN_CLICKED(IDC_BUTTON_BOOKMARK_CLEARALL, OnBnClickedButtonBookmarkClearall)
	ON_BN_CLICKED(IDC_BUTTON_BOOKMARK_SORTP, OnBnClickedButtonBookmarkSortp)
	ON_BN_CLICKED(IDC_BUTTON_BOOKMARK_SORTN, OnBnClickedButtonBookmarkSortn)
	ON_LBN_SELCHANGE(IDC_LIST_BOOKMARKS, OnLbnSelchangeListBookmarks)
	ON_LBN_DBLCLK(IDC_LIST_BOOKMARKS, OnLbnDblclkListBookmarks)
	ON_BN_CLICKED(IDC_CHECK_BOOKMARK_HIGH1, OnBnClickedCheckBookmarkHigh1)
	ON_BN_CLICKED(IDC_CHECK_BOOKMARK_HIGH2, OnBnClickedCheckBookmarkHigh2)
	ON_BN_CLICKED(IDC_CHECK_BOOKMARK_PERSIST, OnBnClickedCheckBookmarkPersist)
END_MESSAGE_MAP()


// CBookmarkDlg message handlers

CBookmark *CBookmarkDlg::MakeBookmark() const
{
	CString str;
	GetDlgItem(IDC_EDIT_BOOKMARK_NAME)->GetWindowText(str);

	CBookmark *pMark = new CBookmark(m_cSpinFrame->GetPos(), m_cSpinRow->GetPos());
	pMark->m_Highlight.First = m_bEnableHighlight1 ? m_cSpinHighlight1->GetPos() : -1;
	pMark->m_Highlight.Second = m_bEnableHighlight2 ? m_cSpinHighlight2->GetPos() : -1;
	pMark->m_Highlight.Offset = 0;
	pMark->m_bPersist = m_bPersist;
	pMark->m_sName = std::string(str);

	pMark->m_iFrame %= m_pDocument->GetFrameCount(m_iTrack);
	while (true) {
		unsigned int Len = m_pDocument->GetFrameLength(m_iTrack, pMark->m_iFrame);
		if (pMark->m_iRow < Len) break;
		pMark->m_iRow -= Len;
		if (++pMark->m_iFrame >= m_pDocument->GetFrameCount(m_iTrack))
			pMark->m_iFrame = 0;
	}

	return pMark;
}

void CBookmarkDlg::UpdateBookmarkList()
{
	LoadBookmarks(m_iTrack);
	m_pDocument->UpdateAllViews(NULL, UPDATE_PATTERN);
	m_pDocument->UpdateAllViews(NULL, UPDATE_FRAME);
}

void CBookmarkDlg::LoadBookmarks(int Track)
{
	m_pCollection = m_pManager->GetCollection(m_iTrack = Track);

	m_cListBookmark->ResetContent();
	if (m_pCollection) for (unsigned i = 0; i < m_pCollection->GetCount(); ++i) {
		const CBookmark *pMark = m_pCollection->GetBookmark(i);
		CString str(pMark->m_sName.c_str());
		if (str.IsEmpty()) str = _T("Bookmark");
		str.AppendFormat(_T(" (%02X,%02X)"), pMark->m_iFrame, pMark->m_iRow);
		m_cListBookmark->AddString(str);
	}
}

void CBookmarkDlg::SelectBookmark(int Pos)
{
	int n = m_cListBookmark->SetCurSel(Pos);
	if (n == LB_ERR)
		m_cListBookmark->SetCurSel(-1);
	m_cListBookmark->SetFocus();
	OnLbnSelchangeListBookmarks();
}

void CBookmarkDlg::SetManager(CBookmarkManager *const pManager)
{
	m_pManager = pManager;
}

BOOL CBookmarkDlg::OnInitDialog()
{
	m_cListBookmark = new CListBox();
	m_cSpinFrame = new CSpinButtonCtrl();
	m_cSpinRow = new CSpinButtonCtrl();
	m_cSpinHighlight1 = new CSpinButtonCtrl();
	m_cSpinHighlight2 = new CSpinButtonCtrl();

	m_cListBookmark->SubclassDlgItem(IDC_LIST_BOOKMARKS, this);
	m_cSpinFrame->SubclassDlgItem(IDC_SPIN_BOOKMARK_FRAME, this);
	m_cSpinRow->SubclassDlgItem(IDC_SPIN_BOOKMARK_ROW, this);
	m_cSpinHighlight1->SubclassDlgItem(IDC_SPIN_BOOKMARK_HIGH1, this);
	m_cSpinHighlight2->SubclassDlgItem(IDC_SPIN_BOOKMARK_HIGH2, this);

	m_pDocument = CFamiTrackerDoc::GetDoc();
	m_pManager = nullptr;
	m_iTrack = 0U;

	m_cSpinFrame->SetRange(0, MAX_FRAMES - 1);
	m_cSpinRow->SetRange(0, MAX_PATTERN_LENGTH - 1);
	m_cSpinHighlight1->SetRange(0, MAX_PATTERN_LENGTH);
	m_cSpinHighlight2->SetRange(0, MAX_PATTERN_LENGTH);

	m_cListBookmark->SetCurSel(-1);

	m_bEnableHighlight1 = false;
	m_bEnableHighlight2 = false;
	m_bPersist = false;

	static_cast<CEdit*>(GetDlgItem(IDC_EDIT_BOOKMARK_FRAME))->SetLimitText(3);
	static_cast<CEdit*>(GetDlgItem(IDC_EDIT_BOOKMARK_ROW))->SetLimitText(3);
	static_cast<CEdit*>(GetDlgItem(IDC_EDIT_BOOKMARK_HIGH1))->SetLimitText(3);
	static_cast<CEdit*>(GetDlgItem(IDC_EDIT_BOOKMARK_HIGH2))->SetLimitText(3);

	return CDialog::OnInitDialog();
}

BOOL CBookmarkDlg::PreTranslateMessage(MSG* pMsg)
{
	if (GetFocus() == m_cListBookmark) {
		if (pMsg->message == WM_KEYDOWN) {
			switch (pMsg->wParam) {
				case VK_INSERT:
					OnBnClickedButtonBookmarkAdd();
					break;
				case VK_DELETE:
					OnBnClickedButtonBookmarkRemove();
					break;
				case VK_UP:
					if ((::GetKeyState(VK_CONTROL) & 0x80) == 0x80) {
						OnBnClickedButtonBookmarkMoveup();
						return TRUE;
					}
					break;
				case VK_DOWN:
					if ((::GetKeyState(VK_CONTROL) & 0x80) == 0x80) {
						OnBnClickedButtonBookmarkMovedown();
						return TRUE;
					}
					break;
			}
		}
	}

	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN) return TRUE;

	return CDialog::PreTranslateMessage(pMsg);
}

void CBookmarkDlg::OnBnClickedButtonBookmarkAdd()
{
	CBookmark *pMark = MakeBookmark();
	m_pCollection->AddBookmark(pMark);
	UpdateBookmarkList();
	m_cListBookmark->SetCurSel(m_pCollection->GetCount() - 1);
}

void CBookmarkDlg::OnBnClickedButtonBookmarkUpdate()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos == LB_ERR) return;
	
	CBookmark *pMark = MakeBookmark();
	m_pCollection->SetBookmark(pos, pMark);
	UpdateBookmarkList();
	m_cListBookmark->SetCurSel(pos);
}

void CBookmarkDlg::OnBnClickedButtonBookmarkRemove()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos != LB_ERR) {
		m_pCollection->RemoveBookmark(pos);
		UpdateBookmarkList();
		if (int Count = m_pCollection->GetCount()) {
			if (pos == Count) --pos;
			m_cListBookmark->SetCurSel(pos);
		}
		OnLbnSelchangeListBookmarks();
	}
	m_cListBookmark->SetFocus();
}

void CBookmarkDlg::OnBnClickedButtonBookmarkMoveup()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos != LB_ERR && pos != 0) {
		m_pCollection->SwapBookmarks(pos, pos - 1);
		UpdateBookmarkList();
		m_cListBookmark->SetCurSel(pos - 1);
	}
	m_cListBookmark->SetFocus();
}

void CBookmarkDlg::OnBnClickedButtonBookmarkMovedown()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos != LB_ERR && pos != m_pCollection->GetCount() - 1) {
		m_pCollection->SwapBookmarks(pos, pos + 1);
		UpdateBookmarkList();
		m_cListBookmark->SetCurSel(pos + 1);
	}
	m_cListBookmark->SetFocus();
}

void CBookmarkDlg::OnBnClickedButtonBookmarkClearall()
{
	m_pCollection->ClearBookmarks();
	UpdateBookmarkList();
}

void CBookmarkDlg::OnBnClickedButtonBookmarkSortp()
{
	if (m_pCollection->GetCount()) {
		const CBookmark *pMark = m_pCollection->GetBookmark(m_cListBookmark->GetCurSel());
		m_pCollection->SortByPosition(false);
		UpdateBookmarkList();
		m_cListBookmark->SetCurSel(m_pCollection->GetBookmarkIndex(pMark));
	}
	m_cListBookmark->SetFocus();
}

void CBookmarkDlg::OnBnClickedButtonBookmarkSortn()
{
	if (m_pCollection->GetCount()) {
		const CBookmark *pMark = m_pCollection->GetBookmark(m_cListBookmark->GetCurSel());
		m_pCollection->SortByName(false);
		UpdateBookmarkList();
		m_cListBookmark->SetCurSel(m_pCollection->GetBookmarkIndex(pMark));
	}
	m_cListBookmark->SetFocus();
}

void CBookmarkDlg::OnLbnSelchangeListBookmarks()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos == LB_ERR) return;

	const CBookmark *pMark = m_pCollection->GetBookmark(pos);
	m_bPersist = pMark->m_bPersist;
	static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_PERSIST))->SetCheck(m_bPersist ? BST_CHECKED : BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_BOOKMARK_NAME)->SetWindowText(pMark->m_sName.c_str());

	m_bEnableHighlight1 = pMark->m_Highlight.First != -1;
	m_cSpinHighlight1->SetPos(m_bEnableHighlight1 ? pMark->m_Highlight.First : m_pDocument->GetHighlight().First);
	static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_HIGH1))->SetCheck(m_bEnableHighlight1);
	static_cast<CEdit*>(GetDlgItem(IDC_EDIT_BOOKMARK_HIGH1))->EnableWindow(m_bEnableHighlight1);

	m_bEnableHighlight2 = pMark->m_Highlight.Second != -1;
	m_cSpinHighlight2->SetPos(m_bEnableHighlight2 ? pMark->m_Highlight.Second : m_pDocument->GetHighlight().Second);
	static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_HIGH2))->SetCheck(m_bEnableHighlight2);
	static_cast<CEdit*>(GetDlgItem(IDC_EDIT_BOOKMARK_HIGH2))->EnableWindow(m_bEnableHighlight2);

	static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_PERSIST))->SetCheck(m_bPersist);

	m_cSpinFrame->SetPos(pMark->m_iFrame);
	m_cSpinRow->SetPos(pMark->m_iRow);
}

void CBookmarkDlg::OnLbnDblclkListBookmarks()
{
	CPoint cursor;
	cursor.x = GetCurrentMessage()->pt.x;
	cursor.y = GetCurrentMessage()->pt.y;

	m_cListBookmark->ScreenToClient(&cursor);

	BOOL is_outside = FALSE;
	UINT item_index = m_cListBookmark->ItemFromPoint(cursor, is_outside);
	if (!is_outside) {
		CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(static_cast<CMainFrame*>(AfxGetMainWnd())->GetActiveView());
		CBookmark *pMark = m_pCollection->GetBookmark(item_index);
		
		pView->SelectFrame(pMark->m_iFrame);
		pView->SelectRow(pMark->m_iRow);
		//static_cast<CMainFrame*>(AfxGetMainWnd())->SelectTrack(pMark->m_iTrack);
		pView->SetFocus();
	}
}

void CBookmarkDlg::OnBnClickedCheckBookmarkHigh1()
{
	m_bEnableHighlight1 = static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_HIGH1))->GetCheck() == BST_CHECKED;
	static_cast<CEdit*>(GetDlgItem(IDC_EDIT_BOOKMARK_HIGH1))->EnableWindow(m_bEnableHighlight1);
}

void CBookmarkDlg::OnBnClickedCheckBookmarkHigh2()
{
	m_bEnableHighlight2 = static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_HIGH2))->GetCheck() == BST_CHECKED;
	static_cast<CEdit*>(GetDlgItem(IDC_EDIT_BOOKMARK_HIGH2))->EnableWindow(m_bEnableHighlight2);
}

void CBookmarkDlg::OnBnClickedCheckBookmarkPersist()
{
	m_bPersist = static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_PERSIST))->GetCheck() == BST_CHECKED;
}
