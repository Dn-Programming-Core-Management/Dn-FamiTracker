/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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
#include <algorithm>
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "DetuneDlg.h"
#include "DetuneTable.h"
#include "SoundGen.h"


// CDetuneDlg dialog

IMPLEMENT_DYNAMIC(CDetuneDlg, CDialog)

CDetuneDlg::CDetuneDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDetuneDlg::IDD, pParent)
{
}

CDetuneDlg::~CDetuneDlg()
{
}

void CDetuneDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDetuneDlg, CDialog)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_OCTAVE, OnDeltaposSpinOctave)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_NOTE, OnDeltaposSpinNote)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_OFFSET, OnDeltaposSpinOffset)
	ON_EN_KILLFOCUS(IDC_EDIT_OCTAVE, OnEnKillfocusEditOctave)
	ON_EN_KILLFOCUS(IDC_EDIT_NOTE, OnEnKillfocusEditNote)
	ON_EN_KILLFOCUS(IDC_EDIT_OFFSET, OnEnKillfocusEditOffset)
	ON_BN_CLICKED(IDC_RADIO_NTSC, OnBnClickedRadioNTSC)
	ON_BN_CLICKED(IDC_RADIO_PAL, OnBnClickedRadioPAL)
	ON_BN_CLICKED(IDC_RADIO_VRC6, OnBnClickedRadioVRC6)
	ON_BN_CLICKED(IDC_RADIO_VRC7, OnBnClickedRadioVRC7)
	ON_BN_CLICKED(IDC_RADIO_FDS, OnBnClickedRadioFDS)
	ON_BN_CLICKED(IDC_RADIO_N163, OnBnClickedRadioN163)
	ON_BN_CLICKED(IDC_BUTTON_RESET, OnBnClickedButtonReset)
	ON_BN_CLICKED(IDC_BUTTON_IMPORT, OnBnClickedButtonImport)
	ON_BN_CLICKED(IDC_BUTTON_EXPORT, OnBnClickedButtonExport)
END_MESSAGE_MAP()


// CDetuneDlg message handlers

const TCHAR *CDetuneDlg::m_pNote[12]	 = {_T("C") , _T("C#"), _T("D") , _T("D#")
										  , _T("E") , _T("F") , _T("F#"), _T("G")
										  , _T("G#"), _T("A") , _T("A#"), _T("B")};
const TCHAR *CDetuneDlg::m_pNoteFlat[12] = {_T("C") , _T("D-"), _T("D") , _T("E-")
										  , _T("E") , _T("F") , _T("G-"), _T("G")
										  , _T("A-"), _T("A") , _T("B-"), _T("B")};
const CString CDetuneDlg::CHIP_STR[6] = {_T("NTSC"), _T("PAL"), _T("Saw"), _T("VRC7"), _T("FDS"), _T("N163")};

