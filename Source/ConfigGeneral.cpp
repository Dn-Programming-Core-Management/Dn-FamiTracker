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
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ConfigGeneral.h"
#include "Settings.h"

const CString CConfigGeneral::CONFIG_STR[] = {		// // //
	_T("Wrap cursor"),
	_T("Wrap across frames"),
	_T("Free cursor edit"),
	_T("Preview wave-files"),
	_T("Key repeat"),
	_T("Show row numbers in hex"),
	_T("Preview next/previous frames"),
	_T("Don't reset DPCM on note stop"),
	_T("Ignore Step when moving"),
	_T("Delete-key pulls up rows"),
	_T("Backup files"),
	_T("Single instance"),
	_T("Preview full row"),
	_T("Don't select on double-click"),
	_T("Warp pattern values"),
	_T("Cut sub-volume"),
	_T("Use old FDS volume table"),
	_T("Retrieve channel state"),
	_T("Overflow paste mode"),
	_T("Show skipped rows"),
	_T("Hexadecimal keypad")
};

const CString CConfigGeneral::CONFIG_DESC[] = {		// // //
	_T("Wrap around the cursor when reaching top or bottom in the pattern editor."),
	_T("Move to next or previous frame when reaching top or bottom in the pattern editor."),
	_T("Unlock the cursor from the center of the pattern editor."),
	_T("Preview wave and DPCM files in the open file dialog when loading samples to the module."),
	_T("Enable key repetition in the pattern editor."),
	_T("Display row numbers and the frame count on the status bar in hexadecimal."),
	_T("Preview next and previous frames in the pattern editor."),
	_T("Prevent resetting the DPCM channel on note stop commands."),
	_T("Ignore the pattern step setting when moving the cursor, only use it when inserting notes."),
	_T("Make delete key pull up rows rather than only deleting the value."),
	_T("Create a backup copy of the existing file when saving a module."),
	_T("Only allow one single instance of the FamiTracker application."),
	_T("Preview all channels when inserting notes in the pattern editor."),
	_T("Don't select the whole channel when double-clicking in the pattern editor."),
	_T("When using Shift + Mouse Wheel to modify a pattern value, allow the parameter to wrap around its limit values."),
	_T("Always silent volume values below 1 caused by the Axy or 7xy effects."),
	_T("Use the existing volume table for the FDS channel which has higher precision than in exported NSFs."),
	_T("Reconstruct the current channel's state from previous frames upon playing (except when playing one row)."),
	_T("Move pasted pattern data outside the rows of the current frame to subsequent frames."),
	_T("Display rows that are truncated by Bxx, Cxx, or Dxx effects."),
	_T("Use the extra keys on the keypad as hexadecimal digits in the pattern editor.")
};

// CConfigGeneral dialog

IMPLEMENT_DYNAMIC(CConfigGeneral, CPropertyPage)
CConfigGeneral::CConfigGeneral()
	: CPropertyPage(CConfigGeneral::IDD)
{
}

CConfigGeneral::~CConfigGeneral()
{
}

void CConfigGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigGeneral, CPropertyPage)
	ON_CBN_EDITUPDATE(IDC_PAGELENGTH, OnCbnEditupdatePagelength)
	ON_CBN_SELENDOK(IDC_PAGELENGTH, OnCbnSelendokPagelength)
	ON_CBN_SELCHANGE(IDC_COMBO_STYLE, OnCbnSelchangeComboStyle)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_CONFIG_LIST, &CConfigGeneral::OnLvnItemchangedConfigList)
END_MESSAGE_MAP()


// CConfigGeneral message handlers

BOOL CConfigGeneral::OnSetActive()
{
	SetDlgItemInt(IDC_PAGELENGTH, m_iPageStepSize, FALSE);
	static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_STYLE))->SetCurSel(m_iEditStyle);		// // //

	return CPropertyPage::OnSetActive();
}

void CConfigGeneral::OnOK()
{
	CPropertyPage::OnOK();
}

