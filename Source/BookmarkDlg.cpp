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
#include <vector>
#include "FamiTrackerDoc.h"
#include "FamiTracker.h"
#include "MainFrm.h"
#include "BookmarkDlg.h"

// CBookmarkDlg dialog

IMPLEMENT_DYNAMIC(CBookmarkDlg, CDialog)

CBookmarkDlg::CBookmarkDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBookmarkDlg::IDD, pParent)
{

}

CBookmarkDlg::~CBookmarkDlg()
{
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
END_MESSAGE_MAP()


// CBookmarkDlg message handlers

void CBookmarkDlg::SetBookmarkList()
{
	int Track = static_cast<CMainFrame*>(AfxGetMainWnd())->GetSelectedTrack();
	m_pDocument->SetBookmarkList(Track, m_pBookmarkList);
	LoadBookmarks();
	m_pDocument->SetModifiedFlag();
}

void CBookmarkDlg::LoadBookmarks()
{
	LoadBookmarks(static_cast<CMainFrame*>(AfxGetMainWnd())->GetSelectedTrack());
}

void CBookmarkDlg::LoadBookmarks(int Track)
{
	m_pBookmarkList = m_pDocument->GetBookmarkList(Track);
	m_cListBookmark->ResetContent();

	if (m_pBookmarkList != NULL) for (auto it = m_pBookmarkList->begin(); it < m_pBookmarkList->end(); it++) {
		CString str = *it->Name;
		if (str.IsEmpty()) str = _T("Bookmark");
		str.AppendFormat(_T(" (%02X,%02X)"), it->Frame, it->Row);
		m_cListBookmark->AddString(str);
	}
	m_cListBookmark->SetFocus();
}

void CBookmarkDlg::UpdateSpinControls()
{
	int Track = static_cast<CMainFrame*>(AfxGetMainWnd())->GetSelectedTrack();

	m_cSpinFrame->SetRange(0, m_pDocument->GetFrameCount(Track) - 1);
	if (m_cSpinFrame->GetPos() > static_cast<signed>(m_pDocument->GetFrameCount(Track) - 1))
		m_cSpinFrame->SetPos(m_pDocument->GetFrameCount(Track) - 1);

	m_cSpinRow->SetRange(0, m_pDocument->GetPatternLength(Track) - 1);
	if (m_cSpinRow->GetPos() > static_cast<signed>(m_pDocument->GetPatternLength(Track) - 1))
		m_cSpinRow->SetPos(m_pDocument->GetPatternLength(Track) - 1);
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

	LoadBookmarks();
	UpdateSpinControls();
	m_cSpinHighlight1->SetRange(0, MAX_PATTERN_LENGTH);
	m_cSpinHighlight2->SetRange(0, MAX_PATTERN_LENGTH);

	m_cListBookmark->SetCurSel(-1);

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
				case VK_RETURN:
					OnBnClickedButtonBookmarkUpdate();
					break;
				case VK_DELETE:
					OnBnClickedButtonBookmarkRemove();
					break;
				case VK_UP:
					if (GetKeyState(VK_CONTROL) & 0x8000) {
						OnBnClickedButtonBookmarkMoveup();
						return TRUE;
					}
					break;
				case VK_DOWN:
					if (GetKeyState(VK_CONTROL) & 0x8000) {
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
	stBookmark Mark = {};
	Mark.Frame = m_cSpinFrame->GetPos();
	Mark.Row = m_cSpinRow->GetPos();
	if (Mark.Frame >> 16) return;
	if (Mark.Row >> 16) return;

	Mark.Highlight1 = m_cSpinHighlight1->GetPos();
	Mark.Highlight2 = m_cSpinHighlight2->GetPos();
	Mark.Name = new CString();
	GetDlgItem(IDC_EDIT_BOOKMARK_NAME)->GetWindowText(*Mark.Name);
	Mark.Persist = static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_PERSIST))->GetCheck() == BST_CHECKED;

	m_pBookmarkList->push_back(Mark);
	SetBookmarkList();

	m_cListBookmark->SetCurSel(m_pBookmarkList->size() - 1);
}

void CBookmarkDlg::OnBnClickedButtonBookmarkUpdate()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos == LB_ERR) {
		m_cListBookmark->SetFocus();
		return;
	}
	if (m_cSpinFrame->GetPos() >> 16) return;
	if (m_cSpinRow->GetPos() >> 16) return;
	
	auto it = m_pBookmarkList->begin() + pos;
	it->Frame = m_cSpinFrame->GetPos();
	it->Row = m_cSpinRow->GetPos();
	it->Highlight1 = m_cSpinHighlight1->GetPos();
	it->Highlight2 = m_cSpinHighlight2->GetPos();
	it->Name = new CString();
	GetDlgItem(IDC_EDIT_BOOKMARK_NAME)->GetWindowText(*it->Name);
	it->Persist = static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_PERSIST))->GetCheck() == BST_CHECKED;

	SetBookmarkList();
	m_cListBookmark->SetCurSel(pos);
}

