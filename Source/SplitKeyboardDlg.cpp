/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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

#include "stdafx.h"
#include "../resource.h"
#include "SplitKeyboardDlg.h"
#include "FamiTrackerTypes.h"
#include "PatternNote.h"
#include "FamiTrackerDoc.h"
#include "TrackerChannel.h"

// CSplitKeyboardDlg dialog

const CString KEEP_INST_STRING = _T("Keep");
const int CSplitKeyboardDlg::MAX_TRANSPOSE = 24;

IMPLEMENT_DYNAMIC(CSplitKeyboardDlg, CDialog)

CSplitKeyboardDlg::CSplitKeyboardDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_SPLIT_KEYBOARD, pParent),
	m_bSplitEnable(false),
	m_iSplitChannel(-1),
	m_iSplitNote(-1),
	m_iSplitInstrument(MAX_INSTRUMENTS),
	m_iSplitTranspose(0)
{

}

CSplitKeyboardDlg::~CSplitKeyboardDlg()
{
}

void CSplitKeyboardDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSplitKeyboardDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_SPLIT_ENABLE, OnBnClickedCheckSplitEnable)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_NOTE, OnCbnSelchangeComboSplitNote)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_OCTAVE, OnCbnSelchangeComboSplitNote)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_CHAN, OnCbnSelchangeComboSplitChan)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_INST, OnCbnSelchangeComboSplitInst)
	ON_CBN_SELCHANGE(IDC_COMBO_SPLIT_TRSP, OnCbnSelchangeComboSplitTrsp)
END_MESSAGE_MAP()


// CSplitKeyboardDlg message handlers

BOOL CSplitKeyboardDlg::OnInitDialog()
{
	CComboBox *pCombo;
	CString str;
	const auto pDoc = CFamiTrackerDoc::GetDoc();

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_NOTE));
	for (auto n : stChanNote::NOTE_NAME) {
		if (n.Right(1) == _T("-"))
			n.Delete(1);
		pCombo->AddString(n);
	}
	pCombo->SetCurSel(m_iSplitNote != -1 ? (GET_NOTE(m_iSplitNote) - 1) : 0);
	
	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_OCTAVE));
	for (int i = 0; i < OCTAVE_RANGE; ++i) {
		str.Format(_T("%d"), i);
		pCombo->AddString(str);
	}
	pCombo->SetCurSel(m_iSplitNote != -1 ? GET_OCTAVE(m_iSplitNote) : 3);
	
	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_CHAN));
	pCombo->AddString(KEEP_INST_STRING);
	pCombo->SetCurSel(0);
	for (int i = 0; i < pDoc->GetChannelCount(); ++i) {
		auto pChan = pDoc->GetChannel(i);
		pCombo->AddString(pChan->GetChannelName());
		if (m_iSplitChannel == pChan->GetID())
			pCombo->SetCurSel(i + 1);
	}

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_INST));
	pCombo->AddString(KEEP_INST_STRING);
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		if (pDoc->IsInstrumentUsed(i)) {
			str.Format(_T("%02X"), i);
			pCombo->AddString(str);
		}
	str.Format(_T("%02X"), m_iSplitInstrument);
	if (pCombo->SelectString(-1, str) == CB_ERR)
		pCombo->SelectString(-1, KEEP_INST_STRING);

	pCombo = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_TRSP));
	for (int i = -MAX_TRANSPOSE; i <= MAX_TRANSPOSE; ++i) {
		str.Format(_T("%+d"), i);
		pCombo->AddString(str);
	}
	str.Format(_T("%+d"), m_iSplitTranspose);
	pCombo->SelectString(-1, str);

	CheckDlgButton(IDC_CHECK_SPLIT_ENABLE, m_bSplitEnable ? BST_CHECKED : BST_UNCHECKED);
	OnBnClickedCheckSplitEnable();

	return CDialog::OnInitDialog();
}

void CSplitKeyboardDlg::OnBnClickedCheckSplitEnable()
{
	if (m_bSplitEnable = (IsDlgButtonChecked(IDC_CHECK_SPLIT_ENABLE) == BST_CHECKED)) {
		OnCbnSelchangeComboSplitNote();
		OnCbnSelchangeComboSplitChan();
		OnCbnSelchangeComboSplitInst();
		OnCbnSelchangeComboSplitTrsp();
	}
	GetDlgItem(IDC_COMBO_SPLIT_NOTE)->EnableWindow(m_bSplitEnable);
	GetDlgItem(IDC_COMBO_SPLIT_OCTAVE)->EnableWindow(m_bSplitEnable);
	GetDlgItem(IDC_COMBO_SPLIT_CHAN)->EnableWindow(m_bSplitEnable);
	GetDlgItem(IDC_COMBO_SPLIT_INST)->EnableWindow(m_bSplitEnable);
	GetDlgItem(IDC_COMBO_SPLIT_TRSP)->EnableWindow(m_bSplitEnable);
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitNote()
{
	m_iSplitNote = MIDI_NOTE(
		static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_OCTAVE))->GetCurSel(),
		static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_NOTE))->GetCurSel() + 1
	);
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitChan()
{
	if (int Pos = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_CHAN))->GetCurSel())
		m_iSplitChannel = CFamiTrackerDoc::GetDoc()->GetChannelType(Pos - 1);
	else
		m_iSplitChannel = -1;
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitInst()
{
	CString str;
	GetDlgItemText(IDC_COMBO_SPLIT_INST, str);
	m_iSplitInstrument = str == KEEP_INST_STRING ? MAX_INSTRUMENTS : strtol(str, nullptr, 16);
}

void CSplitKeyboardDlg::OnCbnSelchangeComboSplitTrsp()
{
	m_iSplitTranspose = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_SPLIT_TRSP))->GetCurSel() - MAX_TRANSPOSE;
}
