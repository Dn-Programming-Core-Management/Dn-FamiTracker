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

#pragma once


// CDetuneDlg dialog

class CDetuneDlg : public CDialog
{
	DECLARE_DYNAMIC(CDetuneDlg)

public:
	CDetuneDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDetuneDlg();

	int* GetDetuneTable();

// Dialog Data
	enum { IDD = IDD_DETUNE };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	CFamiTrackerDoc* m_pDocument;

	CSliderCtrl *SliderOctave, *SliderNote, *SliderOffset;
	CSpinButtonCtrl *SpinOctave, *SpinNote, *SpinOffset;
	CEdit *EditOctave, *EditNote, *EditOffset;
	
	static const TCHAR *m_pNote[12];
	static const TCHAR *m_pNoteFlat[12];
	static const CString chipStr[6];

	int    m_iOctave;
	int    m_iNote;
	int    m_iOffset;
	int    m_iCurrentChip;
	int    m_iDetuneTable[6][96];		// NTSC PAL VRC6 VRC7 FDS N163
	double m_iCent;

	// 0CC: merge with definitions from PatternEditor.cpp
	unsigned int FreqToReg(double Freq, int Chip, int Octave);
	double       RegToFreq(unsigned int Reg, int Chip, int Octave);
	double       NoteToFreq(int Note);

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
	afx_msg void OnBnClickedButtonTune();
	afx_msg void OnBnClickedButtonReset();
	afx_msg void OnBnClickedButtonImport();
	afx_msg void OnBnClickedButtonExport();
};
