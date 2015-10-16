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

#include <string>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "InstrumentEditDlg.h"
#include "SequenceEditor.h"
#include "InstrumentEditPanel.h"
#include "InstrumentEditor2A03.h"
#include "MainFrm.h"

LPCTSTR CInstrumentEditor2A03::INST_SETTINGS_2A03[] = {
	_T("Volume"), 
	_T("Arpeggio"), 
	_T("Pitch"), 
	_T("Hi-pitch"), 
	_T("Duty / Noise")
};

// CInstrumentSettings dialog

IMPLEMENT_DYNAMIC(CInstrumentEditor2A03, CSequenceInstrumentEditPanel)
CInstrumentEditor2A03::CInstrumentEditor2A03(CWnd* pParent) 
	: CSequenceInstrumentEditPanel(CInstrumentEditor2A03::IDD, pParent),
	m_pInstrument(NULL)
{
}

CInstrumentEditor2A03::~CInstrumentEditor2A03()
{
	SAFE_RELEASE(m_pSequenceEditor);
	
	if (m_pInstrument != NULL)
		m_pInstrument->Release();
}

void CInstrumentEditor2A03::DoDataExchange(CDataExchange* pDX)
{
	CSequenceInstrumentEditPanel::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CInstrumentEditor2A03, CSequenceInstrumentEditPanel)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTSETTINGS, OnLvnItemchangedInstsettings)	
	ON_EN_CHANGE(IDC_SEQ_INDEX, OnEnChangeSeqIndex)
	ON_BN_CLICKED(IDC_FREE_SEQ, OnBnClickedFreeSeq)
	ON_COMMAND(ID_CLONE_SEQUENCE, OnCloneSequence)
END_MESSAGE_MAP()


// CInstrumentSettings message handlers

BOOL CInstrumentEditor2A03::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	SetupDialog(INST_SETTINGS_2A03);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditor2A03::OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	CListCtrl *pList = (CListCtrl*)GetDlgItem(IDC_INSTSETTINGS);

	if (pNMLV->uChanged & LVIF_STATE && m_pInstrument != NULL) {
		// Selected new setting
		if (pNMLV->uNewState & LVNI_SELECTED || pNMLV->uNewState & LCTRL_CHECKBOX_STATE) {
			m_iSelectedSetting = pNMLV->iItem;
			int Sequence = m_pInstrument->GetSeqIndex(m_iSelectedSetting);
			SetDlgItemInt(IDC_SEQ_INDEX, Sequence);
			SelectSequence(Sequence, m_iSelectedSetting);
			pList->SetSelectionMark(m_iSelectedSetting);
			pList->SetItemState(m_iSelectedSetting, LVIS_SELECTED, LVIS_SELECTED);
		}

		// Changed checkbox
		switch(pNMLV->uNewState & LCTRL_CHECKBOX_STATE) {
			case LCTRL_CHECKBOX_CHECKED:
				m_pInstrument->SetSeqEnable(m_iSelectedSetting, 1);
				break;
			case LCTRL_CHECKBOX_UNCHECKED:
				m_pInstrument->SetSeqEnable(m_iSelectedSetting, 0);
				break;
		}
	}

	*pResult = 0;
}

void CInstrumentEditor2A03::OnEnChangeSeqIndex()
{
	// Selected sequence changed
	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));
	int Index = GetDlgItemInt(IDC_SEQ_INDEX);

	if (Index < 0)
		Index = 0;
	if (Index > (MAX_SEQUENCES - 1))
		Index = (MAX_SEQUENCES - 1);

	if (m_pInstrument != NULL) {
		// Update list
		pList->SetItemText(m_iSelectedSetting, 1, MakeIntString(Index));

		if (m_pInstrument->GetSeqIndex(m_iSelectedSetting) != Index)
			m_pInstrument->SetSeqIndex(m_iSelectedSetting, Index);

		SelectSequence(Index, m_iSelectedSetting);
	}
}

