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

#include "FamiTrackerView.h"

class CharRange
{
public:
	CharRange() { Min = '\x00'; Max = '\xFF'; };
	CharRange(unsigned char a, unsigned char b) { Min = a; Max = b; };
	void Set(unsigned char x, bool Half = false) { if (!Half) Min = x; Max = x; };
	bool IsMatch(unsigned char x) const { return (x >= Min && x <= Max) || (x >= Max && x <= Min); };
	bool IsSingle() const { return Min == Max; };
	unsigned char Min;
	unsigned char Max;
};

class searchTerm
{
public:
	searchTerm();

	void Release();
	void Reset();

	CharRange *Note;
	CharRange *Oct;
	CharRange *Inst;
	CharRange *Vol;
	bool EffNumber[EF_COUNT];
	CharRange *EffParam;
	bool Definite[6];
	bool NoiseChan;
};

struct replaceTerm
{
	stChanNote Note;
	bool Definite[6];
	bool NoiseChan;
};

// CFindDlg dialog

class CFindDlg : public CDialog
{
	DECLARE_DYNAMIC(CFindDlg)

public:
	CFindDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFindDlg();

	CString ConvertFindSimple(); // 0CC: unimplemented
	bool Find(bool ShowEnd = false);
	bool Replace(bool CanUndo = false);
	void Reset();

// Dialog Data
	enum { IDD = IDD_FIND };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	bool ParseNote(searchTerm &Term, CString str, bool Half, CString &err);
	bool ParseInst(searchTerm &Term, CString str, bool Half, CString &err);
	bool ParseVol(searchTerm &Term, CString str, bool Half, CString &err);
	bool ParseEff(searchTerm &Term, CString str, bool Half, CString &err);

	bool GetSimpleFindTerm();
	bool GetSimpleReplaceTerm();
	bool CompareFields(const stChanNote Target, bool Noise, int EffCount);

	replaceTerm toReplace(const searchTerm x);
	searchTerm toSearch(const replaceTerm x);
	
	CFamiTrackerDoc *m_pDocument;
	CFamiTrackerView *m_pView;

	CEdit *m_cFindNoteField, *m_cFindInstField, *m_cFindVolField, *m_cFindEffField;
	CEdit *m_cFindNoteField2, *m_cFindInstField2, *m_cFindVolField2;
	CEdit *m_cReplaceNoteField, *m_cReplaceInstField, *m_cReplaceVolField, *m_cReplaceEffField;
	CComboBox *m_cSearchArea, *m_cEffectColumn;

	searchTerm m_searchTerm, m_replaceTerm;
	bool m_bFound, m_bSkipFirst, m_bVisible;
	int m_iFrame, m_iRow, m_iChannel;
	static const CString m_pNoteName[7];
	static const CString m_pNoteSign[3];
	static const int m_iNoteOffset[7];

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void UpdateFields();
	afx_msg void OnUpdateFields(UINT nID);
	afx_msg void OnBnClickedButtonFindNext();
	afx_msg void OnBnClickedButtonReplace();
};
