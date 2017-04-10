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

// Source\ChannelsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "TrackerChannel.h"
#include "Source\ChannelsDlg.h"
#include "apu/apu.h"

// Used to handle channels in a future version. Not finished.

const TCHAR *ROOT_ITEMS[] = {
	_T("2A03/2A07"), 
	_T("Konami VRC6"), 
	_T("Konami VRC7"), 
	_T("Nintendo FDS"), 
	_T("Nintendo MMC5"), 
	_T("Namco 106"), 
	_T("Sunsoft 5B")
};

const int CHILD_ITEMS_ID[ROOT_ITEM_COUNT][9] = {
	// 2A03
	{CHANID_SQUARE1, CHANID_SQUARE2, CHANID_TRIANGLE, CHANID_NOISE, CHANID_DPCM},
	// VRC 6
	{CHANID_VRC6_PULSE1, CHANID_VRC6_PULSE2, CHANID_VRC6_SAWTOOTH},
	// VRC 7
	{CHANID_VRC7_CH1, CHANID_VRC7_CH2, CHANID_VRC7_CH3, CHANID_VRC7_CH4, CHANID_VRC7_CH5, CHANID_VRC7_CH6},
	// FDS
	{CHANID_FDS},
	// MMC5
	{CHANID_MMC5_SQUARE1, CHANID_MMC5_SQUARE2},
	// N163
	{CHANID_N163_CH1, CHANID_N163_CH2, CHANID_N163_CH3, CHANID_N163_CH4, CHANID_N163_CH5, CHANID_N163_CH6, CHANID_N163_CH7, CHANID_N163_CH8}, 
	 // S5B
	{CHANID_S5B_CH1, CHANID_S5B_CH2, CHANID_S5B_CH3}
};

const TCHAR *CHILD_ITEMS[ROOT_ITEM_COUNT][9] = {
	// 2A03
	{_T("Square 1"), _T("Square 2"), _T("Triangle"), _T("Noise"), _T("DPCM")},
	// VRC 6
	{_T("Pulse 1"), _T("Pulse 2"), _T("Sawtooth")},
	// VRC 7
	{_T("Channel 1"), _T("Channel 2"), _T("Channel 3"), _T("Channel 4"), _T("Channel 5"), _T("Channel 6")},
	// FDS
	{_T("FDS")},
	// MMC5
	{_T("Square 1"), _T("Square 2")},
	// N163
	{_T("Channel 1"), _T("Channel 2"), _T("Channel 3"), _T("Channel 4"), _T("Channel 5"), _T("Channel 6"), _T("Channel 7"), _T("Channel 8")},
	 // S5B
	{_T("Square 1"), _T("Square 2"), _T("Square 3")}
};

// CChannelsDlg dialog

IMPLEMENT_DYNAMIC(CChannelsDlg, CDialog)

CChannelsDlg::CChannelsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CChannelsDlg::IDD, pParent)
{

}

CChannelsDlg::~CChannelsDlg()
{
}

void CChannelsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CChannelsDlg, CDialog)
	ON_NOTIFY(NM_CLICK, IDC_AVAILABLE_TREE, OnClickAvailable)
	ON_NOTIFY(NM_DBLCLK, IDC_AVAILABLE_TREE, OnDblClickAvailable)
	ON_NOTIFY(NM_DBLCLK, IDC_ADDED_LIST, OnDblClickAdded)
	ON_BN_CLICKED(IDC_MOVE_DOWN, &CChannelsDlg::OnBnClickedMoveDown)
	ON_NOTIFY(NM_RCLICK, IDC_AVAILABLE_TREE, &CChannelsDlg::OnNMRClickAvailableTree)
	ON_BN_CLICKED(IDC_MOVE_UP, &CChannelsDlg::OnBnClickedMoveUp)
END_MESSAGE_MAP()

// CChannelsDlg message handlers

BOOL CChannelsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pAvailableTree = static_cast<CTreeCtrl*>(GetDlgItem(IDC_AVAILABLE_TREE));
	m_pAddedChannels = static_cast<CListCtrl*>(GetDlgItem(IDC_ADDED_LIST));

	int RootItems = sizeof(ROOT_ITEMS) / sizeof(TCHAR);

//	m_pAddedChannels->GetWIndowLon

	m_pAddedChannels->InsertColumn(0, _T("Name"), 0, 150);

	for (int i = 0; i < ROOT_ITEM_COUNT; ++i) {
		HTREEITEM hItem = m_pAvailableTree->InsertItem(ROOT_ITEMS[i]);
		m_hRootItems[i] = hItem;
		for (int j = 0; CHILD_ITEMS[i][j] != NULL; ++j) {
			CString str;
			str.Format(_T("%i: %s"), j + 1, CHILD_ITEMS[i][j]);
			HTREEITEM hChild = m_pAvailableTree->InsertItem(str, hItem);
			m_pAvailableTree->SetItemData(hChild, CHILD_ITEMS_ID[i][j]);
		}
		m_pAvailableTree->SortChildren(hItem);
	}

	CChannelMap *map = theApp.GetChannelMap();

	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	for (unsigned i = 0; i < pDoc->GetAvailableChannels(); ++i) {
		CTrackerChannel *pChannel = pDoc->GetChannel(i);
		AddChannel(pChannel->GetID());
	}
