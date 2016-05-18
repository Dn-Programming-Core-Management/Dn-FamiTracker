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
#include "ConfigShortcuts.h"
#include "Accelerator.h"
#include "Settings.h"
#include <unordered_map>		// // //

// CConfigShortcuts dialog

IMPLEMENT_DYNAMIC(CConfigShortcuts, CPropertyPage)
CConfigShortcuts::CConfigShortcuts() : 
	CPropertyPage(CConfigShortcuts::IDD), 
	m_bShift(false), 
	m_bControl(false), 
	m_bAlt(false), 
	m_iSelectedItem(0),
	m_iKeys(new int[CAccelerator::ACCEL_COUNT]),
	m_iMods(new int[CAccelerator::ACCEL_COUNT])
{
}

CConfigShortcuts::~CConfigShortcuts()
{
	SAFE_RELEASE_ARRAY(m_iKeys);
	SAFE_RELEASE_ARRAY(m_iMods);
}

void CConfigShortcuts::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CConfigShortcuts, CPropertyPage)
	ON_NOTIFY(NM_CLICK, IDC_SHORTCUTS, OnNMClickShortcuts)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SHORTCUTS, OnNMClickShortcuts)
	ON_BN_CLICKED(IDC_DEFAULT, OnBnClickedDefault)
	ON_BN_CLICKED(IDC_CLEAR, &CConfigShortcuts::OnBnClickedClear)
END_MESSAGE_MAP()


// CConfigShortcuts message handlers

BOOL CConfigShortcuts::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CAccelerator *pAccel = theApp.GetAccelerator();
	CListCtrl *pListView = static_cast<CListCtrl*>(GetDlgItem(IDC_SHORTCUTS));

	pListView->DeleteAllItems();
	pListView->InsertColumn(0, _T("Action"), LVCFMT_LEFT, 170);
	pListView->InsertColumn(1, _T("Modifier"), LVCFMT_LEFT, 90);
	pListView->InsertColumn(2, _T("Key"), LVCFMT_LEFT, 110);

	// Build shortcut list
	for (int i = 0; i < CAccelerator::ACCEL_COUNT; ++i) {
		pListView->InsertItem(i, pAccel->GetItemName(i), 0);
		pListView->SetItemText(i, 1, pAccel->GetItemModName(i));
		pListView->SetItemText(i, 2, pAccel->GetItemKeyName(i));

		m_iKeys[i] = pAccel->GetItemKey(i);
		m_iMods[i] = pAccel->GetItemMod(i);
	}
	
	pListView->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	pListView->SetSelectionMark(0);

	m_iSelectedItem = 0;

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CConfigShortcuts::OnNMClickShortcuts(NMHDR *pNMHDR, LRESULT *pResult)
{
	CListCtrl *pListView = static_cast<CListCtrl*>(GetDlgItem(IDC_SHORTCUTS));
	m_iSelectedItem = pListView->GetSelectionMark();
	CString KeyString = AssembleKeyString(m_iMods[m_iSelectedItem], m_iKeys[m_iSelectedItem]);
	SetDlgItemText(IDC_KEY, KeyString);

	*pResult = 0;
}

void CConfigShortcuts::OnBnClickedDefault()
{
	CAccelerator *pAccel = theApp.GetAccelerator();
	
	int Key = pAccel->GetDefaultKey(m_iSelectedItem);
	int Mod = pAccel->GetDefaultMod(m_iSelectedItem);

	StoreKey(m_iSelectedItem, Key, Mod);

	CString KeyString = AssembleKeyString(Mod, Key);
	SetDlgItemText(IDC_KEY, KeyString);

	SetModified();
}

BOOL CConfigShortcuts::OnApply()
{
	CAccelerator *pAccel = theApp.GetAccelerator();
	
	std::unordered_map<int, int> m;		// // // check for conflicts
	for (int i = 0; i < CAccelerator::ACCEL_COUNT; ++i) {
		int KeyVal = (m_iKeys[i] & 0xFF) | (m_iMods[i] << 8);
		if (!KeyVal) continue;
		auto it = m.find(KeyVal);
		if (it == m.end())
			m[KeyVal] = i;
		else {
			CString msg;
			msg.Format(_T("These two commands are assigned to the same shortcut (%s):\n- %s\n- %s"),
					   AssembleKeyString(m_iMods[i], m_iKeys[i]),
					   CAccelerator::DEFAULT_TABLE[it->second].name,
					   CAccelerator::DEFAULT_TABLE[i].name);
			
			AfxMessageBox(msg, MB_ICONERROR);
			return FALSE;
		}
	}

	// Store keys
	for (int i = 0; i < CAccelerator::ACCEL_COUNT; ++i)
		pAccel->StoreShortcut(i, m_iKeys[i], m_iMods[i]);

	pAccel->Setup();

	theApp.UpdateMenuShortcuts();		// // //

	return CPropertyPage::OnApply();
}

