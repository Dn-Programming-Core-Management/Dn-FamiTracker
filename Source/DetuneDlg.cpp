/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
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
** Any permitted reproduction of these routin, in whole or in part,
** must bear this legend.
*/

#include "stdafx.h"
#include <algorithm>
#include <cmath>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "DetuneDlg.h"


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
	ON_BN_CLICKED(IDC_BUTTON_TUNE, OnBnClickedButtonTune)
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
const CString CDetuneDlg::chipStr[6] = {_T("NTSC"), _T("PAL"), _T("Saw"), _T("VRC7"), _T("FDS"), _T("N163")};

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
	
	SliderOctave = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_OCTAVE);
	SliderNote = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_NOTE);
	SliderOffset = (CSliderCtrl*)GetDlgItem(IDC_SLIDER_OFFSET);

	SpinOctave = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_OCTAVE);
	SpinNote = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_NOTE);
	SpinOffset = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_OFFSET);

	EditOctave = (CEdit*)GetDlgItem(IDC_EDIT_OCTAVE);
	EditNote = (CEdit*)GetDlgItem(IDC_EDIT_NOTE);
	EditOffset = (CEdit*)GetDlgItem(IDC_EDIT_OFFSET);

	SliderOctave->SetRange(0, OCTAVE_RANGE - 1);
	SpinOctave->SetRange(0, OCTAVE_RANGE - 1);
	SliderOctave->SetPos(m_iOctave);
	SpinOctave->SetPos(m_iOctave);
	SliderOctave->SetTicFreq(1);

	SliderNote->SetRange(0, NOTE_RANGE - 1);
	SpinNote->SetRange(0, NOTE_RANGE - 1);
	SliderNote->SetPos(m_iNote % NOTE_RANGE);
	SpinNote->SetPos(m_iNote % NOTE_RANGE);
	SliderNote->SetTicFreq(1);

	SliderOffset->SetRange(-128, 128);
	SpinOffset->SetRange(-128, 128);
	SpinOffset->SetPos(m_iDetuneTable[m_iCurrentChip][m_iNote]);
	SliderOffset->SetPos(m_iDetuneTable[m_iCurrentChip][m_iNote]);
	SliderOffset->SetTicFreq(16);

	EditNote->SetWindowText(_T(m_pNote[m_iNote % NOTE_RANGE]));

	UDACCEL Acc[1];
    Acc[0].nSec = 0;
    Acc[0].nInc = 1;
	SpinNote->SetAccel(1, Acc);

	CheckRadioButton(IDC_RADIO_NTSC, IDC_RADIO_N163, IDC_RADIO_NTSC);
	CheckRadioButton(IDC_RADIO_CURRENT, IDC_RADIO_ALL, IDC_RADIO_ALL);

	CSpinButtonCtrl* SpinCent = (CSpinButtonCtrl*)GetDlgItem(IDC_SPIN_CENT);
	SpinCent->SetRange(-200, 200);

	UpdateOffset();

	return TRUE;  // return TRUE unless you set the focus to a control
}

int* CDetuneDlg::GetDetuneTable()
{
	CDialog::DoModal();
	return *m_iDetuneTable;
}

unsigned int CDetuneDlg::FreqToReg(double Freq, int Chip, int Octave)
{
	const double BASE_FREQ_NTSC = 236250000.0 / 132.0;
	const double BASE_FREQ_PAL  = 4433618.75 * 6.0 / 16.0;
	double dReg;
	switch (Chip) {
	default:
	case 0: dReg = BASE_FREQ_NTSC / Freq / 16.0 - 1.0; break;
	case 1: dReg = BASE_FREQ_PAL  / Freq / 16.0 - 1.0; break;
	case 2: dReg = BASE_FREQ_NTSC / Freq / 14.0 - 1.0; break;
	case 3: dReg = Freq / 49716.0 * (1 << (18 - Octave)); break;
	case 4: dReg = Freq / BASE_FREQ_NTSC * (1 << 20); break;
	case 5: dReg = Freq / BASE_FREQ_NTSC * 15.0 * (1 << 18) * m_pDocument->GetNamcoChannels(); break;
	}
	return (unsigned int)(dReg + .5);
}

