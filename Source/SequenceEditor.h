/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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
