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
** Any permitted reproduction of these routin, in whole or in part,
** must bear this legend.
*/


#include "stdafx.h"
#include "resource.h"
#include "ControlPanelDlg.h"

// This class is mainly used to forward messages to the parent window

// CControlPanelDlg dialog

IMPLEMENT_DYNAMIC(CControlPanelDlg, CDialog)

CControlPanelDlg::CControlPanelDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CControlPanelDlg::IDD, pParent), m_pMainFrame(NULL)
{

}

CControlPanelDlg::~CControlPanelDlg()
{
}

void CControlPanelDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CControlPanelDlg, CDialog)
END_MESSAGE_MAP()


// CControlPanelDlg message handlers

BOOL CControlPanelDlg::PreTranslateMessage(MSG* pMsg)
{
	UpdateDialogControls(this, TRUE);
	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CControlPanelDlg::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// Route command messages to main frame
//	if (nID != IDC_KEYSTEP_SPIN) {
		if (m_pMainFrame->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
//		if (GetParent()->GetParent()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
//			return TRUE;
//	}
	return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

BOOL CControlPanelDlg::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// Route notify messages to main frame
	//*pResult = GetParent()->GetParent()->SendMessage(WM_NOTIFY, wParam, lParam);
	return CDialog::OnNotify(wParam, lParam, pResult);
}

void CControlPanelDlg::SetFrameParent(CWnd *pMainFrm)
{
	// TODO: Use parent instead?
	m_pMainFrame = pMainFrm;
}