double CDetuneDlg::RegToFreq(unsigned int Reg, int Chip, int Octave)
{
	const double BASE_FREQ_NTSC = 236250000.0 / 132.0;
	const double BASE_FREQ_PAL  = 4433618.75 * 6.0 / 16.0;
	switch (Chip) {
	default:
	case 0: return BASE_FREQ_NTSC / 16.0 / (Reg + 1.0); break;
	case 1: return BASE_FREQ_PAL  / 16.0 / (Reg + 1.0); break;
	case 2: return BASE_FREQ_NTSC / 14.0 / (Reg + 1.0); break;
	case 3: return 49716.0 * Reg / (1 << (18 - Octave)); break;
	case 4: return BASE_FREQ_NTSC * Reg / (1 << 20); break;
	case 5: return BASE_FREQ_NTSC * Reg / 15 / (1 << 18) / m_pDocument->GetNamcoChannels(); break;
	}
}

double CDetuneDlg::NoteToFreq(int Note)
{
	return 440.0 * pow(2.0, (Note - 45.0) / 12.0);
}

void CDetuneDlg::UpdateOctave()
{
	if (m_iOctave > SliderOctave->GetRangeMax()) m_iOctave = SliderOctave->GetRangeMax();
	if (m_iOctave < SliderOctave->GetRangeMin()) m_iOctave = SliderOctave->GetRangeMin();

	CString String;
	SliderOctave->SetPos(m_iOctave);
	String.Format(_T("%i"), m_iOctave);
	EditOctave->SetWindowText(String);
	m_iNote = m_iOctave * NOTE_RANGE + m_iNote % NOTE_RANGE;
	UpdateOffset();
}

void CDetuneDlg::UpdateNote()
{
	if (m_iNote > NOTE_COUNT - 1) m_iNote = NOTE_COUNT - 1;
	if (m_iNote < 0) m_iNote = 0;
	
	SliderNote->SetPos(m_iNote % NOTE_RANGE);
	EditNote->SetWindowText(_T(m_pNote[m_iNote % NOTE_RANGE]));
	m_iOctave = m_iNote / NOTE_RANGE;
	UpdateOctave();
}

