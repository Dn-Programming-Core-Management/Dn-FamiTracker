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

#include "InstrumentEditorSeq.h"
#include "SeqInstrument.h"		// // //
#include "Sequence.h"		// // //
#include "InstrumentManager.h"		// // //
#include "SequenceEditor.h"
#include "SequenceParser.h"		// // //

// // // CInstrumentEditorSeq dialog

/*
LPCTSTR CInstrumentEditorSeq::INST_SETTINGS[] = {_T(""), _T(""), _T(""), _T(""), _T("")};
const int CInstrumentEditorSeq::MAX_VOLUME = 0;
const int CInstrumentEditorSeq::MAX_DUTY = 0;
const inst_type_t CInstrumentEditorSeq::INST_TYPE = INST_NONE;
*/

IMPLEMENT_DYNAMIC(CInstrumentEditorSeq, CSequenceInstrumentEditPanel)
CInstrumentEditorSeq::CInstrumentEditorSeq(CWnd* pParent, TCHAR *Title, LPCTSTR *SeqName, int Vol, int Duty, inst_type_t Type) : 
	CSequenceInstrumentEditPanel(CInstrumentEditorSeq::IDD, pParent),
	m_pTitle(Title),
	m_pSequenceName(SeqName),
	m_iMaxVolume(Vol),
	m_iMaxDuty(Duty),
	m_iInstType(Type)
{
}

void CInstrumentEditorSeq::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CSeqInstrument>(pInst);
	ASSERT(m_pInstrument && m_pInstrument->GetType() == m_iInstType);

	int Sel = m_iSelectedSetting;

	// Update instrument setting list
	if (CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS))) {		// // //
		pList->SetRedraw(FALSE);
		CString str;
		for (int i = 0; i < SEQ_COUNT; ++i) {
			pList->SetCheck(i, m_pInstrument->GetSeqEnable(i));
			str.Format(_T("%i"), m_pInstrument->GetSeqIndex(i));
			pList->SetItemText(i, 1, str);
		}
		pList->SetRedraw();
		pList->RedrawWindow();
	}

	// Setting text box
	SetDlgItemInt(IDC_SEQ_INDEX, m_pInstrument->GetSeqIndex(m_iSelectedSetting = Sel));

	SelectSequence(m_pInstrument->GetSeqIndex(m_iSelectedSetting), m_iSelectedSetting);

	SetFocus();
}

void CInstrumentEditorSeq::SelectSequence(int Sequence, int Type)
{
	// Selects the current sequence in the sequence editor
	m_pSequence = m_pInstrument->GetSequence(Type);		// // //
	if (CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS)))
		pList->SetItemState(Type, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_pSequenceEditor->SelectSequence(m_pSequence, Type, m_iInstType);
	SetupParser();		// // //
}

void CInstrumentEditorSeq::SetupParser() const		// // //
{
	int Max, Min;
	CSeqConversionBase *pConv = nullptr;
	switch (m_iSelectedSetting) {
	case SEQ_VOLUME:
		Max = m_pSequence->GetSetting() == SETTING_VOL_64_STEPS ? 0x3F : m_iMaxVolume; Min = 0; break;
	case SEQ_ARPEGGIO:
		switch (m_pSequence->GetSetting()) {
		case SETTING_ARP_SCHEME:		// // //
			pConv = new CSeqConversionArpScheme {ARPSCHEME_MIN}; break;
		case SETTING_ARP_FIXED:
			pConv = new CSeqConversionArpFixed { }; break;		// // //
		default:
			Max = NOTE_COUNT; Min = -NOTE_COUNT; break;
		}
		break;
	case SEQ_PITCH: case SEQ_HIPITCH:
		Max = 126; Min = -127; break;
	case SEQ_DUTYCYCLE:
		if (m_iInstType == INST_S5B) {
			pConv = new CSeqConversion5B { }; break;
		}
		Max = m_iMaxDuty; Min = 0; break;
	}
	if (pConv == nullptr)
		pConv = new CSeqConversionDefault {Min, Max};
	m_pParser->SetSequence(m_pSequence);
	m_pParser->SetConversion(pConv);
	m_pSequenceEditor->SetConversion(pConv);		// // //
}

void CInstrumentEditorSeq::UpdateSequenceString(bool Changed)		// // //
{
	// Update sequence string
	SetupParser();		// // //
	SetDlgItemText(IDC_SEQUENCE_STRING, m_pParser->PrintSequence().c_str());		// // //
	// If the sequence was changed, assume the user wants to enable it
	if (Changed) {
		static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS))->SetCheck(m_iSelectedSetting, 1);
	}
}

