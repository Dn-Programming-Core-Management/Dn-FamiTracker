/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
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
** Any permitted reproduction of these routin, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "GrooveDlg.h"
#include "PatternEditorTypes.h"
#include "Clipboard.h"


// CGrooveDlg dialog

IMPLEMENT_DYNAMIC(CGrooveDlg, CDialog)

CGrooveDlg::CGrooveDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGrooveDlg::IDD, pParent)
{

}

CGrooveDlg::~CGrooveDlg()
{
	SAFE_RELEASE(m_cGrooveTable);
	SAFE_RELEASE(m_cCurrentGroove);
	for (int i = 0; i < MAX_GROOVE; i++)
		SAFE_RELEASE(GrooveTable[i]);
}

void CGrooveDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CGrooveDlg, CDialog)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDAPPLY, OnBnClickedApply)
	ON_LBN_SELCHANGE(IDC_LIST_GROOVE_TABLE, OnLbnSelchangeListGrooveTable)
	ON_LBN_SELCHANGE(IDC_LIST_GROOVE_EDITOR, OnLbnSelchangeListCurrentGroove)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_UP, OnBnClickedButtonGroovelUp)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_DOWN, OnBnClickedButtonGroovelDown)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_CLEAR, OnBnClickedButtonGroovelClear)
	ON_BN_CLICKED(IDC_BUTTON_GROOVEL_CLEARALL, OnBnClickedButtonGroovelClearall)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_UP, OnBnClickedButtonGrooveUp)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_DOWN, OnBnClickedButtonGrooveDown)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_COPY, OnBnClickedButtonGrooveCopyFxx)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_EXPAND, OnBnClickedButtonGrooveExpand)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_SHRINK, OnBnClickedButtonGrooveShrink)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_GENERATE, OnBnClickedButtonGrooveGenerate)
	ON_BN_CLICKED(IDC_BUTTON_GROOVE_PAD, OnBnClickedButtonGroovePad)
END_MESSAGE_MAP()


// CGrooveDlg message handlers

BOOL CGrooveDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pDocument = static_cast<CFamiTrackerDoc*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument());

	m_cGrooveTable = new CListBox;
	m_cCurrentGroove = new CListBox;
	m_cGrooveTable->SubclassDlgItem(IDC_LIST_GROOVE_TABLE, this);
	m_cCurrentGroove->SubclassDlgItem(IDC_LIST_GROOVE_EDITOR, this);

	CSpinButtonCtrl *SpinPad = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_PAD);
	SpinPad->SetRange(1, 255);
	SpinPad->SetPos(1);
	SetDlgItemText(IDC_EDIT_GROOVE_NUM, _T("12"));
	SetDlgItemText(IDC_EDIT_GROOVE_DENOM, _T("2"));

	ReloadGrooves();
	SetGrooveIndex(0);

	return TRUE;  // return TRUE unless you set the focus to a control
}

BOOL CGrooveDlg::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
	case WM_KEYDOWN:
		switch (pMsg->wParam) {
		case VK_RETURN:	// Return
			pMsg->wParam = 0;
			ParseGrooveField();
			return TRUE;
		case VK_TAB:
		case VK_DOWN:
		case VK_UP:
		case VK_LEFT:
		case VK_RIGHT:
		case VK_SPACE:
			// Do nothing
			break;
		}
		break;
	}

	return CDialog::PreTranslateMessage(pMsg);
}

void CGrooveDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
	if (bShow == TRUE) {
		ReloadGrooves();
		UpdateCurrentGroove();
	}
}

