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

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "MainFrm.h"
#include "Source\CommentsDlg.h"

// CCommentsDlg dialog

// Font
LPCTSTR CCommentsDlg::FONT_FACE = _T("Courier");
int		CCommentsDlg::FONT_SIZE = 12;

RECT CCommentsDlg::WinRect;

IMPLEMENT_DYNAMIC(CCommentsDlg, CDialog)

CCommentsDlg::CCommentsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCommentsDlg::IDD, pParent), m_pFont(NULL)
{
}

CCommentsDlg::~CCommentsDlg()
{
	SAFE_RELEASE(m_pFont);
}

void CCommentsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CCommentsDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CCommentsDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CCommentsDlg::OnBnClickedCancel)
	ON_WM_SIZE()
	ON_EN_CHANGE(IDC_COMMENTS, &CCommentsDlg::OnEnChangeComments)
	ON_BN_CLICKED(IDC_SHOWONOPEN, &CCommentsDlg::OnBnClickedShowonopen)
	ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()


// CCommentsDlg message handlers

void CCommentsDlg::SaveComment()
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CString comment;

	GetDlgItemText(IDC_COMMENTS, comment);

	pDoc->SetComment(comment, IsDlgButtonChecked(IDC_SHOWONOPEN) == BST_CHECKED);
}

void CCommentsDlg::OnBnClickedOk()
{
	if (m_bChanged)
		SaveComment();

	EndDialog(0);
}

void CCommentsDlg::OnBnClickedCancel()
{
	EndDialog(0);
}

void CCommentsDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	CWnd *pEdit = GetDlgItem(IDC_COMMENTS);
	CWnd *pOk = GetDlgItem(IDOK);
	CWnd *pCancel = GetDlgItem(IDCANCEL);
	CWnd *pCheckBox = GetDlgItem(IDC_SHOWONOPEN);

	CRect dlgRect;
	GetClientRect(dlgRect);

	if (pEdit != NULL) {
		dlgRect.bottom -= 39;
		pEdit->MoveWindow(dlgRect);
		CRect buttonRect;
		pOk->GetClientRect(buttonRect);
		buttonRect.MoveToY(dlgRect.bottom + 8);
		buttonRect.MoveToX(dlgRect.right - buttonRect.Width() * 2 - 10);
		pOk->MoveWindow(buttonRect);
		pCancel->GetClientRect(buttonRect);
		buttonRect.MoveToY(dlgRect.bottom + 8);
		buttonRect.MoveToX(dlgRect.right - buttonRect.Width() - 7);
		pCancel->MoveWindow(buttonRect);
		pCheckBox->GetClientRect(buttonRect);
		buttonRect.MoveToY(dlgRect.bottom + 14);
		buttonRect.MoveToX(8);
		pCheckBox->MoveWindow(buttonRect);
	}
}

BOOL CCommentsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
	CString comment = pDoc->GetComment();

	SetDlgItemText(IDC_COMMENTS, comment);

	m_bChanged = false;

	m_pFont = new CFont();
	m_pFont->CreateFont(FONT_SIZE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, FONT_FACE);

	CEdit *pEdit = (CEdit*)GetDlgItem(IDC_COMMENTS);
	pEdit->SetFont(m_pFont);

	CheckDlgButton(IDC_SHOWONOPEN, pDoc->ShowCommentOnOpen() ? BST_CHECKED : BST_UNCHECKED);

	if (WinRect.top == 0 && WinRect.bottom == 0) {
		GetWindowRect(&WinRect);
	}
	else {
		MoveWindow(WinRect.left, WinRect.top, WinRect.right - WinRect.left, WinRect.bottom - WinRect.top);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CCommentsDlg::DestroyWindow()
{
	GetWindowRect(&WinRect);
	return CDialog::DestroyWindow();
}

void CCommentsDlg::OnEnChangeComments()
{
	m_bChanged = true;
}

void CCommentsDlg::OnBnClickedShowonopen()
{
	m_bChanged = true;
}

void CCommentsDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = 400;
	lpMMI->ptMinTrackSize.y = 200;

	CDialog::OnGetMinMaxInfo(lpMMI);
}