void CBookmarkDlg::OnBnClickedButtonBookmarkRemove()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos == LB_ERR) {
		m_cListBookmark->SetFocus();
		return;
	}

	auto it = m_pBookmarkList->begin() + pos;
	delete it->Name;
	m_pBookmarkList->erase(it);

	SetBookmarkList();
	m_cListBookmark->SetCurSel(pos - 1);
}

void CBookmarkDlg::OnBnClickedButtonBookmarkMoveup()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos == LB_ERR || pos == 0) {
		m_cListBookmark->SetFocus();
		return;
	}
	
	auto it = m_pBookmarkList->begin() + pos;
	stBookmark Temp = *it;
	*it = *(it - 1);
	*(it - 1) = Temp;

	SetBookmarkList();
	m_cListBookmark->SetCurSel(pos - 1);
}

void CBookmarkDlg::OnBnClickedButtonBookmarkMovedown()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos == LB_ERR || pos == m_pBookmarkList->size() - 1) {
		m_cListBookmark->SetFocus();
		return;
	}
	
	auto it = m_pBookmarkList->begin() + pos;
	stBookmark Temp = *it;
	*it = *(it + 1);
	*(it + 1) = Temp;

	SetBookmarkList();
	m_cListBookmark->SetCurSel(pos + 1);
}

void CBookmarkDlg::OnBnClickedButtonBookmarkClearall()
{
	for (auto it = m_pBookmarkList->begin(); it < m_pBookmarkList->end(); it++)
		delete it->Name;
	m_pBookmarkList->clear();
	SetBookmarkList();
}

void CBookmarkDlg::OnBnClickedButtonBookmarkSortp()
{
	std::vector<stBookmark> *List = new std::vector<stBookmark>();

	int Cursor = -1;
	for (auto it = m_pBookmarkList->begin(); it < m_pBookmarkList->end(); it++) {
		auto pos = List->begin();
		while (pos < List->end()) {
			if (pos->Frame > it->Frame || (pos->Frame == it->Frame && pos->Row > it->Row))
				break;
			pos++;
		}
		if (Cursor != -1 && pos - List->begin() <= Cursor)
			Cursor++;
		if (m_cListBookmark->GetCurSel() != LB_ERR && it - m_pBookmarkList->begin() == m_cListBookmark->GetCurSel())
			Cursor = pos - List->begin();
		List->insert(pos, *it);
	}
	
	delete m_pBookmarkList;
	m_pBookmarkList = List;
	SetBookmarkList();
	m_cListBookmark->SetCurSel(Cursor);
}

void CBookmarkDlg::OnBnClickedButtonBookmarkSortn()
{
	std::vector<stBookmark> *List = new std::vector<stBookmark>();
	
	int Cursor = -1;
	for (auto it = m_pBookmarkList->begin(); it < m_pBookmarkList->end(); it++) {
		auto pos = List->begin();
		while (pos < List->end()) {
			if (pos->Name->CollateNoCase(*it->Name) > 0)
				break;
			pos++;
		}
		if (Cursor != -1 && pos - List->begin() <= Cursor)
			Cursor++;
		if (m_cListBookmark->GetCurSel() != LB_ERR && it - m_pBookmarkList->begin() == m_cListBookmark->GetCurSel())
			Cursor = pos - List->begin();
		List->insert(pos, *it);
	}
	
	delete m_pBookmarkList;
	m_pBookmarkList = List;
	SetBookmarkList();
	m_cListBookmark->SetCurSel(Cursor);
}

void CBookmarkDlg::OnLbnSelchangeListBookmarks()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos == LB_ERR) return;

	stBookmark Mark = (*m_pBookmarkList)[pos];
	m_cSpinFrame->SetPos(Mark.Frame);
	m_cSpinRow->SetPos(Mark.Row);
	m_cSpinHighlight1->SetPos(Mark.Highlight1);
	m_cSpinHighlight2->SetPos(Mark.Highlight2);
	static_cast<CButton*>(GetDlgItem(IDC_CHECK_BOOKMARK_PERSIST))->SetCheck(Mark.Persist ? BST_CHECKED : BST_UNCHECKED);
	GetDlgItem(IDC_EDIT_BOOKMARK_NAME)->SetWindowText(*Mark.Name);
}

void CBookmarkDlg::OnLbnDblclkListBookmarks()
{
	int pos = m_cListBookmark->GetCurSel();
	if (pos == LB_ERR) return;
	stBookmark Mark = (*m_pBookmarkList)[pos];

	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(static_cast<CMainFrame*>(AfxGetMainWnd())->GetActiveView());
	//static_cast<CMainFrame*>(AfxGetMainWnd())->SelectTrack(Mark.Track);
	pView->SelectFrame(Mark.Frame);
	pView->SelectRow(Mark.Row);
	pView->SetFocus();
}
