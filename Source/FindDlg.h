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
/*
struct searchTerm
{
	stChanNote Note;
	bool Definite[6];
	unsigned char rowOffset;
	unsigned char colOffset;
	bool NoiseChan;
};
*/

class IntRange
{
public:
	IntRange() { Min = 0x00; Max = 0xFF; };
	IntRange(int a, int b) { Min = a; Max = b; };
	int Min;
	int Max;
};

class IntQuery
{
public:
	IntQuery() { Current = new IntRange(); Next = NULL; };
	IntQuery(int a) { Current = new IntRange(a, a); Next = NULL; };
	IntQuery(int a, int b) { Current = new IntRange(a, b); Next = NULL; };
	IntQuery(IntRange *r) { Current = r; Next = NULL; };
	IntRange* Current;
	IntQuery* Next;

	bool IsMatch(int x);
	bool ParseTerm(IntQuery *target, CString &in, CString &err);
	bool ParseFull(CString &in, CString &err);

	IntQuery& operator=(int x) { Current->Min = x; Current->Max = x; Next = NULL; return *this; };
	IntQuery& operator+=(int x) { Current->Min += x; Current->Max += x; return *this; };
	IntQuery& operator-=(int x) { Current->Min -= x; Current->Max -= x; return *this; };
	bool operator>(int x) { return Current->Max > x && Current->Min > x; };
	bool operator<(int x) { return Current->Max < x && Current->Min < x; };
	bool operator>=(int x) { return Current->Max >= x && Current->Min >= x; };
	bool operator<=(int x) { return Current->Max <= x && Current->Min <= x; };
	bool operator==(int x) { return Current->Max == x && Current->Min == x; };
	bool operator!=(int x) { return Current->Max != x || Current->Min != x; };
};

class searchTerm
{
public:
	searchTerm();

	IntQuery *Note;
	IntQuery *Oct;
	IntQuery *Inst;
	IntQuery *Vol;
	IntQuery *EffNumber[4];
	IntQuery *EffParam[4];
	bool Definite[6];
	unsigned char rowOffset;
	unsigned char colOffset;
	bool NoiseChan;
};

struct replaceTerm
{
	stChanNote Note;
	bool Definite[6];
	unsigned char rowOffset;
	unsigned char colOffset;
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

	bool ParseNote(searchTerm &Term, CString str, bool Simple, CString &err);
	bool ParseInst(searchTerm &Term, CString str, bool Simple, CString &err);
	bool ParseVol(searchTerm &Term, CString str, bool Simple, CString &err);
	bool ParseEff(searchTerm &Term, CString str, bool Simple, CString &err);

	bool GetSimpleFindTerm(searchTerm &Term);
	bool GetSimpleReplaceTerm(searchTerm &Term);
	bool CompareFields(const searchTerm &Source, const stChanNote Target, bool Noise, int EffCount);

	replaceTerm toReplace(const searchTerm x);
	searchTerm toSearch(const replaceTerm x);
	
	CFamiTrackerDoc *m_pDocument;
	CFamiTrackerView *m_pView;

	CEdit *m_cFindNoteField, *m_cFindInstField, *m_cFindVolField, *m_cFindEffField;
	CEdit *m_cReplaceNoteField, *m_cReplaceInstField, *m_cReplaceVolField, *m_cReplaceEffField;
	CEdit *m_cFindMacroField, *m_cReplaceMacroField, *m_cFilterMacroField;
	CComboBox *m_cSearchArea, *m_cEffectColumn;

	bool m_bFindMacro, m_bReplaceMacro, m_bFilterMacro;
	bool m_bFound, m_bSkipFirst;
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