BOOL CDetuneDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CFrameWnd *pFrameWnd = static_cast<CFrameWnd*>(GetParent());
	m_pDocument = static_cast<CFamiTrackerDoc*>(pFrameWnd->GetActiveDocument());
	m_iOctave = 3;
	m_iNote = 36;
	m_iCurrentChip = 0;
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		m_iDetuneTable[i][j] = m_pDocument->GetDetuneOffset(i, j);
	m_iGlobalSemitone = m_pDocument->GetTuningSemitone();		// // // 050B
	m_iGlobalCent = m_pDocument->GetTuningCent();

	m_pDetuneNTSC = std::make_unique<CDetuneNTSC>(45.0);
	m_pDetunePAL = std::make_unique<CDetunePAL>(45.0);
	m_pDetuneSaw = std::make_unique<CDetuneSaw>(45.0);
	m_pDetuneVRC7 = std::make_unique<CDetuneVRC7>(45.0);
	m_pDetuneFDS = std::make_unique<CDetuneFDS>(45.0);
	m_pDetuneN163 = std::make_unique<CDetuneN163>(45.0);
	
	m_cSliderOctave = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_OCTAVE);
	m_cSliderNote = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_NOTE);
	m_cSliderOffset = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_OFFSET);

	m_cEditOctave = (CEdit*)GetDlgItem(IDC_EDIT_OCTAVE);
	m_cEditNote = (CEdit*)GetDlgItem(IDC_EDIT_NOTE);
	m_cEditOffset = (CEdit*)GetDlgItem(IDC_EDIT_OFFSET);
	
	auto SpinOctave = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_OCTAVE);
	m_cSliderOctave->SetRange(0, OCTAVE_RANGE - 1);
	SpinOctave->SetRange(0, OCTAVE_RANGE - 1);
	m_cSliderOctave->SetPos(m_iOctave);
	SpinOctave->SetPos(m_iOctave);
	m_cSliderOctave->SetTicFreq(1);
	
	auto SpinNote = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_NOTE);
	m_cSliderNote->SetRange(0, NOTE_RANGE - 1);
	SpinNote->SetRange(0, NOTE_RANGE - 1);
	m_cSliderNote->SetPos(m_iNote % NOTE_RANGE);
	SpinNote->SetPos(m_iNote % NOTE_RANGE);
	m_cSliderNote->SetTicFreq(1);
	
	auto SpinOffset = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_OFFSET);
	m_cSliderOffset->SetRange(-16, 16);
	SpinOffset->SetRange(-16, 16);
	m_cSliderOffset->SetPos(m_iDetuneTable[m_iCurrentChip][m_iNote]);
	SpinOffset->SetPos(m_iDetuneTable[m_iCurrentChip][m_iNote]);
	m_cSliderOffset->SetTicFreq(4);

	m_cEditNote->SetWindowText(_T(m_pNote[m_iNote % NOTE_RANGE]));

	UDACCEL Acc[1];
	Acc[0].nSec = 0;
	Acc[0].nInc = 1;
	SpinNote->SetAccel(1, Acc);

	CheckRadioButton(IDC_RADIO_NTSC, IDC_RADIO_N163, IDC_RADIO_NTSC);

	auto pSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_DETUNE_SEMITONE));
	pSlider->SetRange(-NOTE_RANGE, NOTE_RANGE);
	pSlider->SetTicFreq(1);
	pSlider->SetPos(m_iGlobalSemitone);
	pSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_DETUNE_CENT));
	pSlider->SetRange(-100, 100);
	pSlider->SetTicFreq(5);
	pSlider->SetPos(m_iGlobalCent);

	UpdateOffset();

	return TRUE;  // return TRUE unless you set the focus to a control
}

const int *CDetuneDlg::GetDetuneTable() const
{
	return *m_iDetuneTable;
}

int CDetuneDlg::GetDetuneSemitone() const
{
	return m_iGlobalSemitone;
}

int CDetuneDlg::GetDetuneCent() const
{
	return m_iGlobalCent;
}

unsigned int CDetuneDlg::FreqToPeriod(double Freq, int Chip, int Octave)
{
	switch (Chip) {
	default:
	case 0: return m_pDetuneNTSC->FrequencyToPeriod(Freq, Octave, 0); break;
	case 1: return m_pDetunePAL->FrequencyToPeriod(Freq, Octave, 0); break;
	case 2: return m_pDetuneSaw->FrequencyToPeriod(Freq, Octave, 0); break;
	case 3: return m_pDetuneVRC7->FrequencyToPeriod(Freq, Octave, 0); break;
	case 4: return m_pDetuneFDS->FrequencyToPeriod(Freq, Octave, 0); break;
	case 5: return m_pDetuneN163->FrequencyToPeriod(Freq, Octave, m_pDocument->GetNamcoChannels()); break;
	}
}

double CDetuneDlg::PeriodToFreq(unsigned int Period, int Chip, int Octave)
{
	switch (Chip) {
	default:
	case 0: return m_pDetuneNTSC->PeriodToFrequency(Period, Octave, 0); break;
	case 1: return m_pDetunePAL->PeriodToFrequency(Period, Octave, 0); break;
	case 2: return m_pDetuneSaw->PeriodToFrequency(Period, Octave, 0); break;
	case 3: return m_pDetuneVRC7->PeriodToFrequency(Period, Octave, 0); break;
	case 4: return m_pDetuneFDS->PeriodToFrequency(Period, Octave, 0); break;
	case 5: return m_pDetuneN163->PeriodToFrequency(Period, Octave, m_pDocument->GetNamcoChannels()); break;
	}
}

double CDetuneDlg::NoteToFreq(double Note, int Chip)
{
	// I could instead create a pointer directly to CDetuneTable,
	// but these pointers to its children are already available.
	switch (Chip) {
	default:
	case 0: return m_pDetuneNTSC->NoteToFreq(Note); break;
	case 1: return m_pDetunePAL->NoteToFreq(Note); break;
	case 2: return m_pDetuneSaw->NoteToFreq(Note); break;
	case 3: return m_pDetuneVRC7->NoteToFreq(Note); break;
	case 4: return m_pDetuneFDS->NoteToFreq(Note); break;
	case 5: return m_pDetuneN163->NoteToFreq(Note); break;
	}
}