BOOL CConfigGeneral::OnApply()
{
	// Translate page length
	BOOL Trans;

	m_iPageStepSize = GetDlgItemInt(IDC_PAGELENGTH, &Trans, FALSE);
	
	if (Trans == FALSE)
		m_iPageStepSize = 4;
	else if (m_iPageStepSize > MAX_PATTERN_LENGTH)
		m_iPageStepSize = MAX_PATTERN_LENGTH;

	theApp.GetSettings()->General.bWrapCursor		= m_bWrapCursor;
	theApp.GetSettings()->General.bWrapFrames		= m_bWrapFrames;
	theApp.GetSettings()->General.bFreeCursorEdit	= m_bFreeCursorEdit;
	theApp.GetSettings()->General.bWavePreview		= m_bPreviewWAV;
	theApp.GetSettings()->General.bKeyRepeat		= m_bKeyRepeat;
	theApp.GetSettings()->General.bRowInHex			= m_bRowInHex;
	theApp.GetSettings()->General.iEditStyle		= m_iEditStyle;
	theApp.GetSettings()->General.bFramePreview		= m_bFramePreview;
	theApp.GetSettings()->General.bNoDPCMReset		= m_bNoDPCMReset;
	theApp.GetSettings()->General.bNoStepMove		= m_bNoStepMove;
	theApp.GetSettings()->General.iPageStepSize		= m_iPageStepSize;
	theApp.GetSettings()->General.bPullUpDelete		= m_bPullUpDelete;
	theApp.GetSettings()->General.bBackups			= m_bBackups;
	theApp.GetSettings()->General.bSingleInstance	= m_bSingleInstance;
	theApp.GetSettings()->General.bPreviewFullRow	= m_bPreviewFullRow;
	theApp.GetSettings()->General.bDblClickSelect	= m_bDisableDblClick;
	// // //
	theApp.GetSettings()->General.bWrapPatternValue	= m_bWrapPatternValue;
	theApp.GetSettings()->General.bCutVolume		= m_bCutVolume;
	theApp.GetSettings()->General.bFDSOldVolume		= m_bFDSOldVolume;
	theApp.GetSettings()->General.bRetrieveChanState = m_bRetrieveChanState;
	theApp.GetSettings()->General.bOverflowPaste	= m_bOverflowPaste;
	theApp.GetSettings()->General.bShowSkippedRows	= m_bShowSkippedRows;
	theApp.GetSettings()->General.bHexKeypad		= m_bHexKeypad;

	theApp.GetSettings()->Keys.iKeyNoteCut			= m_iKeyNoteCut;
	theApp.GetSettings()->Keys.iKeyNoteRelease		= m_iKeyNoteRelease;
	theApp.GetSettings()->Keys.iKeyClear			= m_iKeyClear;
	theApp.GetSettings()->Keys.iKeyRepeat			= m_iKeyRepeat;
	theApp.GetSettings()->Keys.iKeyEchoBuffer		= m_iKeyEchoBuffer;		// // //

	return CPropertyPage::OnApply();
}