BOOL CConfigShortcuts::PreTranslateMessage(MSG* pMsg)
{
	if (GetFocus() == GetDlgItem(IDC_KEY)) {
		switch (pMsg->message) {
			case WM_KEYDOWN:
			case WM_SYSKEYDOWN:
				KeyPressed(pMsg->wParam);
				return TRUE;
			case WM_KEYUP:
			case WM_SYSKEYUP:
				KeyReleased(pMsg->wParam);
				return TRUE;
		}
	}

	return CPropertyPage::PreTranslateMessage(pMsg);
}

void CConfigShortcuts::KeyPressed(int Key)
{
	switch (Key) {
		case VK_SHIFT:
			m_bShift = true;
			return;
		case VK_CONTROL:
			m_bControl = true;
			return;
		case VK_MENU:
			m_bAlt = true;
			return;
	}

	SetupKey(Key);
}

void CConfigShortcuts::KeyReleased(int Key)
{
	switch (Key) {
		case VK_SHIFT:
			m_bShift = false;
			break;
		case VK_CONTROL:
			m_bControl = false;
			break;
		case VK_MENU:
			m_bAlt = false;
			break;
	}
}

void CConfigShortcuts::SetupKey(int Key)
{
	int Mod = (m_bShift ? MOD_SHIFT : 0) | (m_bControl ? MOD_CONTROL : 0) | (m_bAlt ? MOD_ALT : 0);

	StoreKey(m_iSelectedItem, Key, Mod);

	// Display key
	CString KeyStr = AssembleKeyString(Mod, Key);
	SetDlgItemText(IDC_KEY, KeyStr);

	SetModified();
}

void CConfigShortcuts::StoreKey(int Item, int Key, int Mod)
{
	// Store in temp. list
	CAccelerator *pAccel = theApp.GetAccelerator();
	CString KeyName = pAccel->GetVKeyName(Key);

	// Save to list
	CListCtrl *pListView = static_cast<CListCtrl*>(GetDlgItem(IDC_SHORTCUTS));

	pListView->SetItemText(m_iSelectedItem, 1, CAccelerator::MOD_NAMES[Mod]);
	pListView->SetItemText(m_iSelectedItem, 2, KeyName);

	m_iKeys[Item] = Key;
	m_iMods[Item] = Mod;
}

void CConfigShortcuts::OnBnClickedClear()
{
	CListCtrl *pListView = static_cast<CListCtrl*>(GetDlgItem(IDC_SHORTCUTS));
	
	pListView->SetItemText(m_iSelectedItem, 1, CAccelerator::MOD_NAMES[MOD_NONE]);
	pListView->SetItemText(m_iSelectedItem, 2, _T("None"));

	SetDlgItemText(IDC_KEY, _T(""));

	m_iKeys[m_iSelectedItem] = 0;
	m_iMods[m_iSelectedItem] = MOD_NONE;

	SetModified();
}

CString CConfigShortcuts::AssembleKeyString(int Mod, int Key)
{
	CAccelerator *pAccel = theApp.GetAccelerator();
	CString KeyStr;

	if (Mod & MOD_SHIFT) {
		KeyStr.Append(pAccel->GetVKeyName(VK_SHIFT));
		KeyStr.Append(_T(" + "));
	}

	if (Mod & MOD_CONTROL) {
		KeyStr.Append(pAccel->GetVKeyName(VK_CONTROL));
		KeyStr.Append(_T(" + "));
	}

	if (Mod & MOD_ALT) {
		KeyStr.Append(pAccel->GetVKeyName(VK_MENU));
		KeyStr.Append(_T(" + "));
	}

	KeyStr.Append(pAccel->GetVKeyName(Key));

	return KeyStr;
}
