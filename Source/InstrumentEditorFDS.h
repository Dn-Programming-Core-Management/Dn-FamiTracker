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

class CWaveEditorFDS;
class CModSequenceEditor;
class CInstrumentFDS;

// CInstrumentEditorFDS dialog

class CInstrumentEditorFDS : public CInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditorFDS)

public:
	CInstrumentEditorFDS(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditorFDS();
	virtual int GetIDD() const { return IDD; };
	virtual TCHAR *GetTitle() const { return _T("Nintendo FDS"); };

	// Public
	virtual void SelectInstrument(std::shared_ptr<CInstrument> pInst);

	// Dialog Data
	enum { IDD = IDD_INSTRUMENT_FDS };

protected:
	virtual	void PreviewNote(unsigned char Key);

	void EnableModControls(bool enable);

	void ParseWaveString(LPCTSTR pString);
	void ParseTableString(LPCTSTR pString);

protected:
	std::shared_ptr<CInstrumentFDS> m_pInstrument;
	CWaveEditorFDS		*m_pWaveEditor;
	CModSequenceEditor	*m_pModSequenceEditor;

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
	afx_msg void OnModPresetFlat();
	afx_msg void OnModPresetSine();
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnModRateChange();
	afx_msg void OnModDepthChange();
	afx_msg void OnModDelayChange();
	afx_msg void OnBnClickedCopyWave();
	afx_msg void OnBnClickedPasteWave();
	afx_msg void OnBnClickedCopyTable();
	afx_msg void OnBnClickedPasteTable();
	afx_msg void OnBnClickedEnableFm();
	afx_msg LRESULT OnModChanged(WPARAM wParam, LPARAM lParam);
};
