/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#pragma once

// CSampleEditorDlg dialog

class CSoundGen;

class CSampleEditorDlg : public CDialog
{
	DECLARE_DYNAMIC(CSampleEditorDlg)

public:
	CSampleEditorDlg(CWnd* pParent = NULL, CDSample *pSample = NULL);   // standard constructor
	virtual ~CSampleEditorDlg();

	void CopySample(CDSample *pTarget);
	void SelectionChanged();

// Dialog Data
	enum { IDD = IDD_SAMPLE_EDITOR };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void MoveControls();
	void UpdateSampleView();

	CDSample		  *m_pSample;
	CDSample		  *m_pOriginalSample;
	CSampleEditorView *m_pSampleEditorView;
	CSoundGen		  *m_pSoundGen;

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
};