BOOL CConfigGeneral::OnInitDialog()
{
	char Text[64];

	CPropertyPage::OnInitDialog();

	m_bWrapCursor		= theApp.GetSettings()->General.bWrapCursor;
	m_bWrapFrames		= theApp.GetSettings()->General.bWrapFrames;
	m_bFreeCursorEdit	= theApp.GetSettings()->General.bFreeCursorEdit;
	m_bPreviewWAV		= theApp.GetSettings()->General.bWavePreview;
	m_bKeyRepeat		= theApp.GetSettings()->General.bKeyRepeat;
	m_bRowInHex			= theApp.GetSettings()->General.bRowInHex;
	m_iEditStyle		= theApp.GetSettings()->General.iEditStyle;
	m_bFramePreview		= theApp.GetSettings()->General.bFramePreview;
	m_bNoDPCMReset		= theApp.GetSettings()->General.bNoDPCMReset;
	m_bNoStepMove		= theApp.GetSettings()->General.bNoStepMove;
	m_iPageStepSize		= theApp.GetSettings()->General.iPageStepSize;
	m_bPullUpDelete		= theApp.GetSettings()->General.bPullUpDelete;
	m_bBackups			= theApp.GetSettings()->General.bBackups;
	m_bSingleInstance	= theApp.GetSettings()->General.bSingleInstance;
	m_bPreviewFullRow	= theApp.GetSettings()->General.bPreviewFullRow;
	m_bDisableDblClick	= theApp.GetSettings()->General.bDblClickSelect;
	// // //
	m_bWrapPatternValue = theApp.GetSettings()->General.bWrapPatternValue;
	m_bCutVolume		= theApp.GetSettings()->General.bCutVolume;
	m_bFDSOldVolume		= theApp.GetSettings()->General.bFDSOldVolume;
	m_bRetrieveChanState = theApp.GetSettings()->General.bRetrieveChanState;
	m_bOverflowPaste	= theApp.GetSettings()->General.bOverflowPaste;
	m_bShowSkippedRows	= theApp.GetSettings()->General.bShowSkippedRows;
	m_bHexKeypad		= theApp.GetSettings()->General.bHexKeypad;

	m_iKeyNoteCut		= theApp.GetSettings()->Keys.iKeyNoteCut; 
	m_iKeyNoteRelease	= theApp.GetSettings()->Keys.iKeyNoteRelease; 
	m_iKeyClear			= theApp.GetSettings()->Keys.iKeyClear; 
	m_iKeyRepeat		= theApp.GetSettings()->Keys.iKeyRepeat;
	m_iKeyEchoBuffer	= theApp.GetSettings()->Keys.iKeyEchoBuffer;		// // //

	GetKeyNameText(MapVirtualKey(m_iKeyNoteCut, MAPVK_VK_TO_VSC) << 16, Text, 64);
	SetDlgItemText(IDC_KEY_NOTE_CUT, Text);
	GetKeyNameText(MapVirtualKey(m_iKeyNoteRelease, MAPVK_VK_TO_VSC) << 16, Text, 64);
	SetDlgItemText(IDC_KEY_NOTE_RELEASE, Text);
	GetKeyNameText(MapVirtualKey(m_iKeyClear, MAPVK_VK_TO_VSC) << 16, Text, 64);
	SetDlgItemText(IDC_KEY_CLEAR, Text);
	GetKeyNameText(MapVirtualKey(m_iKeyRepeat, MAPVK_VK_TO_VSC) << 16, Text, 64);
	SetDlgItemText(IDC_KEY_REPEAT, Text);
	GetKeyNameText(MapVirtualKey(m_iKeyEchoBuffer, MAPVK_VK_TO_VSC) << 16, Text, 64);		// // //
	SetDlgItemText(IDC_KEY_ECHO_BUFFER, Text);

	EnableToolTips(TRUE);

	m_wndToolTip.Create(this, TTS_ALWAYSTIP);
	m_wndToolTip.Activate(TRUE);

	CWnd *pWndChild = GetWindow(GW_CHILD);
	CString strToolTip;

	while (pWndChild) {
		int nID = pWndChild->GetDlgCtrlID();
		if (strToolTip.LoadString(nID)) {
			m_wndToolTip.AddTool(pWndChild, strToolTip);
		}
		pWndChild = pWndChild->GetWindow(GW_HWNDNEXT);
	}

	const bool CONFIG_BOOL[SETTINGS_BOOL_COUNT] = {		// // //
		m_bWrapCursor,
		m_bWrapFrames,
		m_bFreeCursorEdit,
		m_bPreviewWAV,
		m_bKeyRepeat,
		m_bRowInHex,
		m_bFramePreview,
		m_bNoDPCMReset,
		m_bNoStepMove,
		m_bPullUpDelete,
		m_bBackups,
		m_bSingleInstance,
		m_bPreviewFullRow,
		m_bDisableDblClick,
		m_bWrapPatternValue,
		m_bCutVolume,
		m_bFDSOldVolume,
		m_bRetrieveChanState,
		m_bOverflowPaste,
		m_bShowSkippedRows,
		m_bHexKeypad
	};

	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_CONFIG_LIST));
	pList->DeleteAllItems();
	pList->InsertColumn(0, _T(""), LVCFMT_LEFT, 18);
	pList->InsertColumn(1, _T("Option"), LVCFMT_LEFT, 160);
	pList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
	pList->SetItemState(0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	
	for (int i = SETTINGS_BOOL_COUNT - 1; i > -1; i--) {
		pList->InsertItem(0, _T(""), 0);
		pList->SetCheck(0, CONFIG_BOOL[i]);
		pList->SetItemText(0, 1, CONFIG_STR[i]);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigGeneral::OnCbnEditupdatePagelength()
{
	SetModified();
}

void CConfigGeneral::OnCbnSelendokPagelength()
{
	SetModified();
}

void CConfigGeneral::OnCbnSelchangeComboStyle()		// // //
{
	m_iEditStyle = static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_STYLE))->GetCurSel();
	SetModified();
}

