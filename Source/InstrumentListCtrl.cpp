/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/

#include "stdafx.h"
#include "../resource.h"
#include "FamiTrackerDoc.h"
#include "MainFrm.h"
#include "CustomControls.h"

///
/// CInstrumentList
///

// This class takes care of handling the instrument list, since mapping 
// between instruments list and instruments are not necessarily 1:1

IMPLEMENT_DYNAMIC(CInstrumentList, CListCtrl)

BEGIN_MESSAGE_MAP(CInstrumentList, CListCtrl)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, &CInstrumentList::OnLvnBeginlabeledit)
	ON_NOTIFY_REFLECT(NM_CLICK, &CInstrumentList::OnNMClick)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, &CInstrumentList::OnLvnKeydown)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, &CInstrumentList::OnLvnEndlabeledit)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, &CInstrumentList::OnLvnItemchanged)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CInstrumentList::OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CInstrumentList::OnLvnBegindrag)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()

CInstrumentList::CInstrumentList(CMainFrame *pMainFrame) : 
	m_pMainFrame(pMainFrame),
	m_pDragImage(NULL),
	m_nDragIndex(-1),
	m_nDropIndex(-1),
	m_bDragging(false)
{
}

int CInstrumentList::GetInstrumentIndex(int Selection) const
{
	// Get the instrument number from an item in the list (Selection = list index)
	if (Selection == -1)
		return -1;

	TCHAR Text[CInstrument::INST_NAME_MAX];
	GetItemText(Selection, 0, Text, CInstrument::INST_NAME_MAX);

	int Instrument;
	_stscanf(Text, _T("%X"), &Instrument);

	return Instrument;
}

int CInstrumentList::FindInstrument(int Index) const
{
	// Find the instrument item from the list (Index = instrument number)
	CString Txt;
	Txt.Format(_T("%02X"), Index);

	LVFINDINFO info;
	info.flags = LVFI_PARTIAL | LVFI_STRING;
	info.psz = Txt;

	return FindItem(&info);
}

void CInstrumentList::SelectInstrument(int Index)
{
	// Highlight a specified instrument (Index = instrument number)	
	int ListIndex = FindInstrument(Index);
	SetSelectionMark(ListIndex);
	SetItemState(ListIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	EnsureVisible(ListIndex, FALSE);
}

void CInstrumentList::SelectNextItem()
{
	// Select next instrument
	int SelIndex = GetSelectionMark();
	int Count = GetItemCount();
	if (SelIndex < (Count - 1)) {
		int Slot = GetInstrumentIndex(SelIndex + 1);
		m_pMainFrame->SelectInstrument(Slot);
	}
}

void CInstrumentList::SelectPreviousItem()
{
	// Select previous instrument
	int SelIndex = GetSelectionMark();
	if (SelIndex > 0) {
		int Slot = GetInstrumentIndex(SelIndex - 1);
		m_pMainFrame->SelectInstrument(Slot);
	}
}

void CInstrumentList::InsertInstrument(int Index)
{
	// Inserts an instrument in the list (Index = instrument number)
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();

	if (!pDoc->IsInstrumentUsed(Index))
		return;

	char Name[CInstrument::INST_NAME_MAX];
	pDoc->GetInstrumentName(Index, Name);
	int Type = pDoc->GetInstrumentType(Index);

	// Name is of type index - name
	CString Text;
	Text.Format(_T("%02X - %s"), Index, A2T(Name));
	InsertItem(Index, Text, Type - 1);
	SelectInstrument(Index);		// // //
}

void CInstrumentList::RemoveInstrument(int Index)
{
	// Remove an instrument from the list (Index = instrument number)
	int Selection = FindInstrument(Index);
	if (Selection != -1)
		DeleteItem(Selection);
}

void CInstrumentList::SetInstrumentName(int Index, TCHAR *pName)
{
	// Update instrument name in the list
	int ListIndex = GetSelectionMark();
	CString Name;
	Name.Format(_T("%02X - %s"), Index, pName);
	SetItemText(ListIndex, 0, Name);
}

void CInstrumentList::OnContextMenu(CWnd* pWnd, CPoint point)
{
	int Instrument = GetInstrumentIndex(GetSelectionMark());

	// Select the instrument
	if (Instrument != -1)
		m_pMainFrame->SelectInstrument(Instrument);

	// Display the popup menu
	CMenu *pPopupMenu, PopupMenuBar;
	PopupMenuBar.LoadMenu(IDR_INSTRUMENT_POPUP);
	pPopupMenu = PopupMenuBar.GetSubMenu(0);
	// Route the menu messages to mainframe
	pPopupMenu->TrackPopupMenu(TPM_LEFTBUTTON, point.x, point.y, m_pMainFrame);

	// Return focus to pattern editor
	m_pMainFrame->GetActiveView()->SetFocus();
}

void CInstrumentList::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	
	// Selection changed
//	if (pNMLV->uNewState & LVIS_SELECTED)
//		m_pMainFrame->SelectInstrument(GetInstrumentIndex(pNMLV->iItem));

	*pResult = 0;
}