void CDetuneDlg::UpdateOffset()
{
	CString String;
	SliderOffset->SetPos(m_iDetuneTable[m_iCurrentChip][m_iNote]);
	String.Format(_T("%i"), m_iDetuneTable[m_iCurrentChip][m_iNote]);
	EditOffset->SetWindowText(String);

	if (m_iCurrentChip == 3) { // VRC7
		for (int i = 0; i < OCTAVE_RANGE; i++)
			m_iDetuneTable[3][i * NOTE_RANGE + m_iNote % NOTE_RANGE] = m_iDetuneTable[3][m_iNote];
	}

	for (int i = 0; i < 6; i++) {
		CString str, fmt = _T("%s\n%X\n%X");
		int Note = m_iNote - (i == 4) * 24;
		int oldReg = FreqToReg(NoteToFreq(m_iNote), i, m_iNote / NOTE_RANGE);
		int newReg = std::max(0, (signed int)FreqToReg(NoteToFreq(m_iNote), i, m_iNote / NOTE_RANGE) + m_iDetuneTable[i][m_iNote] * (i >= 3 ? 1 : -1));
		double values[4] = {RegToFreq(FreqToReg(NoteToFreq(m_iNote), i, m_iNote / NOTE_RANGE), i, m_iNote / NOTE_RANGE) * (i == 4 ? .25 : 1),
							RegToFreq(newReg, i, m_iNote / NOTE_RANGE) * (i == 4 ? .25 : 1),
							NoteToFreq(m_iNote) * (i == 4 ? .25 : 1),
							1200.0 * log(RegToFreq(newReg, i, m_iNote / NOTE_RANGE) / NoteToFreq(m_iNote)) / log(2.0)};
		for (int j = 0; j < 4; j++) {
			if (abs(values[j]) >= 9999.5) {
				fmt += _T("\n%.0f");
			}
			else if (abs(values[j]) >= 99.995) fmt += _T("\n%.4g");
			else fmt += _T("\n%.2f");
		}

		if (i == 5 && !m_pDocument->GetNamcoChannels()) {
			str.Format(_T("%s\n-\n-\n-\n-\n-\n-"), chipStr[i]);
			SetDlgItemText(IDC_DETUNE_INFO_N163, str);
			continue;
		}
		str.Format(fmt, chipStr[i], oldReg, newReg, values[0], values[1], values[2], values[3]);
		SetDlgItemText(IDC_DETUNE_INFO_NTSC + i, str);
	}
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

	CString String;

	if (pScrollBar == (CScrollBar*)SliderOctave) {
		m_iOctave = SliderOctave->GetPos();
		UpdateOctave();
	}
	else if (pScrollBar == (CScrollBar*)SliderNote) {
		m_iNote = m_iOctave * NOTE_RANGE + SliderNote->GetPos();
		UpdateNote();
	}
	else if (pScrollBar == (CScrollBar*)SliderOffset) {
		m_iDetuneTable[m_iCurrentChip][m_iNote] = SliderOffset->GetPos();
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
	EditOctave->GetWindowText(String);
	m_iOctave = atoi(String);
	UpdateOctave();
}

void CDetuneDlg::OnEnKillfocusEditNote()
{
	CString String;
	EditNote->GetWindowText(String);
	for (int i = 0; i < NOTE_RANGE; i++)
		if (String == m_pNote[i] || String == m_pNoteFlat[i])
			m_iNote = m_iOctave * NOTE_RANGE + i;
	UpdateNote();
}

void CDetuneDlg::OnEnKillfocusEditOffset()
{
	CString String;
	EditOffset->GetWindowText(String);
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

void CDetuneDlg::OnBnClickedButtonTune()
{
	CString String;
	CEdit *EditCent = (CEdit*)GetDlgItem(IDC_EDIT_CENT);
	EditCent->GetWindowText(String);
	for (int i = 0; i < 6; i++) if (IsDlgButtonChecked(IDC_RADIO_ALL) || i == m_iCurrentChip) for (int j = 0; j < NOTE_COUNT; j++) {
		double OldFreq = 440.0 * pow(2.0, (j - 45.0) / 12.0);
		double NewFreq = 440.0 * pow(2.0, (j - 45.0) / 12.0 + atof(String) / 1200.0);
		m_iDetuneTable[i][j] = (FreqToReg(NewFreq, i, j / NOTE_RANGE) - FreqToReg(OldFreq, i, j / NOTE_RANGE)) * (i >= 3 ? 1 : -1);
	}
	UpdateOffset();
}

void CDetuneDlg::OnBnClickedButtonReset()
{
	for (int i = 0; i < 6; i++) if (IsDlgButtonChecked(IDC_RADIO_ALL) || i == m_iCurrentChip) for (int j = 0; j < NOTE_COUNT; j++)
		m_iDetuneTable[i][j] = 0;
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
			if (Count == -1) {
				if (IsDlgButtonChecked(IDC_RADIO_ALL) || Chip == m_iCurrentChip)
					m_iDetuneTable[Chip][Note] = atoi(Line);
				break;
			}
			if (IsDlgButtonChecked(IDC_RADIO_ALL) || Chip == m_iCurrentChip)
				m_iDetuneTable[Chip][Note] = atoi(Line.Left(Count));
			Note++;
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
	for (int i = 0; i < 6; i++) if (IsDlgButtonChecked(IDC_RADIO_ALL) || i == m_iCurrentChip) {
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