void CConfigGeneral::OnLvnItemchangedConfigList(NMHDR *pNMHDR, LRESULT *pResult)		// // //
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_CONFIG_LIST));

	bool *const CONFIG_BOOL[SETTINGS_BOOL_COUNT] = {		// // //
		&m_bWrapCursor,
		&m_bWrapFrames,
		&m_bFreeCursorEdit,
		&m_bPreviewWAV,
		&m_bKeyRepeat,
		&m_bRowInHex,
		&m_bFramePreview,
		&m_bNoDPCMReset,
		&m_bNoStepMove,
		&m_bPullUpDelete,
		&m_bBackups,
		&m_bSingleInstance,
		&m_bPreviewFullRow,
		&m_bDisableDblClick,
		&m_bWrapPatternValue,
		&m_bCutVolume,
		&m_bFDSOldVolume,
		&m_bRetrieveChanState,
		&m_bOverflowPaste,
		&m_bShowSkippedRows,
		&m_bHexKeypad
	};
	
	if (pNMLV->uChanged & LVIF_STATE) {
		if (pNMLV->uNewState & LVNI_SELECTED || pNMLV->uNewState & 0x3000) {
			CString str;
			CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_CONFIG_LIST));
			str.Format(_T("Description: %s"), CONFIG_DESC[pNMLV->iItem]);
			SetDlgItemText(IDC_EDIT_CONFIG_DESC, str);
			
			pList->SetSelectionMark(pNMLV->iItem);
			if (pNMLV->iItem >= 0 && pNMLV->iItem < SETTINGS_BOOL_COUNT)
				pList->SetItemState(pNMLV->iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}

		if (pNMLV->uNewState & 0x3000) {
			SetModified();
			for (int i = 0; i < SETTINGS_BOOL_COUNT; i++)
				*CONFIG_BOOL[i] = pList->GetCheck(i) != 0;
		}
	}

	*pResult = 0;
}

BOOL CConfigGeneral::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN) {
		char Text[64];
		int id = GetFocus()->GetDlgCtrlID();
		int key = pMsg->wParam;

		if (key == 27)		// ESC
			key = 0;

		switch (id) {
			case IDC_KEY_NOTE_CUT:
				m_iKeyNoteCut = key;
				break;
			case IDC_KEY_NOTE_RELEASE:
				m_iKeyNoteRelease = key;
				break;
			case IDC_KEY_CLEAR:
				m_iKeyClear = key;
				break;
			case IDC_KEY_REPEAT:
				m_iKeyRepeat = key;
				break;
			case IDC_KEY_ECHO_BUFFER:		// // //
				m_iKeyEchoBuffer = key;
				break;
			default:
				return CPropertyPage::PreTranslateMessage(pMsg);
		}

		GetKeyNameText(key ? pMsg->lParam : 0, Text, 64);
		SetDlgItemText(id, Text);

		SetModified();

		return TRUE;
	}

	m_wndToolTip.RelayEvent(pMsg);

	return CPropertyPage::PreTranslateMessage(pMsg);
}