void CInstrumentList::OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CInstrumentList::OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;
}

void CInstrumentList::OnNMClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	// Select instrument
	m_pMainFrame->SelectInstrument(GetInstrumentIndex(pNMItemActivate->iItem));

	// Move focus to pattern editor 
	m_pMainFrame->GetActiveView()->SetFocus();

	*pResult = 0;
}

void CInstrumentList::OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	// Empty

	*pResult = 0;
}

void CInstrumentList::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	
	// Double-click = instrument editor
	m_pMainFrame->OpenInstrumentEditor();

	*pResult = 0;
}

void CInstrumentList::OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	// Begin drag operation
	m_bDragging = true;
	m_nDragIndex = pNMLV->iItem;
	m_nDropIndex = -1;

	// Create a drag image
	POINT pt;
	int nOffset = 10;
	pt.x = nOffset;
	pt.y = nOffset;

	m_pDragImage = CreateDragImage(m_nDragIndex, &pt);	// Delete this later
	ASSERT(m_pDragImage);

	m_pDragImage->BeginDrag(0, CPoint(nOffset, nOffset));
	m_pDragImage->DragEnter(this, pNMLV->ptAction);
	
	// Capture all mouse messages
	SetCapture();

	*pResult = 0;
}

void CInstrumentList::OnMouseMove(UINT nFlags, CPoint point)
{
	// Handle drag operation
	if (m_bDragging) {
		// Move the drag image
		m_pDragImage->DragMove(point);
		m_pDragImage->DragShowNolock(false);

		// Turn off hilight for previous drop target
		SetItemState(m_nDropIndex, 0, LVIS_DROPHILITED);
		// Redraw previous item
		RedrawItems(m_nDropIndex, m_nDropIndex);

		// Get drop index
		UINT nFlags;
		m_nDropIndex = HitTest(point, &nFlags);

		// Highlight drop index
		if (m_nDropIndex != -1) {
			SetItemState(m_nDropIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
			RedrawItems(m_nDropIndex, m_nDropIndex);
			UpdateWindow();
		}

		m_pDragImage->DragShowNolock(true);
	}

	CListCtrl::OnMouseMove(nFlags, point);
}

void CInstrumentList::OnLButtonUp(UINT nFlags, CPoint point)
{
	// End a drag operation
	if (m_bDragging) {
		ReleaseCapture();
		m_bDragging = false;

		m_pDragImage->DragLeave(this);
		m_pDragImage->EndDrag();
		SAFE_RELEASE(m_pDragImage);

		// Remove highlight
		SetItemState(-1, 0, LVIS_DROPHILITED);
		RedrawItems(-1, -1);
		UpdateWindow();

		if (m_nDropIndex != -1 && m_nDragIndex != m_nDropIndex) {
			// Perform operation
			int First = GetInstrumentIndex(m_nDragIndex);
			int Second = GetInstrumentIndex(m_nDropIndex);
			m_pMainFrame->SwapInstruments(First, Second);
		}
	}

	CListCtrl::OnLButtonUp(nFlags, point);
}