void CGrooveDlg::SetGrooveIndex(int Index)
{
	m_iGrooveIndex = Index;
	m_iGroovePos = 0;
	Groove = GrooveTable[m_iGrooveIndex];
	m_cGrooveTable->SetCurSel(Index);
	m_cCurrentGroove->SetCurSel(0);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedOk()
{
	ShowWindow(SW_HIDE);
	OnBnClickedApply();
	CDialog::OnOK();
}

void CGrooveDlg::OnBnClickedCancel()
{
	ReloadGrooves();
	CDialog::OnCancel();
	ShowWindow(SW_HIDE);
}

void CGrooveDlg::OnBnClickedApply()
{
	m_pDocument->SetModifiedFlag();
	m_pDocument->SetExceededFlag();

	for (int i = 0; i < MAX_GROOVE; i++)
		if (GrooveTable[i]->GetSize())
			m_pDocument->SetGroove(i, GrooveTable[i]);
		else {
			m_pDocument->SetGroove(i, NULL);
			const unsigned Tracks = m_pDocument->GetTrackCount();
			for (unsigned j = 0; j < Tracks; j++)
			if (m_pDocument->GetSongSpeed(j) == i && m_pDocument->GetSongGroove(j)) {
				m_pDocument->SetSongSpeed(j, DEFAULT_SPEED);
				m_pDocument->SetSongGroove(j, false);
			}
		}
}

void CGrooveDlg::OnLbnSelchangeListGrooveTable()
{
	m_iGrooveIndex = m_cGrooveTable->GetCurSel();
	UpdateCurrentGroove();
}

void CGrooveDlg::OnLbnSelchangeListCurrentGroove()
{
	m_iGroovePos = m_cCurrentGroove->GetCurSel();
}

void CGrooveDlg::ReloadGrooves()
{
	SetDlgItemText(IDC_EDIT_GROOVE_FIELD, _T(""));

	m_cGrooveTable->ResetContent();
	m_cCurrentGroove->ResetContent();
	for (int i = 0; i < MAX_GROOVE; i++) {
		bool Used = false;
		SAFE_RELEASE(GrooveTable[i]);
		if (CGroove *orig = m_pDocument->GetGroove(i)) {
			GrooveTable[i] = new CGroove {*orig};
			Used = true;
		}
		else
			GrooveTable[i] = new CGroove { };

		CString String;
		String.Format(_T("%02X%s"), i, Used ? _T(" *") : _T(""));
		m_cGrooveTable->AddString(String);
	}
}

void CGrooveDlg::UpdateCurrentGroove()
{
	CString String;
	CString disp = "";
	
	Groove = GrooveTable[m_iGrooveIndex];
	m_cCurrentGroove->ResetContent();
	for (int i = 0, n = Groove->GetSize(); i < n; ++i) {
		String.Format(_T("%02X: %d"), i, Groove->GetEntry(i));
		disp.AppendFormat(_T("%d "), Groove->GetEntry(i));
		m_cCurrentGroove->InsertString(-1, String);
	}
	m_cCurrentGroove->InsertString(-1, _T("--"));

	m_cCurrentGroove->SetCurSel(m_iGroovePos);
	SetDlgItemText(IDC_EDIT_GROOVE_FIELD, disp);

	String.Format(_T("%02X%s"), m_iGrooveIndex, Groove->GetSize() ? _T(" *") : _T(""));
	m_cGrooveTable->SetRedraw(FALSE);
	m_cGrooveTable->DeleteString(m_iGrooveIndex);
	m_cGrooveTable->InsertString(m_iGrooveIndex, String);
	m_cGrooveTable->SetCurSel(m_iGrooveIndex);
	m_cGrooveTable->SetRedraw(TRUE);

	UpdateIndicators();
}

void CGrooveDlg::UpdateIndicators()
{
	CString String;

	String.Format(_T("Speed: %.3f"), Groove->GetSize() ? Groove->GetAverage() : DEFAULT_SPEED);
	SetDlgItemText(IDC_STATIC_GROOVE_AVERAGE, String);
	String.Format(_T("Size: %d bytes"), Groove->GetSize() ? Groove->GetSize() + 2 : 0);
	SetDlgItemText(IDC_STATIC_GROOVE_SIZE, String);
	int Total = 0;
	for (int i = 0; i < MAX_GROOVE; i++) if (GrooveTable[i]->GetSize())
		Total += GrooveTable[i]->GetSize() + 2;
	String.Format(_T("Total size: %d / 255 bytes"), Total);
	SetDlgItemText(IDC_STATIC_GROOVE_TOTAL, String);

	CWnd *OKButton = GetDlgItem(IDOK);
	CWnd *ApplyButton = GetDlgItem(IDAPPLY);
	if (Total > 255) {
		OKButton->EnableWindow(false);
		ApplyButton->EnableWindow(false);
	}
	else {
		OKButton->EnableWindow(true);
		ApplyButton->EnableWindow(true);
	}
}

void CGrooveDlg::OnBnClickedButtonGroovelUp()
{
	if (m_iGrooveIndex == 0) return;
	CGroove *Temp = Groove;
	GrooveTable[m_iGrooveIndex] = GrooveTable[m_iGrooveIndex - 1];
	GrooveTable[m_iGrooveIndex - 1] = Temp;
	SetGrooveIndex(m_iGrooveIndex - 1);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovelDown()
{
	if (m_iGrooveIndex == MAX_GROOVE - 1) return;
	CGroove *Temp = Groove;
	GrooveTable[m_iGrooveIndex] = GrooveTable[m_iGrooveIndex + 1];
	GrooveTable[m_iGrooveIndex + 1] = Temp;
	SetGrooveIndex(m_iGrooveIndex + 1);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovelClear()
{
	Groove->Clear(0);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovelClearall()
{
	m_cGrooveTable->SetRedraw(FALSE);
	m_cGrooveTable->ResetContent();
	CString str;
	for (int i = 0; i < MAX_GROOVE - 1; i++) {
		GrooveTable[i]->Clear(0);
		str.Format(_T("%02X"), i);
		m_cGrooveTable->AddString(str);
	}
	m_cGrooveTable->SetRedraw(TRUE);
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveUp()
{
	if (m_iGroovePos == 0) return;
	int Temp = Groove->GetEntry(m_iGroovePos);
	Groove->SetEntry(m_iGroovePos, Groove->GetEntry(m_iGroovePos - 1));
	Groove->SetEntry(m_iGroovePos - 1, Temp);

	m_iGroovePos--;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveDown()
{
	if (m_iGroovePos >= Groove->GetSize() - 1) return;
	int Temp = Groove->GetEntry(m_iGroovePos);
	Groove->SetEntry(m_iGroovePos, Groove->GetEntry(m_iGroovePos + 1));
	Groove->SetEntry(m_iGroovePos + 1, Temp);

	m_iGroovePos++;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveCopyFxx()
{
	const unsigned char size = Groove->GetSize();
	if (!size) {
		MessageBeep(MB_ICONWARNING);
		return;
	}

	CClipboard Clipboard(CFamiTrackerView::GetView(), ::RegisterClipboardFormat(CFamiTrackerView::CLIPBOARD_ID));
	if (!Clipboard.IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return;
	}
	
	CPatternClipData *Fxx = new CPatternClipData(1, size);
	Fxx->ClipInfo.Channels    = 1;
	Fxx->ClipInfo.Rows        = size;
	Fxx->ClipInfo.StartColumn = COLUMN_EFF1;
	Fxx->ClipInfo.EndColumn   = COLUMN_EFF1;
	
	unsigned char prev = 0;
	for (unsigned char i = 0; i < Groove->GetSize(); i++) {
		stChanNote row { };
		unsigned char x = Groove->GetEntry(i);
		if (x != prev || !i) {
			row.EffNumber[0] = EF_SPEED;
			row.EffParam[0] = x;
		}
		memcpy(Fxx->GetPattern(0, i), &row, sizeof(stChanNote));
		prev = x;
	}
	
	std::shared_ptr<CPatternClipData> pClipData(Fxx);
	SIZE_T Size = pClipData->GetAllocSize();
	HGLOBAL hMem = Clipboard.AllocMem(Size);
	if (hMem != NULL) {
		pClipData->ToMem(hMem);
		// Set clipboard for internal data, hMem may not be used after this point
		Clipboard.SetData(hMem);
	}
}

void CGrooveDlg::ParseGrooveField()
{
	CString Str;
	GetDlgItemText(IDC_EDIT_GROOVE_FIELD, Str);

	Groove->Clear(0);
	m_iGroovePos = 0;
	while (true) {
		if (Str.IsEmpty()) break;
		Groove->SetSize(Groove->GetSize() + 1);
		int Speed = atoi(Str);
		if (Speed > 255) Speed = 255;
		if (Speed < 1)   Speed = 1;
		Groove->SetEntry(m_iGroovePos, Speed);
		m_iGroovePos++;
		if (m_iGroovePos == MAX_GROOVE_SIZE || Str.FindOneOf(_T("0123456789")) == -1 || Str.Find(_T(' ')) == -1) break;
		Str.Delete(0, Str.Find(_T(' ')) + 1);
	}
	
	m_iGroovePos = 0;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveExpand()
{
	if (Groove->GetSize() > MAX_GROOVE_SIZE / 2) return;
	for (int i = 0; i < Groove->GetSize(); i++)
		if (Groove->GetEntry(i) < 2) return;
	Groove->SetSize(Groove->GetSize() * 2);
	for (int i = Groove->GetSize() - 1; i >= 0; i--)
		Groove->SetEntry(i, Groove->GetEntry(i / 2) / 2 + (i % 2 == 0) * (Groove->GetEntry(i / 2) % 2 == 1));

	m_iGroovePos *= 2;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveShrink()
{
	if (Groove->GetSize() % 2 == 1) return;
	for (int i = 0; i < Groove->GetSize() / 2; i++)
		Groove->SetEntry(i, Groove->GetEntry(i * 2) + Groove->GetEntry(i * 2 + 1));
	Groove->SetSize(Groove->GetSize() / 2);

	m_iGroovePos /= 2;
	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGrooveGenerate()
{
	CString Str;
	GetDlgItemText(IDC_EDIT_GROOVE_NUM, Str);
	int Num = atoi(Str);
	GetDlgItemText(IDC_EDIT_GROOVE_DENOM, Str);
	int Denom = atoi(Str);
	if (Denom < 1 || Denom > MAX_GROOVE_SIZE || Num < Denom || Num > Denom * 255) return;

	Groove->Clear(0);
	Groove->SetSize(Denom);
	for (int i = 0; i < Num * Denom; i += Num)
		Groove->SetEntry(Denom - i / Num - 1, (i + Num) / Denom - i / Denom);

	UpdateCurrentGroove();
}

void CGrooveDlg::OnBnClickedButtonGroovePad()
{
	CString Str;
	GetDlgItemText(IDC_EDIT_GROOVE_PAD, Str);
	int Amount = atoi(Str);
	if (Groove->GetSize() > MAX_GROOVE_SIZE / 2) return;
	for (int i = 0; i < Groove->GetSize(); i++)
		if (Groove->GetEntry(i) <= Amount) return;

	Groove->SetSize(Groove->GetSize() * 2);
	for (int i = Groove->GetSize() - 1; i >= 0; i--)
		Groove->SetEntry(i, i % 2 == 1 ? Amount : Groove->GetEntry(i / 2) - Amount);

	m_iGroovePos *= 2;
	UpdateCurrentGroove();
}