void CDetuneDlg::UpdateOctave()
{
	CString String;
	m_iOctave = std::max(std::min(m_iOctave, OCTAVE_RANGE - 1), 0);
	m_cSliderOctave->SetPos(m_iOctave);
	String.Format(_T("%i"), m_iOctave);
	m_cEditOctave->SetWindowText(String);
	m_iNote = m_iOctave * NOTE_RANGE + m_iNote % NOTE_RANGE;
	UpdateOffset();
}

void CDetuneDlg::UpdateNote()
{
	m_iNote = std::max(std::min(m_iNote, NOTE_COUNT - 1), 0);
	m_cSliderNote->SetPos(m_iNote % NOTE_RANGE);
	m_cEditNote->SetWindowText(_T(m_pNote[m_iNote % NOTE_RANGE]));
	m_iOctave = m_iNote / NOTE_RANGE;
	UpdateOctave();
}

void CDetuneDlg::UpdateOffset()
{
	CString String;
	m_cSliderOffset->SetPos(m_iDetuneTable[m_iCurrentChip][m_iNote]);
	String.Format(_T("%i"), m_iDetuneTable[m_iCurrentChip][m_iNote]);
	m_cEditOffset->SetWindowText(String);

	if (m_iCurrentChip == 3) // VRC7
		for (int i = 0; i < OCTAVE_RANGE; i++)
			m_iDetuneTable[3][i * NOTE_RANGE + m_iNote % NOTE_RANGE] = m_iDetuneTable[3][m_iNote];

	const auto DoubleFunc = [] (double x) {
		if (std::abs(x) >= 9999.5)
			return _T("\n%.0f");
		if (std::abs(x) >= 99.995)
			return _T("\n%.4g");
		return _T("\n%.2f");
	};

	for (int i = 0; i < 6; i++) {
		CString str, fmt = _T("%s\n%X\n%X");
		if (i == 5 && !m_pDocument->GetNamcoChannels()) {
			str.Format(_T("%s\n-\n-\n-\n-\n-\n-"), CHIP_STR[i]);
			SetDlgItemText(IDC_DETUNE_INFO_N163, str);
			continue;
		}
		double Note = m_iGlobalSemitone + .01 * m_iGlobalCent + m_iNote;
		int oldReg = FreqToPeriod(NoteToFreq(Note, i), i, m_iNote / NOTE_RANGE);
		int newReg = std::max(0, (int)oldReg + m_iDetuneTable[i][m_iNote] * (i >= 3 ? 1 : -1));
		double newFreq = PeriodToFreq(newReg, i, m_iNote / NOTE_RANGE);
		double values[4] = {PeriodToFreq(oldReg, i, m_iNote / NOTE_RANGE) * (i == 4 ? .25 : 1),
							newFreq * (i == 4 ? .25 : 1),
							NoteToFreq(Note, i) * (i == 4 ? .25 : 1),
							1200.0 * log(newFreq / NoteToFreq(Note, i)) / log(2.0)};
		for (const auto x : values)
			fmt += DoubleFunc(x);

		str.Format(fmt, CHIP_STR[i], oldReg, newReg, values[0], values[1], values[2], values[3]);
		SetDlgItemText(IDC_DETUNE_INFO_NTSC + i, str);
	}

	String.Format(_T("Semitone: %+d"), m_iGlobalSemitone);
	SetDlgItemText(IDC_STATIC_DETUNE_SEMITONE, String);
	String.Format(_T("Cent: %+d"), m_iGlobalCent);
	SetDlgItemText(IDC_STATIC_DETUNE_CENT, String);
}

void CDetuneDlg::OnBnClickedOk()
{
	m_pDocument->SetModifiedFlag();
	m_pDocument->SetExceededFlag();
	CDialog::OnOK();
}

void CDetuneDlg::OnBnClickedCancel()
{
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		m_iDetuneTable[i][j] = m_pDocument->GetDetuneOffset(i, j);
	CDialog::OnCancel();
}

void CDetuneDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

	if (pScrollBar == (CScrollBar*)m_cSliderOctave) {
		m_iOctave = m_cSliderOctave->GetPos();
		UpdateOctave();
	}
	else if (pScrollBar == (CScrollBar*)m_cSliderNote) {
		m_iNote = m_iOctave * NOTE_RANGE + m_cSliderNote->GetPos();
		UpdateNote();
	}
	else if (pScrollBar == (CScrollBar*)m_cSliderOffset) {
		m_iDetuneTable[m_iCurrentChip][m_iNote] = m_cSliderOffset->GetPos();
		UpdateOffset();
	}
	else if (pScrollBar == GetDlgItem(IDC_SLIDER_DETUNE_SEMITONE)) {
		m_iGlobalSemitone = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_DETUNE_SEMITONE))->GetPos();
		UpdateOffset();
	}
	else if (pScrollBar == GetDlgItem(IDC_SLIDER_DETUNE_CENT)) {
		m_iGlobalCent = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER_DETUNE_CENT))->GetPos();
		UpdateOffset();
	}
}