/*
	AddChannel(CHANID_SQUARE1);
	AddChannel(CHANID_SQUARE2);
	AddChannel(CHANID_TRIANGLE);
	AddChannel(CHANID_NOISE);
	AddChannel(CHANID_DPCM);
*/
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CChannelsDlg::OnClickAvailable(NMHDR *pNMHDR, LRESULT *result)
{
	
}

void CChannelsDlg::OnDblClickAvailable(NMHDR *pNMHDR, LRESULT *result)
{
	// Add channel

	HTREEITEM hItem = m_pAvailableTree->GetSelectedItem();

	if ((hItem != NULL) && !m_pAvailableTree->ItemHasChildren(hItem)) {
		InsertChannel(hItem);
	}
}

void CChannelsDlg::OnDblClickAdded(NMHDR *pNMHDR, LRESULT *result)
{
	int Index = m_pAddedChannels->GetSelectionMark();
	int Count = m_pAddedChannels->GetItemCount();

	if (Index != -1 && Count > 1) {

		int ChanID = m_pAddedChannels->GetItemData(Index);

		m_pAvailableTree->GetRootItem();

		// Put back in available list
		for (int i = 0; i < ROOT_ITEM_COUNT; ++i) {
			HTREEITEM hParent = m_hRootItems[i];
			HTREEITEM hItem = m_pAvailableTree->GetNextItem(hParent, TVGN_CHILD);
			for (int j = 0; CHILD_ITEMS[i][j] != NULL; ++j) {
				if (CHILD_ITEMS_ID[i][j] == ChanID) {
					CString str;
					str.Format(_T("%i: %s"), j, CHILD_ITEMS[i][j]);
					HTREEITEM hChild = m_pAvailableTree->InsertItem(str, hParent, hParent);
					m_pAvailableTree->SetItemData(hChild, CHILD_ITEMS_ID[i][j]);
					m_pAvailableTree->Expand(hParent, TVE_EXPAND);
				}
				hItem = m_pAvailableTree->GetNextItem(hItem, TVGN_NEXT);
			}
			m_pAvailableTree->SortChildren(hParent);
		}

		m_pAddedChannels->DeleteItem(Index);
	}
}

void CChannelsDlg::AddChannel(int ChanID)
{
	for (int i = 0; i < ROOT_ITEM_COUNT; ++i) {
		HTREEITEM hItem = m_pAvailableTree->GetNextItem(m_hRootItems[i], TVGN_CHILD);
		for (int j = 0; hItem != NULL; ++j) {

			int ID = m_pAvailableTree->GetItemData(hItem);

			if (ID == ChanID) {
				InsertChannel(hItem);
				return;
			}

			hItem = m_pAvailableTree->GetNextItem(hItem, TVGN_NEXT);
		}
	}
}

void CChannelsDlg::InsertChannel(HTREEITEM hItem)
{
	HTREEITEM hParentItem = m_pAvailableTree->GetParentItem(hItem);

	if (hParentItem != NULL) {

		CString ChanName = m_pAvailableTree->GetItemText(hItem);
		CString ChipName = m_pAvailableTree->GetItemText(hParentItem);

		CString AddStr = ChipName + _T(" :: ") + ChanName.Right(ChanName.GetLength() - 3);

		// Channel ID
		int ChanId = m_pAvailableTree->GetItemData(hItem);

		int ChansAdded = m_pAddedChannels->GetItemCount();
		int Index = m_pAddedChannels->InsertItem(ChansAdded, AddStr);

		m_pAddedChannels->SetItemData(Index, ChanId);

		// Remove channel from available list
		m_pAvailableTree->DeleteItem(hItem);
	}
}

void CChannelsDlg::OnBnClickedMoveDown()
{
	int Index = m_pAddedChannels->GetSelectionMark();

	if (Index >= m_pAddedChannels->GetItemCount() - 1 || Index == -1)
		return;

	CString text = m_pAddedChannels->GetItemText(Index, 0);
	int data = m_pAddedChannels->GetItemData(Index);

	m_pAddedChannels->SetItemText(Index, 0, m_pAddedChannels->GetItemText(Index + 1, 0));
	m_pAddedChannels->SetItemData(Index, m_pAddedChannels->GetItemData(Index + 1));

	m_pAddedChannels->SetItemText(Index + 1, 0, text);
	m_pAddedChannels->SetItemData(Index + 1, data);

	m_pAddedChannels->SetSelectionMark(Index + 1);
	m_pAddedChannels->SetItemState(Index + 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_pAddedChannels->EnsureVisible(Index + 1, FALSE);
}

void CChannelsDlg::OnBnClickedMoveUp()
{
	int Index = m_pAddedChannels->GetSelectionMark();

	if (Index == 0 || Index == -1)
		return;

	CString text = m_pAddedChannels->GetItemText(Index, 0);
	int data = m_pAddedChannels->GetItemData(Index);

	m_pAddedChannels->SetItemText(Index, 0, m_pAddedChannels->GetItemText(Index - 1, 0));
	m_pAddedChannels->SetItemData(Index, m_pAddedChannels->GetItemData(Index - 1));

	m_pAddedChannels->SetItemText(Index - 1, 0, text);
	m_pAddedChannels->SetItemData(Index - 1, data);

	m_pAddedChannels->SetSelectionMark(Index - 1);
	m_pAddedChannels->SetItemState(Index - 1, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_pAddedChannels->EnsureVisible(Index - 1, FALSE);
}

void CChannelsDlg::OnNMRClickAvailableTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	// TODO: Add your control notification handler code here
	*pResult = 0;
}
