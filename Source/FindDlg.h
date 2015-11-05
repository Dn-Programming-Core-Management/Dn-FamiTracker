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

class CharRange
{
public:
	CharRange() { Min = 0x00; Max = 0xFF; };
	CharRange(unsigned char a, unsigned char b) { Min = a; Max = b; };
	unsigned char Min;
	unsigned char Max;
};

class CharQuery
{
public:
	CharQuery() { Current = new CharRange(); Next = NULL; };
	~CharQuery() { if (Next) Next->~CharQuery(); SAFE_RELEASE(Current); };
	CharQuery(unsigned char a) { Current = new CharRange(a, a); Next = NULL; };
	CharQuery(unsigned char a, unsigned char b) { Current = new CharRange(a, b); Next = NULL; };
	CharQuery(CharRange *r) { Current = new CharRange(r->Min, r->Max); Next = NULL; };
	CharRange* Current;
	CharQuery* Next;

	bool IsMatch(unsigned char x) const;
	bool IsSingle() const;
	void Join(CharRange *Range);
	bool ParseTerm(CharQuery *target, CString &in, CString &err);
	bool ParseFull(CString &in, CString &err);

	CharQuery& operator=(unsigned char x) { Current->Min = x; Current->Max = x; SAFE_RELEASE(Next); return *this; };
	CharQuery& operator+=(unsigned char x) { Current->Min += x; Current->Max += x; return *this; };
	CharQuery& operator-=(unsigned char x) { Current->Min -= x; Current->Max -= x; return *this; };
	bool operator>(unsigned char x) { return Current->Max > x && Current->Min > x; };
	bool operator<(unsigned char x) { return Current->Max < x && Current->Min < x; };
	bool operator>=(unsigned char x) { return Current->Max >= x && Current->Min >= x; };
	bool operator<=(unsigned char x) { return Current->Max <= x && Current->Min <= x; };
	bool operator==(unsigned char x) { return Current->Max == x && Current->Min == x; };
	bool operator!=(unsigned char x) { return Current->Max != x || Current->Min != x; };
};

class searchTerm
{
public:
	searchTerm();

	void Release();

	CharQuery *Note;
	CharQuery *Oct;
	CharQuery *Inst;
	CharQuery *Vol;
	CharQuery *EffNumber[MAX_EFFECT_COLUMNS];
	CharQuery *EffParam[MAX_EFFECT_COLUMNS];
	bool Definite[6]; // 12
	unsigned char rowOffset;
	unsigned char colOffset;
	bool NoiseChan;
};

struct replaceTerm
{
	stChanNote Note;
	bool Definite[6]; // 12
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

	bool GetSimpleFindTerm();
	bool GetSimpleReplaceTerm();
	bool CompareFields(const stChanNote Target, bool Noise, int EffCount);

	replaceTerm toReplace(const searchTerm x);
	searchTerm toSearch(const replaceTerm x);
	
	CFamiTrackerDoc *m_pDocument;
	CFamiTrackerView *m_pView;

	CEdit *m_cFindNoteField, *m_cFindInstField, *m_cFindVolField, *m_cFindEffField;
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