void CInstrumentEditor2A03::OnBnClickedFreeSeq()
{
	int FreeIndex = GetDocument()->GetFreeSequence(INST_2A03, m_iSelectedSetting);
	if (FreeIndex == -1)
		FreeIndex = 0;
	SetDlgItemInt(IDC_SEQ_INDEX, FreeIndex, FALSE);	// Things will update automatically by changing this
}

BOOL CInstrumentEditor2A03::DestroyWindow()
{
	m_pSequenceEditor->DestroyWindow();
	return CDialog::DestroyWindow();
}

void CInstrumentEditor2A03::OnKeyReturn()
{
	// Translate the sequence text string to a sequence
	CString Text;
	GetDlgItemText(IDC_SEQUENCE_STRING, Text);

	switch (m_iSelectedSetting) {
		case SEQ_VOLUME:
			TranslateMML(Text, 15, 0);
			break;
		case SEQ_ARPEGGIO:
			if (m_pSequence->GetSetting() == SETTING_ARP_SCHEME)	// // //
				TranslateMML(Text, 36, -27);
			else
				TranslateMML(Text, 96, m_pSequence->GetSetting()== SETTING_ARP_FIXED ? 0 : -96);
			break;
		case SEQ_PITCH:
			TranslateMML(Text, 126, -127);
			break;
		case SEQ_HIPITCH:
			TranslateMML(Text, 126, -127);
			break;
		case SEQ_DUTYCYCLE:
			TranslateMML(Text, 3, 0);
			break;
	}
}

void CInstrumentEditor2A03::SelectInstrument(int Instrument)
{
	CInstrument2A03 *pInstrument = static_cast<CInstrument2A03*>(GetDocument()->GetInstrument(Instrument));
	ASSERT(pInstrument->GetType() == INST_2A03);

	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));

	if (m_pInstrument != NULL)
		m_pInstrument->Release();

	m_pInstrument = NULL;

	// Update instrument setting list
	for (int i = 0; i < SEQ_COUNT; ++i) {
		pList->SetCheck(i, pInstrument->GetSeqEnable(i));
		pList->SetItemText(i, 1, MakeIntString(pInstrument->GetSeqIndex(i)));
	} 

	// Setting text box
	SetDlgItemInt(IDC_SEQ_INDEX, pInstrument->GetSeqIndex(m_iSelectedSetting));

	m_pInstrument = pInstrument;

	// Select new sequence
	SelectSequence(pInstrument->GetSeqIndex(m_iSelectedSetting), m_iSelectedSetting);

	SetFocus();
}

void CInstrumentEditor2A03::SelectSequence(int Sequence, int Type)
{
	// Selects the current sequence in the sequence editor
	m_pSequence = GetDocument()->GetSequence(INST_2A03, unsigned(Sequence), Type);		// // //
	m_pSequenceEditor->SelectSequence(m_pSequence, Type, INST_2A03);
}

void CInstrumentEditor2A03::TranslateMML(CString String, int Max, int Min)
{
	CSequenceInstrumentEditPanel::TranslateMML(String, m_pSequence, Max, Min);

	// Update editor
	m_pSequenceEditor->RedrawWindow();

	// Register a document change
	GetDocument()->SetModifiedFlag();
	GetDocument()->SetExceededFlag();		// // //

	// Enable setting
	static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS))->SetCheck(m_iSelectedSetting, 1);
}

void CInstrumentEditor2A03::SetSequenceString(CString Sequence, bool Changed)
{
	// Update sequence string
	SetDlgItemText(IDC_SEQUENCE_STRING, Sequence);
	// If the sequence was changed, assume the user wants to enable it
	if (Changed) {
		static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS))->SetCheck(m_iSelectedSetting, 1);
	}
}

void CInstrumentEditor2A03::OnCloneSequence()
{
	CFamiTrackerDoc *pDoc = GetDocument();
	int FreeIndex = pDoc->GetFreeSequence(INST_2A03, m_iSelectedSetting);
	if (FreeIndex != -1) {
		CSequence *pSeq = pDoc->GetSequence(INST_2A03, FreeIndex, m_iSelectedSetting);
		pSeq->Copy(m_pSequence);
		SetDlgItemInt(IDC_SEQ_INDEX, FreeIndex, FALSE);
	}
}