BEGIN_MESSAGE_MAP(CInstrumentEditorSeq, CSequenceInstrumentEditPanel)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_INSTSETTINGS, OnLvnItemchangedInstsettings)
	ON_EN_CHANGE(IDC_SEQ_INDEX, OnEnChangeSeqIndex)
	ON_BN_CLICKED(IDC_FREE_SEQ, OnBnClickedFreeSeq)
	ON_COMMAND(ID_CLONE_SEQUENCE, OnCloneSequence)
END_MESSAGE_MAP()

// CInstrumentSettings message handlers

BOOL CInstrumentEditorSeq::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	SetupDialog(m_pSequenceName);
	m_pSequenceEditor->SetMaxValues(m_iMaxVolume, m_iMaxDuty);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorSeq::OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	auto mask = pNMLV->uNewState & pNMLV->uOldState;		// // // wine compatibility
	pNMLV->uNewState &= ~mask;
	pNMLV->uOldState &= ~mask;

	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));
	if (pNMLV->uChanged & LVIF_STATE && m_pInstrument != NULL) {
		// Selected new setting
		if (pNMLV->uNewState & LVIS_SELECTED || pNMLV->uNewState & LCTRL_CHECKBOX_STATE) {
			m_iSelectedSetting = pNMLV->iItem;
			int Sequence = m_pInstrument->GetSeqIndex(m_iSelectedSetting);
			SetDlgItemInt(IDC_SEQ_INDEX, Sequence);
			SelectSequence(Sequence, m_iSelectedSetting);
			pList->SetSelectionMark(m_iSelectedSetting);
			pList->SetItemState(m_iSelectedSetting, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		}

		// Changed checkbox
		switch(pNMLV->uNewState & LCTRL_CHECKBOX_STATE) {
			case LCTRL_CHECKBOX_CHECKED:	// Checked
				m_pInstrument->SetSeqEnable(m_iSelectedSetting, 1);
				break;
			case LCTRL_CHECKBOX_UNCHECKED:	// Unchecked
				m_pInstrument->SetSeqEnable(m_iSelectedSetting, 0);
				break;
		}
	}

	*pResult = 0;
}

void CInstrumentEditorSeq::OnEnChangeSeqIndex()
{
	// Selected sequence changed
	CListCtrl *pList = static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS));
	int Index = GetDlgItemInt(IDC_SEQ_INDEX);

	if (Index < 0)
		Index = 0;
	if (Index > (MAX_SEQUENCES - 1))
		Index = (MAX_SEQUENCES - 1);

	if (m_pInstrument != nullptr) {
		// Update list
		CString str;		// // //
		str.Format(_T("%i"), Index);
		pList->SetItemText(m_iSelectedSetting, 1, str);
		if (m_pInstrument->GetSeqIndex(m_iSelectedSetting) != Index)
			m_pInstrument->SetSeqIndex(m_iSelectedSetting, Index);
		SelectSequence(Index, m_iSelectedSetting);
	}
}

void CInstrumentEditorSeq::OnBnClickedFreeSeq()
{
	int FreeIndex = m_pInstManager->AddSequence(m_iInstType, m_iSelectedSetting, nullptr, m_pInstrument.get());		// // //
	if (FreeIndex == -1)
		FreeIndex = 0;
	SetDlgItemInt(IDC_SEQ_INDEX, FreeIndex, FALSE);	// Things will update automatically by changing this
}

BOOL CInstrumentEditorSeq::DestroyWindow()
{
	m_pSequenceEditor->DestroyWindow();
	return CDialog::DestroyWindow();
}

void CInstrumentEditorSeq::OnKeyReturn()
{
	// Translate the sequence text string to a sequence
	CString Text;
	GetDlgItemText(IDC_SEQUENCE_STRING, Text);
	TranslateMML(Text);		// // //

	// Enable setting
	static_cast<CListCtrl*>(GetDlgItem(IDC_INSTSETTINGS))->SetCheck(m_iSelectedSetting, 1);
}

void CInstrumentEditorSeq::OnCloneSequence()
{
	CSequence *pSeq = new CSequence(*m_pSequence);		// // // default copy ctor
	int FreeIndex = m_pInstManager->AddSequence(m_iInstType, m_iSelectedSetting, pSeq, m_pInstrument.get());
	if (FreeIndex != -1)
		SetDlgItemInt(IDC_SEQ_INDEX, FreeIndex, FALSE);
	else if (pSeq)
		delete pSeq;
}