void CDetuneDlg::OnDeltaposSpinOctave(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	
	m_iOctave += pNMUpDown->iDelta;
	UpdateOctave();

	*pResult = 0;
}

void CDetuneDlg::OnDeltaposSpinNote(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	m_iNote += pNMUpDown->iDelta;
	UpdateNote();

	*pResult = 0;
}

void CDetuneDlg::OnDeltaposSpinOffset(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	
	m_iDetuneTable[m_iCurrentChip][m_iNote] += pNMUpDown->iDelta;
	UpdateOffset();

	*pResult = 0;
}

void CDetuneDlg::OnEnKillfocusEditOctave()
{
	CString String;
	m_cEditOctave->GetWindowText(String);
	m_iOctave = atoi(String);
	UpdateOctave();
}

void CDetuneDlg::OnEnKillfocusEditNote()
{
	CString String;
	m_cEditNote->GetWindowText(String);
	for (int i = 0; i < NOTE_RANGE; i++)
		if (String == m_pNote[i] || String == m_pNoteFlat[i])
			m_iNote = m_iOctave * NOTE_RANGE + i;
	UpdateNote();
}

void CDetuneDlg::OnEnKillfocusEditOffset()
{
	CString String;
	m_cEditOffset->GetWindowText(String);
	m_iDetuneTable[m_iCurrentChip][m_iNote] = atoi(String);
	UpdateOctave();
}

void CDetuneDlg::OnBnClickedRadioNTSC()
{
	m_iCurrentChip = 0;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioPAL()
{
	m_iCurrentChip = 1;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioVRC6()
{
	m_iCurrentChip = 2;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioVRC7()
{
	m_iCurrentChip = 3;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioFDS()
{
	m_iCurrentChip = 4;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedRadioN163()
{
	m_iCurrentChip = 5;
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedButtonReset()
{
	for (auto &x : m_iDetuneTable[m_iCurrentChip])
		x = 0;
	UpdateOffset();
}


void CDetuneDlg::OnBnClickedButtonImport()
{
	CString    Path;
	CStdioFile csv;
	CFrameWnd *pFrameWnd = static_cast<CFrameWnd*>(GetParent());

	CFileDialog FileDialog(TRUE, _T("csv"), 0,
		OFN_HIDEREADONLY, _T("Comma-separated values (*.csv)|*.csv|All files|*.*||"));

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	Path = FileDialog.GetPathName();

	if (!csv.Open(Path, CFile::modeRead)) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR);
		return;
	}

	CString Line;
	int Count, Chip = 0, Note = 0;
	while (csv.ReadString(Line)) {
		Count = Line.Find(_T(','), 0);
		Chip = atoi(Line.Left(Count));
		Note = 0;
		while (Line.Delete(0, Count)) {
			if (!Line.Delete(0, 1)) break;
			Count = Line.Find(_T(','), 0);
			if (Count == -1)
				m_iDetuneTable[Chip][Note] = atoi(Line);
			else {
				m_iDetuneTable[Chip][Note] = atoi(Line.Left(Count));
				++Note;
			}
		}
	}

	csv.Close();
	UpdateOffset();
}


void CDetuneDlg::OnBnClickedButtonExport()
{
	CString    Path;
	CStdioFile csv;

	CFileDialog SaveFileDialog(FALSE, _T("csv"), (LPCSTR)m_pDocument->GetFileTitle(),
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Comma-separated values (*.csv)|*.csv|All files|*.*||"));

	if (SaveFileDialog.DoModal() == IDCANCEL)
		return;

	Path = SaveFileDialog.GetPathName();

	if (!csv.Open(Path, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR);
		return;
	}

	CString Line, Unit;
	for (int i = 0; i < 6; i++) {
		Line.Format(_T("%i"), i);
		for (int j = 0; j < NOTE_COUNT; j++) {
			Unit.Format(_T(",%i"), m_iDetuneTable[i][j]);
			Line += Unit;
		}
		Line += _T("\n");
		csv.WriteString(Line);
	}

	csv.Close();
	UpdateOffset();
}
