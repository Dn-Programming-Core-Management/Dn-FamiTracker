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

// CSampleEditorDlg dialog

class CSoundGen;

class CSampleEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CSampleEditorDlg)

public:
	CSampleEditorDlg(CWnd* pParent = NULL, CDSample *pSample = NULL);   // standard constructor
	virtual ~CSampleEditorDlg();
	CDSample *GetDSample() const;

	void SelectionChanged();
	void UpdateStatus(int Index, LPCTSTR Text);		// // //

// Dialog Data
	enum { IDD = IDD_SAMPLE_EDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void UpdateSampleView();

	CDSample		  *m_pSample;
	CSampleEditorView *m_pSampleEditorView;
	CSoundGen		  *m_pSoundGen;

	CStatusBar		  m_wndInfoStatusBar;		// // // 050B

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg void OnMove(int x, int y);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnBnClickedPlay();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedDelete();
	afx_msg void OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedTest();
	afx_msg void OnBnClickedDeltastart();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnBnClickedTilt();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	afx_msg void OnBnClickedBitReverse();
};
