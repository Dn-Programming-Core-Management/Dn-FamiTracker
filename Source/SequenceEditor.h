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

class CSequence;
class CGraphEditor;
class CSizeEditor;
class CSequenceSetting;
class CSeqConversionBase;		// // //

// Sequence editor
class CSequenceEditor : public CWnd
{
	DECLARE_DYNAMIC(CSequenceEditor)
public:
	CSequenceEditor();		// // //
	virtual ~CSequenceEditor();
	
	BOOL CreateEditor(CWnd *pParentWnd, const RECT &rect);
	void SelectSequence(CSequence *pSequence, int Type, int InstrumentType);
	void SetMaxValues(int MaxVol, int MaxDuty);
	void SetConversion(const CSeqConversionBase *pConv);		// // //

private:
	CWnd			 *m_pParent;
	CFont			 *m_pFont;
	CSizeEditor		 *m_pSizeEditor;
	CGraphEditor	 *m_pGraphEditor;
	CSequence		 *m_pSequence;
	CSequenceSetting *m_pSetting;
	const CSeqConversionBase *m_pConversion = nullptr;		// // // does not own
	
	int m_iSelectedSetting;
	int m_iInstrumentType;
	int m_iMaxVol;
	int m_iMaxDuty;
private:
	void DestroyGraphEditor();
	void SequenceChangedMessage(bool Changed);
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	//virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL DestroyWindow();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg LRESULT OnSizeChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCursorChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSequenceChanged(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSettingChanged(WPARAM wParam, LPARAM lParam);		// // //
};
