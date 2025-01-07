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

#pragma once


// CDetuneDlg dialog

class CDetuneNTSC;
class CDetunePAL;
class CDetuneSaw;
class CDetuneVRC7;
class CDetuneFDS;
class CDetuneN163;


class CDetuneDlg : public CDialog
{
	DECLARE_DYNAMIC(CDetuneDlg)

public:
	CDetuneDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDetuneDlg();

	const int *GetDetuneTable() const;
	int GetDetuneSemitone() const;
	int GetDetuneCent() const;
	static const CString CHIP_STR[6];

// Dialog Data
	enum { IDD = IDD_DETUNE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CFamiTrackerDoc* m_pDocument;

	CSliderCtrl *m_cSliderOctave, *m_cSliderNote, *m_cSliderOffset;
	CEdit *m_cEditOctave, *m_cEditNote, *m_cEditOffset;


	std::unique_ptr<CDetuneNTSC> m_pDetuneNTSC;
	std::unique_ptr<CDetunePAL> m_pDetunePAL;
	std::unique_ptr<CDetuneSaw> m_pDetuneSaw;
	std::unique_ptr<CDetuneVRC7> m_pDetuneVRC7;
	std::unique_ptr<CDetuneFDS> m_pDetuneFDS;
	std::unique_ptr<CDetuneN163> m_pDetuneN163;
	
	static const TCHAR *m_pNote[12];
	static const TCHAR *m_pNoteFlat[12];

	int    m_iOctave;
	int    m_iNote;
	int    m_iOffset;
	int    m_iCurrentChip;
	int    m_iDetuneTable[6][96];		// NTSC PAL VRC6 VRC7 FDS N163
	int    m_iGlobalSemitone, m_iGlobalCent;

	unsigned int FreqToPeriod(double Freq, int Chip, int Octave);
	double       PeriodToFreq(unsigned int Period, int Chip, int Octave);
	double       NoteToFreq(double Note, int Chip);

	void UpdateOctave();
	void UpdateNote();
	void UpdateOffset();

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDeltaposSpinOctave(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpinNote(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposSpinOffset(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnKillfocusEditOctave();
	afx_msg void OnEnKillfocusEditNote();
	afx_msg void OnEnKillfocusEditOffset();
	afx_msg void OnBnClickedRadioNTSC();
	afx_msg void OnBnClickedRadioPAL();
	afx_msg void OnBnClickedRadioVRC6();
	afx_msg void OnBnClickedRadioVRC7();
	afx_msg void OnBnClickedRadioFDS();
	afx_msg void OnBnClickedRadioN163();
	afx_msg void OnBnClickedButtonReset();
	afx_msg void OnBnClickedButtonImport();
	afx_msg void OnBnClickedButtonExport();
};
