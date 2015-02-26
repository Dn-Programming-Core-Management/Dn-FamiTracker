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

enum {
	WM_SIZE_CHANGE = WM_USER, 
	WM_CURSOR_CHANGE, 
	WM_SEQUENCE_CHANGED
};

class CSequence;
class CGraphEditor;
class CSizeEditor;
class CSequenceSetting;

// Sequence editor
class CSequenceEditor : public CWnd
{
	DECLARE_DYNAMIC(CSequenceEditor)
public:
	CSequenceEditor(CFamiTrackerDoc *pDoc);
	virtual ~CSequenceEditor();
	
	BOOL CreateEditor(CWnd *pParentWnd, const RECT &rect);
	void SelectSequence(CSequence *pSequence, int Type, int InstrumentType);
	void SetMaxValues(int MaxVol, int MaxDuty);
	void ChangedSetting();

public:
	static const int SEQUENCE_EDIT_WIDTH = 540;
	static const int SEQUENCE_EDIT_HEIGHT = 237;
private:
	CFamiTrackerDoc  *m_pDocument;
	CWnd			 *m_pParent;
	CFont			 *m_pFont;
	CSizeEditor		 *m_pSizeEditor;
	CGraphEditor	 *m_pGraphEditor;
	CSequence		 *m_pSequence;
	CSequenceSetting *m_pSetting;
	
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
};
