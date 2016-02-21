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

#include <memory>
#include <string>
#include "stdafx.h"
#include "resource.h"		// // //
#include "Instrument.h"
#include "Sequence.h"
#include "InstrumentEditPanel.h"
#include "SequenceEditor.h"
#include "InstrumentEditorFDSEnvelope.h"

// CInstrumentEditorFDSEnvelope dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorFDSEnvelope, CSequenceInstrumentEditPanel)

CInstrumentEditorFDSEnvelope::CInstrumentEditorFDSEnvelope(CWnd* pParent) :
	CSequenceInstrumentEditPanel(CInstrumentEditorFDSEnvelope::IDD, pParent),
	m_iSelectedType(0)
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
	rect.DeflateRect(10, 10, 10, 45);

	m_pSequenceEditor = new CSequenceEditor();		// // //
	m_pSequenceEditor->CreateEditor(this, rect);
	m_pSequenceEditor->SetMaxValues(MAX_VOLUME, 0);

	static_cast<CComboBox*>(GetDlgItem(IDC_TYPE))->SetCurSel(0);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorFDSEnvelope::SetSequenceString(CString Sequence, bool Changed)
{
	SetDlgItemText(IDC_SEQUENCE_STRING, Sequence);
}

void CInstrumentEditorFDSEnvelope::OnCbnSelchangeType()
{
	CComboBox *pTypeBox = static_cast<CComboBox*>(GetDlgItem(IDC_TYPE));
	m_iSelectedType = pTypeBox->GetCurSel();
	LoadSequence();
}

void CInstrumentEditorFDSEnvelope::LoadSequence()
{
	switch (m_iSelectedType) {
		case SEQ_VOLUME:
			m_pSequenceEditor->SelectSequence(m_pInstrument->GetVolumeSeq(), SEQ_VOLUME, INST_FDS);	
			break;
		case SEQ_ARPEGGIO:
			m_pSequenceEditor->SelectSequence(m_pInstrument->GetArpSeq(), SEQ_ARPEGGIO, INST_FDS);
			break;
		case SEQ_PITCH:
			m_pSequenceEditor->SelectSequence(m_pInstrument->GetPitchSeq(), SEQ_PITCH, INST_FDS);
			break;
	}
}

void CInstrumentEditorFDSEnvelope::OnKeyReturn()
{
	CString string;

	GetDlgItemText(IDC_SEQUENCE_STRING, string);

	switch (m_iSelectedType) {
		case SEQ_VOLUME:
			TranslateMML(string, m_pInstrument->GetVolumeSeq(), MAX_VOLUME, 0);
			break;
		case SEQ_ARPEGGIO:
			if (m_pInstrument->GetArpSeq()->GetSetting() == SETTING_ARP_SCHEME)	// // //
				TranslateMML(string, m_pInstrument->GetArpSeq(), 36, -27);
			else
				TranslateMML(string, m_pInstrument->GetArpSeq(), 96, m_pInstrument->GetArpSeq()->GetSetting()== SETTING_ARP_FIXED ? 0 : -96);
			break;
		case SEQ_PITCH:
			TranslateMML(string, m_pInstrument->GetPitchSeq(), 126, -127);
			break;
	}
}
