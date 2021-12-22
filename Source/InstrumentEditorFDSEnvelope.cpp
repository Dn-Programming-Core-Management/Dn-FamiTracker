/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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
#include "../resource.h"        // // //
#include "Instrument.h"
#include "SeqInstrument.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "Sequence.h"
#include "InstrumentEditPanel.h"
#include "SequenceEditor.h"
#include "InstrumentEditorFDSEnvelope.h"
#include "SequenceParser.h"		// // //
#include "DPI.h"		// // //

// CInstrumentEditorFDSEnvelope dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorFDSEnvelope, CSequenceInstrumentEditPanel)

CInstrumentEditorFDSEnvelope::CInstrumentEditorFDSEnvelope(CWnd* pParent) :
	CSequenceInstrumentEditPanel(CInstrumentEditorFDSEnvelope::IDD, pParent)
{
}

CInstrumentEditorFDSEnvelope::~CInstrumentEditorFDSEnvelope()
{
}

void CInstrumentEditorFDSEnvelope::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

void CInstrumentEditorFDSEnvelope::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CInstrumentFDS>(pInst);
	ASSERT(m_pInstrument);
	
	LoadSequence();
	
	SetFocus();
}


BEGIN_MESSAGE_MAP(CInstrumentEditorFDSEnvelope, CInstrumentEditPanel)
	ON_CBN_SELCHANGE(IDC_TYPE, &CInstrumentEditorFDSEnvelope::OnCbnSelchangeType)
END_MESSAGE_MAP()

// CInstrumentEditorFDSEnvelope message handlers

BOOL CInstrumentEditorFDSEnvelope::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	CRect rect;
	GetClientRect(&rect);
	rect.DeflateRect(DPI::SX(10), DPI::SY(10), DPI::SX(10), DPI::SY(45));		// // //

	m_pSequenceEditor = new CSequenceEditor();		// // //
	m_pSequenceEditor->CreateEditor(this, rect);
	m_pSequenceEditor->SetMaxValues(MAX_VOLUME, 0);

	static_cast<CComboBox*>(GetDlgItem(IDC_TYPE))->SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorFDSEnvelope::UpdateSequenceString(bool Changed)		// // //
{
	SetupParser();		// // //
	SetDlgItemText(IDC_SEQUENCE_STRING, m_pParser->PrintSequence().c_str());		// // //
}

void CInstrumentEditorFDSEnvelope::SetupParser() const		// // //
{
	int Max, Min;
	CSeqConversionBase *pConv = nullptr;
	
	switch (m_iSelectedSetting) {
	case SEQ_VOLUME:
		Max = MAX_VOLUME; Min = 0; break;
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
	case SEQ_PITCH: // case SEQ_HIPITCH:
		Max = 126; Min = -127; break;
	}
	if (pConv == nullptr)
		pConv = new CSeqConversionDefault {Min, Max};
	m_pParser->SetSequence(m_pSequence);
	m_pParser->SetConversion(pConv);
	m_pSequenceEditor->SetConversion(pConv);		// // //
}

void CInstrumentEditorFDSEnvelope::OnCbnSelchangeType()
{
	CComboBox *pTypeBox = static_cast<CComboBox*>(GetDlgItem(IDC_TYPE));
	m_iSelectedSetting = pTypeBox->GetCurSel();
	LoadSequence();
}

void CInstrumentEditorFDSEnvelope::LoadSequence()
{
	m_pSequence = m_pInstrument->GetSequence(m_iSelectedSetting);		// // //
	m_pSequenceEditor->SelectSequence(m_pSequence, m_iSelectedSetting, INST_FDS);
	SetupParser();		// // //
}

void CInstrumentEditorFDSEnvelope::OnKeyReturn()
{
	CString string;
	GetDlgItemText(IDC_SEQUENCE_STRING, string);
	TranslateMML(string);		// // //
}
