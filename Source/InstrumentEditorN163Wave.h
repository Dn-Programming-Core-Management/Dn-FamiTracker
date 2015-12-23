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

#include "WaveEditor.h"

// CInstrumentEditorN163Wave dialog

class CInstrumentEditorN163Wave : public CInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditorN163Wave)

public:
	CInstrumentEditorN163Wave(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditorN163Wave();
	virtual int GetIDD() const { return IDD; };
	virtual TCHAR *GetTitle() const { return _T("Wave"); };

	// Public
	virtual void SelectInstrument(CInstrument *pInst);
	virtual void SelectWave(int Index);		// // //

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_N163_WAVE };

protected:
	virtual void OnKeyReturn();

	void ParseString(LPCTSTR pString);
	void FillPosBox(int size);
	void PopulateWaveBox();		// // //
	void UpdateWaveBox(int Index);		// // //
	void CreateWaveImage(char *const Pos, int Index) const;		// // //

protected:
	CInstrumentN163	*m_pInstrument;
	CWaveEditorN163	*m_pWaveEditor;
	int m_iWaveIndex;
	CImageList m_WaveImage;		// // //
	CListCtrl *m_pWaveListCtrl;		// // //

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);		// // //
	afx_msg void OnPresetSine();
	afx_msg void OnPresetTriangle();
	afx_msg void OnPresetPulse50();
	afx_msg void OnPresetPulse25();
	afx_msg void OnPresetSawtooth();
	afx_msg LRESULT OnWaveChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedCopy();
	afx_msg void OnBnClickedPaste();
	afx_msg void OnWaveSizeChange();
	afx_msg void OnWavePosChange();
	afx_msg void OnWavePosSelChange();
//	afx_msg void OnPositionClicked();
	afx_msg void OnLvnItemchangedN163Waves(NMHDR *pNMHDR, LRESULT *pResult);		// // //
	afx_msg void OnBnClickedN163Add();
	afx_msg void OnBnClickedN163Delete();
};
