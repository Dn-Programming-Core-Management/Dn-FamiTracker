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
#include "res/resource.h"
#include "FamiTrackerDoc.h"
#include "Instrument.h"
#include "TransposeDlg.h"
#include "DPI.h"

// CTransposeDlg dialog

bool CTransposeDlg::s_bDisableInst[MAX_INSTRUMENTS] = {false};
const UINT CTransposeDlg::BUTTON_ID = 0xDD00; // large enough

IMPLEMENT_DYNAMIC(CTransposeDlg, CDialog)

CTransposeDlg::CTransposeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_TRANSPOSE, pParent), m_iTrack(0)
{

}

CTransposeDlg::~CTransposeDlg()
{
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		SAFE_RELEASE(m_cInstButton[i]);
	SAFE_RELEASE(m_cInstButton);
	SAFE_RELEASE(m_pFont);
}

void CTransposeDlg::SetTrack(unsigned int Track)
{
	m_iTrack = Track;
}

void CTransposeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void CTransposeDlg::Transpose(int Trsp, unsigned int Track)
{
	if (!Trsp) return;
	stChanNote Note;
	for (int c = m_pDocument->GetChannelCount() - 1; c >= 0; --c) {
		int Type = m_pDocument->GetChannelType(c);
		if (Type == CHANID_NOISE || Type == CHANID_DPCM) continue;
		for (int p = 0; p < MAX_PATTERN; ++p) for (int r = 0; r < MAX_PATTERN_LENGTH; ++r) {
			m_pDocument->GetDataAtPattern(Track, p, c, r, &Note);
			if (Note.Note >= NOTE_C && Note.Note <= NOTE_B && !s_bDisableInst[Note.Instrument]) {
				int MIDI = MIDI_NOTE(Note.Octave, Note.Note) + Trsp;
				if (MIDI < 0) MIDI = 0;
				else if (MIDI >= NOTE_COUNT) MIDI = NOTE_COUNT - 1;
				Note.Octave = GET_OCTAVE(MIDI);
				Note.Note = GET_NOTE(MIDI);
				m_pDocument->SetDataAtPattern(Track, p, c, r, &Note);
			}
		}
	}
}

BEGIN_MESSAGE_MAP(CTransposeDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CTransposeDlg::OnBnClickedOk)
	ON_CONTROL_RANGE(BN_CLICKED, BUTTON_ID, BUTTON_ID + MAX_INSTRUMENTS - 1, OnBnClickedInst)
	ON_BN_CLICKED(IDC_BUTTON_TRSP_REVERSE, &CTransposeDlg::OnBnClickedButtonTrspReverse)
	ON_BN_CLICKED(IDC_BUTTON_TRSP_CLEAR, &CTransposeDlg::OnBnClickedButtonTrspClear)
END_MESSAGE_MAP()


// CTransposeDlg message handlers

BOOL CTransposeDlg::OnInitDialog()
{
	LOGFONT LogFont;
	const TCHAR *SMALL_FONT_FACE = _T("Verdana");
	m_pFont = new CFont();

	memset(&LogFont, 0, sizeof LOGFONT);
	_tcscpy_s(LogFont.lfFaceName, 32, SMALL_FONT_FACE);
	LogFont.lfHeight = -DPI::SY(10);
	LogFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
	m_pFont->CreateFontIndirect(&LogFont);

	m_pDocument = CFamiTrackerDoc::GetDoc();
	CRect r;
	GetClientRect(&r);

	m_cInstButton = new CButton*[MAX_INSTRUMENTS];
	CString Name;
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		Name.Format(_T("%02X"), i);
		m_cInstButton[i] = new CButton();
		int x = DPI::SX(20) + i % 8 * ((r.Width() - DPI::SX(30)) / 8);
		int y = DPI::SY(104) + i / 8 * DPI::SY(20);
		m_cInstButton[i]->Create(Name, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
								 CRect(x, y, x + DPI::SX(30), y + DPI::SY(18)), this, BUTTON_ID + i);
		m_cInstButton[i]->SetCheck(s_bDisableInst[i] ? BST_CHECKED : BST_UNCHECKED);
		m_cInstButton[i]->SetFont(m_pFont);
		m_cInstButton[i]->EnableWindow(m_pDocument->IsInstrumentUsed(i));
	}

	CSpinButtonCtrl *pSpin = static_cast<CSpinButtonCtrl*>(GetDlgItem(IDC_SPIN_TRSP_SEMITONE));
	pSpin->SetRange(0, 96);
	pSpin->SetPos(0);

	CheckRadioButton(IDC_RADIO_SEMITONE_INC, IDC_RADIO_SEMITONE_DEC, IDC_RADIO_SEMITONE_INC);

	return CDialog::OnInitDialog();
}

void CTransposeDlg::OnBnClickedInst(UINT nID)
{
	s_bDisableInst[nID - BUTTON_ID] = !s_bDisableInst[nID - BUTTON_ID];
}

void CTransposeDlg::OnBnClickedOk()
{
	int Trsp = GetDlgItemInt(IDC_EDIT_TRSP_SEMITONE);
	bool All = IsDlgButtonChecked(IDC_CHECK_TRSP_ALL) != 0;
	if (GetCheckedRadioButton(IDC_RADIO_SEMITONE_INC, IDC_RADIO_SEMITONE_DEC) == IDC_RADIO_SEMITONE_DEC)
		Trsp = -Trsp;

	if (All) {
		const unsigned int Tracks = m_pDocument->GetTrackCount();
		for (unsigned int t = 0; t < Tracks; ++t)
			Transpose(Trsp, t);
	}
	else
		Transpose(Trsp, m_iTrack);

	m_pDocument->UpdateAllViews(NULL, UPDATE_PATTERN);
	CDialog::OnOK();
}

void CTransposeDlg::OnBnClickedButtonTrspReverse()
{
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		CheckDlgButton(BUTTON_ID + i, IsDlgButtonChecked(BUTTON_ID + i) == BST_UNCHECKED);
}

void CTransposeDlg::OnBnClickedButtonTrspClear()
{
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		CheckDlgButton(BUTTON_ID + i, BST_UNCHECKED);
}
