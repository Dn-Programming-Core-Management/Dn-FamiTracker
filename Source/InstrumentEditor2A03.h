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

class CSequenceEditor;

class CInstrumentEditor2A03 : public CSequenceInstrumentEditPanel
{
	DECLARE_DYNAMIC(CInstrumentEditor2A03)

public:
	CInstrumentEditor2A03(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditor2A03();
	virtual int GetIDD() const { return IDD; };
	virtual TCHAR *GetTitle() const { return _T("2A03 settings"); };

	// Public
	virtual void SelectInstrument(int Instrument);
	virtual void SetSequenceString(CString Sequence, bool Changed);

// Dialog Data
	enum { IDD = IDD_INSTRUMENT_INTERNAL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnKeyReturn();
	
	void TranslateMML(CString String, int Max, int Min);
	void SelectSequence(int Sequence, int Type);

protected:
	static LPCTSTR INST_SETTINGS_2A03[];

protected:
	CInstrument2A03	*m_pInstrument;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnLvnItemchangedInstsettings(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeSeqIndex();
	afx_msg void OnBnClickedFreeSeq();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	virtual BOOL DestroyWindow();
	afx_msg void OnCloneSequence();
};
