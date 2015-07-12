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

#include <algorithm>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "PatternEditor.h"
#include "FindDlg.h"

#define FIND_RAISE_ERROR(cond,...) {if (cond) { err.Format(__VA_ARGS__); return false; }}
#define FIND_SINGLE_CHANNEL(x) ( ((x) & 0x01) == 0x01 )
#define FIND_SINGLE_FRAME(x) ( ((x) & 0x02) == 0x02 )
#define FIND_WILD _T("*")
#define FIND_BLANK _T("!")

// CFindDlg dialog

IMPLEMENT_DYNAMIC(CFindDlg, CDialog)

CFindDlg::CFindDlg(CWnd* pParent /*=NULL*/) : CDialog(CFindDlg::IDD, pParent),
	m_bFindMacro(false),
	m_bReplaceMacro(false),
	m_bFilterMacro(false),
	m_bFound(false),
	m_bSkipFirst(true),
	m_bVisible(true),
	m_iFrame(0),
	m_iRow(0),
	m_iChannel(0)
{
	//memset(&m_searchTerm, 0, sizeof(searchTerm));
	//memset(&m_replaceTerm, 0, sizeof(replaceTerm));
}

CFindDlg::~CFindDlg()
{
	SAFE_RELEASE(m_cFindNoteField);
	SAFE_RELEASE(m_cFindInstField);
	SAFE_RELEASE(m_cFindVolField);
	SAFE_RELEASE(m_cFindEffField);
	SAFE_RELEASE(m_cReplaceNoteField);
	SAFE_RELEASE(m_cReplaceInstField);
	SAFE_RELEASE(m_cReplaceVolField);
	SAFE_RELEASE(m_cReplaceEffField);
	SAFE_RELEASE(m_cFindMacroField);
	SAFE_RELEASE(m_cReplaceMacroField);
	SAFE_RELEASE(m_cFilterMacroField);
	SAFE_RELEASE(m_cSearchArea);
	SAFE_RELEASE(m_cEffectColumn);
	m_searchTerm.Release();
	m_replaceTerm.Release();
}

void CFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFindDlg, CDialog)
	ON_BN_CLICKED(IDC_RADIO_FIND_SIMPLE, UpdateFields)
	ON_BN_CLICKED(IDC_RADIO_FIND_MACRO, UpdateFields)
	ON_BN_CLICKED(IDC_RADIO_REPLACE_SIMPLE, UpdateFields)
	ON_BN_CLICKED(IDC_RADIO_REPLACE_MACRO, UpdateFields)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECK_FIND_NOTE, IDC_CHECK_FIND_EFF, OnUpdateFields)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_CHECK_REPLACE_NOTE, IDC_CHECK_REPLACE_EFF, OnUpdateFields)
	ON_BN_CLICKED(IDC_BUTTON_FIND_NEXT, OnBnClickedButtonFindNext)
	ON_BN_CLICKED(IDC_BUTTON_REPLACE, OnBnClickedButtonReplace)
END_MESSAGE_MAP()


// CFindDlg message handlers

const CString CFindDlg::m_pNoteName[7] = {_T("C"), _T("D"), _T("E"), _T("F"), _T("G"), _T("A"), _T("B")};
const CString CFindDlg::m_pNoteSign[3] = {_T("b"), _T("-"), _T("#")};
const int CFindDlg::m_iNoteOffset[7] = {NOTE_C, NOTE_D, NOTE_E, NOTE_F, NOTE_G, NOTE_A, NOTE_B};

enum {
	WC_NOTE = 0,
	WC_OCT,
	WC_INST,
	WC_VOL,
	WC_EFF,
	WC_PARAM
};

class CPatternView;

bool IntQuery::IsMatch(int x) const
{
	if (x >= Current->Min && x <= Current->Max)
		return true;
	else if (Next == NULL)
		return false;
	else
		return Next->IsMatch(x);
}

bool IntQuery::IsSingle() const
{
	return Next == NULL && Current->Min == Current->Max;
}

void IntQuery::Join(IntRange *Range)
{
	if (Current == NULL)
		Current = Range;
	else if (Next == NULL)
		Next = new IntQuery(Range);
	else
		Next->Join(Range);
}

bool IntQuery::ParseTerm(IntQuery *target, CString &in, CString &err)
{
	CString str;

	if (in[0] == _T('[')) {
		in.Delete(0);
		str = in.SpanIncluding(_T("0123456789"));
		FIND_RAISE_ERROR(str.GetLength() != in.Find(_T(',')), _T("Unexpected characters found in integer range."));
		target->Current->Min = atoi(str);
		in.Delete(0, str.GetLength() + 1);
		str = in.SpanIncluding(_T("0123456789"));
		FIND_RAISE_ERROR(str.GetLength() != in.Find(_T(']')), _T("Unexpected characters found in integer range."));
		target->Current->Max = atoi(str);
		in.Delete(0, str.GetLength() + 1);
	}
	else {
		str = in.SpanIncluding(_T("0123456789"));
		FIND_RAISE_ERROR(str.GetLength() != in.GetLength(), _T("Unexpected characters found in integer."));
		target->Current->Min = atoi(str);
		target->Current->Max = atoi(str);
		in.Delete(0, str.GetLength());
	}

	FIND_RAISE_ERROR(target->Current->Min < 0 || target->Current->Min > 0xFF || target->Current->Max < 0 || target->Current->Max > 0xFF,
		_T("Integer range must not exceed 0 and 255."));

	return true;
}

bool IntQuery::ParseFull(CString &in, CString &err)
{
	int pos = 0;
	IntQuery *now = Next;

	if (in[0] != _T('(')) {
		if (!ParseTerm(this, in, err)) return false;
		FIND_RAISE_ERROR(in.GetLength(), _T("Unexpected characters found in integer query."));
		return true;
	}
	in.Delete(0);
	if (!ParseTerm(this, in, err)) return false;
	while (in[0] == _T(',')) {
		in.Delete(0);
		now->Next = new IntQuery();
		if (!ParseTerm(now, in, err)) return false;
		now = now->Next;
	}
	FIND_RAISE_ERROR(in != _T(")"), _T("Incomplete integer query."));

	return true;
}

searchTerm::searchTerm() :
	rowOffset(0),
	colOffset(0),
	NoiseChan(false)
{
	Note = new IntQuery();
	Oct  = new IntQuery();
	Inst = new IntQuery();
	Vol  = new IntQuery();
	for (int i = 0; i < MAX_EFFECT_COLUMNS; i++) {
		EffNumber[i] = new IntQuery();
		EffParam[i]  = new IntQuery();
	}
	for (int i = 0; i < 6; i++)
		Definite[i] = false;
}

void searchTerm::Release()
{
	SAFE_RELEASE(Note);
	SAFE_RELEASE(Oct);
	SAFE_RELEASE(Inst);
	SAFE_RELEASE(Vol);
	for (int i = 0; i < MAX_EFFECT_COLUMNS; i++) {
		SAFE_RELEASE(EffNumber[i]);
		SAFE_RELEASE(EffParam[i]);
	}
}

BOOL CFindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CheckRadioButton(IDC_RADIO_FIND_SIMPLE, IDC_RADIO_FIND_MACRO, IDC_RADIO_FIND_SIMPLE);
	CheckRadioButton(IDC_RADIO_REPLACE_SIMPLE, IDC_RADIO_REPLACE_MACRO, IDC_RADIO_REPLACE_SIMPLE);

	m_cFindNoteField     = new CEdit();
	m_cFindInstField     = new CEdit();
	m_cFindVolField      = new CEdit();
	m_cFindEffField      = new CEdit();
	m_cReplaceNoteField  = new CEdit();
	m_cReplaceInstField  = new CEdit();
	m_cReplaceVolField   = new CEdit();
	m_cReplaceEffField   = new CEdit();
	m_cFindMacroField    = new CEdit();
	m_cReplaceMacroField = new CEdit();
	m_cFilterMacroField  = new CEdit();

	m_cFindNoteField   ->SubclassDlgItem(IDC_EDIT_FIND_NOTE, this);
	m_cFindInstField   ->SubclassDlgItem(IDC_EDIT_FIND_INST, this);
	m_cFindVolField    ->SubclassDlgItem(IDC_EDIT_FIND_VOL, this);
	m_cFindEffField    ->SubclassDlgItem(IDC_EDIT_FIND_EFF, this);
	m_cReplaceNoteField->SubclassDlgItem(IDC_EDIT_REPLACE_NOTE, this);
	m_cReplaceInstField->SubclassDlgItem(IDC_EDIT_REPLACE_INST, this);
	m_cReplaceVolField ->SubclassDlgItem(IDC_EDIT_REPLACE_VOL, this);
	m_cReplaceEffField ->SubclassDlgItem(IDC_EDIT_REPLACE_EFF, this);
	m_cFindMacroField   ->SubclassDlgItem(IDC_EDIT_FIND_MACRO, this);
	m_cReplaceMacroField->SubclassDlgItem(IDC_EDIT_REPLACE_MACRO, this);
	m_cFilterMacroField ->SubclassDlgItem(IDC_EDIT_FILTER_MACRO, this);

	m_cSearchArea = new CComboBox();
	m_cEffectColumn = new CComboBox();
	m_cSearchArea->SubclassDlgItem(IDC_COMBO_FIND_IN, this);
	m_cEffectColumn->SubclassDlgItem(IDC_COMBO_EFFCOLUMN, this);
	m_cSearchArea->SetCurSel(0);
	m_cEffectColumn->SetCurSel(0);
	
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CFindDlg::UpdateFields()
{
	m_bFindMacro = IsDlgButtonChecked(IDC_RADIO_FIND_MACRO) ? true : false;
	if (m_bFindMacro) {
		CheckRadioButton(IDC_RADIO_REPLACE_SIMPLE, IDC_RADIO_REPLACE_MACRO, IDC_RADIO_REPLACE_MACRO);
		GetDlgItem(IDC_RADIO_REPLACE_SIMPLE)->EnableWindow(false);
	}
	else
		GetDlgItem(IDC_RADIO_REPLACE_SIMPLE)->EnableWindow(true);
	m_bReplaceMacro = m_bFindMacro || IsDlgButtonChecked(IDC_RADIO_REPLACE_MACRO) ? true : false;
	m_bFilterMacro = m_cSearchArea->GetCurSel() == 4 ? true : false;

	m_cFindNoteField->EnableWindow((!m_bFindMacro) && IsDlgButtonChecked(IDC_CHECK_FIND_NOTE));
	m_cFindInstField->EnableWindow((!m_bFindMacro) && IsDlgButtonChecked(IDC_CHECK_FIND_INST));
	m_cFindVolField->EnableWindow((!m_bFindMacro) && IsDlgButtonChecked(IDC_CHECK_FIND_VOL));
	m_cFindEffField->EnableWindow((!m_bFindMacro) && IsDlgButtonChecked(IDC_CHECK_FIND_EFF));
	m_cReplaceNoteField->EnableWindow(!m_bReplaceMacro && IsDlgButtonChecked(IDC_CHECK_REPLACE_NOTE));
	m_cReplaceInstField->EnableWindow(!m_bReplaceMacro && IsDlgButtonChecked(IDC_CHECK_REPLACE_INST));
	m_cReplaceVolField->EnableWindow(!m_bReplaceMacro && IsDlgButtonChecked(IDC_CHECK_REPLACE_VOL));
	m_cReplaceEffField->EnableWindow(!m_bReplaceMacro && IsDlgButtonChecked(IDC_CHECK_REPLACE_EFF));

	m_cFindMacroField->EnableWindow(m_bFindMacro);
	m_cReplaceMacroField->EnableWindow(m_bReplaceMacro);
	m_cFilterMacroField->EnableWindow(m_bFilterMacro);
}

void CFindDlg::OnUpdateFields(UINT nID)
{
	UpdateFields();
}

bool CFindDlg::ParseNote(searchTerm &Term, CString str, bool Simple, CString &err)
{
	Term.Definite[WC_NOTE] = false;
	Term.Definite[WC_OCT] = false;
	if (str.IsEmpty()) {
		Term.Definite[WC_NOTE] = true;
		Term.Definite[WC_OCT] = true;
		*Term.Note = NONE;
		*Term.Oct = 0;
	}
	if (str.Mid(1, 2) != _T("-#")) for (int i = 0; i < 7; i++) {
		if (str.Left(1).MakeUpper() == m_pNoteName[i]) {
			CString Accidental = _T("");
			Term.Definite[WC_NOTE] = true;
			*Term.Note = m_iNoteOffset[i];
			for (int j = 0; j < 3; j++)
				if (str[1] == m_pNoteSign[j]) {
					*Term.Note += j - 1;
					Accidental = str[1];
				}
			if (str.Delete(0, 1 + Accidental.GetLength())) {
				Term.Definite[WC_OCT] = true;
				*Term.Oct = atoi(str);
				FIND_RAISE_ERROR(*Term.Oct >= OCTAVE_RANGE || *Term.Oct < 0,
					_T("Note octave \"") + str + _T("\" is out of range, maximum is 7."));
			}
			if (*Term.Note > NOTE_RANGE) { *Term.Note -= NOTE_RANGE; *Term.Oct += 1; }
			if (*Term.Note < 1) { *Term.Note += NOTE_RANGE; *Term.Oct -= 1; }
			FIND_RAISE_ERROR(Term.Definite[WC_OCT] && (*Term.Oct >= OCTAVE_RANGE),
				_T("Actual note octave \"" + str + "\" is out of range, check if the note contains Cb or B#."));
			return true;
		}
	}
	if (!Term.Definite[WC_NOTE]) {
		if (str.Right(2) == _T("-#") && str.GetLength() == 3) {
			int NoteValue = static_cast<unsigned char>(strtol(str.Left(1), NULL, 16));
			Term.Definite[WC_NOTE] = true;
			Term.Definite[WC_OCT] = true;
			*Term.Note = NoteValue % NOTE_RANGE + 1;
			*Term.Oct = NoteValue / NOTE_RANGE;
			Term.NoiseChan = true;
		}
		else if (str == _T("-") || str == _T("---")) {
			Term.Definite[WC_NOTE] = true;
			Term.Definite[WC_OCT] = true;
			*Term.Note = HALT;
			*Term.Oct = 0;
		}
		else if (str == _T("=") || str == _T("===")) {
			Term.Definite[WC_NOTE] = true;
			Term.Definite[WC_OCT] = true;
			*Term.Note = RELEASE;
			*Term.Oct = 0;
		}
		else if (str == _T(".")) {
			Term.Definite[WC_NOTE] = true;
			Term.Note->Current->Min = NONE + 1;
			Term.Note->Current->Max = ECHO;
		}
		else if (str.Left(1) == _T("^")) {
			Term.Definite[WC_NOTE] = true;
			*Term.Note = ECHO;
			if (str.Delete(0)) {
				Term.Definite[WC_OCT] = true;
				*Term.Oct = atoi(str);
				FIND_RAISE_ERROR(*Term.Oct > ECHO_BUFFER_LENGTH,
					_T("Echo buffer access \"^") + str + _T("\" is out of range, maximum is %d."), ECHO_BUFFER_LENGTH);
			}
		}
		else {
			int NoteValue = atoi(str);
			FIND_RAISE_ERROR(NoteValue == 0 && str.Left(1) != _T("0"),
				_T("Invalid note \"" + str + "\"."));
			FIND_RAISE_ERROR(NoteValue >= NOTE_COUNT || NoteValue < 0,
				_T("Note value \"") + str + _T("\" is out of range, maximum is 95."));
			Term.Definite[WC_NOTE] = true;
			Term.Definite[WC_OCT] = true;
			*Term.Note = NoteValue % NOTE_RANGE + 1;
			*Term.Oct = NoteValue / NOTE_RANGE;
		}
	}

	return true;
}

bool CFindDlg::ParseInst(searchTerm &Term, CString str, bool Simple, CString &err)
{
	Term.Definite[WC_INST] = true;
	if (str.IsEmpty())
		*Term.Inst = MAX_INSTRUMENTS;
	else if (str == _T(".")) {
		Term.Inst->Current->Min = 0;
		Term.Inst->Current->Max = MAX_INSTRUMENTS - 1;
	}
	else {
		*Term.Inst = static_cast<unsigned char>(strtol(str, NULL, 16));
		FIND_RAISE_ERROR(*Term.Inst >= MAX_INSTRUMENTS,
			_T("Instrument \"") + str + _T("\" is out of range, maximum is %2X."), MAX_INSTRUMENTS - 1);
	}

	return true;
}

bool CFindDlg::ParseVol(searchTerm &Term, CString str, bool Simple, CString &err)
{
	Term.Definite[WC_VOL] = true;
	if (str.IsEmpty())
		*Term.Vol = MAX_VOLUME;
	else if (str == _T(".")) {
		Term.Vol->Current->Min = 0;
		Term.Vol->Current->Max = MAX_VOLUME - 1;
	}
	else {
		*Term.Vol = static_cast<unsigned char>(strtol(str, NULL, 16));
		FIND_RAISE_ERROR(*Term.Vol >= MAX_VOLUME,
			_T("Channel volume \"") + str + _T("\" is out of range, maximum is F."));
	}

	return true;
}

bool CFindDlg::ParseEff(searchTerm &Term, CString str, bool Simple, CString &err)
{
	FIND_RAISE_ERROR(str.GetLength() > 3,
		_T("Effect \"") + str.Left(1) + _T("\" is too long."));
	FIND_RAISE_ERROR(str.GetLength() == 2,
		_T("Effect \"") + str.Left(1) + _T("\" is too short."));

	if (str.IsEmpty()) {
		Term.Definite[WC_EFF] = true;
		Term.Definite[WC_PARAM] = true;
		*Term.EffNumber[0] = 0;
		*Term.EffParam[0] = 0;
	}
	else if (str == _T(".")) {
		Term.Definite[WC_EFF] = true;
		Term.EffNumber[0]->Current->Min = 1;
		Term.EffNumber[0]->Current->Max = EF_COUNT - 1;
	}
	else {
		SAFE_RELEASE(Term.EffNumber[0]->Next);
		SAFE_RELEASE(Term.EffNumber[0]->Current);
		for (unsigned char i = 0; i <= EF_COUNT; i++) {
			if (str.Left(1) == EFF_CHAR[i]) {
				Term.Definite[WC_EFF] = true;
				Term.EffNumber[0]->Join(new IntRange(i + 1, i + 1));
			}
		}
		FIND_RAISE_ERROR(Term.EffNumber[0]->Current == NULL,
			_T("Unknown effect \"") + str.Left(1) + _T("\" found in search query."));
	}
	if (str.GetLength() > 1) {
		Term.Definite[WC_PARAM] = true;
		*Term.EffParam[0] = static_cast<unsigned char>(strtol(str.Right(2), NULL, 16));
	}

	return true;
}

#define RETURN_ERROR(str) {AfxMessageBox(str, MB_OK | MB_ICONSTOP); m_searchTerm = old; return false; }
bool CFindDlg::GetSimpleFindTerm()
{
	CString str = _T(""), err = _T("");
	searchTerm old = m_searchTerm;
	*m_searchTerm.Inst = MAX_INSTRUMENTS;
	*m_searchTerm.Vol = MAX_VOLUME;
	for (int i = 0; i <= 6; i++)
		m_searchTerm.Definite[i] = false;

	if (IsDlgButtonChecked(IDC_CHECK_FIND_NOTE)) {
		m_cFindNoteField->GetWindowText(str);
		if (!ParseNote(m_searchTerm, str, true, err))
			RETURN_ERROR(err);
	}
	if (IsDlgButtonChecked(IDC_CHECK_FIND_INST)) {
		m_cFindInstField->GetWindowText(str);
		if (!ParseInst(m_searchTerm, str, true, err))
			RETURN_ERROR(err);
	}
	if (IsDlgButtonChecked(IDC_CHECK_FIND_VOL)) {
		m_cFindVolField->GetWindowText(str);
		if (!ParseVol(m_searchTerm, str, true, err))
			RETURN_ERROR(err);
	}
	if (IsDlgButtonChecked(IDC_CHECK_FIND_EFF)) {
		m_cFindEffField->GetWindowText(str);
		if (!ParseEff(m_searchTerm, str, true, err))
			RETURN_ERROR(err);
	}

	for (int i = 0; i <= 6; i++) {
		if (i == 6)
			RETURN_ERROR(_T("Search query is empty."));
		if (m_searchTerm.Definite[i]) break;
	}

	return true;
}
#undef RETURN_ERROR

#define RETURN_ERROR(str) {AfxMessageBox(str, MB_OK | MB_ICONSTOP); m_replaceTerm = old; return false; }
bool CFindDlg::GetSimpleReplaceTerm()
{
	CString str = _T(""), err = _T("");
	searchTerm old = m_replaceTerm;
	*m_replaceTerm.Inst = MAX_INSTRUMENTS;
	*m_replaceTerm.Vol = MAX_VOLUME;
	for (int i = 0; i <= 6; i++)
		m_replaceTerm.Definite[i] = false;

	if (IsDlgButtonChecked(IDC_CHECK_REPLACE_NOTE)) {
		m_cReplaceNoteField->GetWindowText(str);
		if (!ParseNote(m_replaceTerm, str, true, err))
			RETURN_ERROR(err);
	}
	if (IsDlgButtonChecked(IDC_CHECK_REPLACE_INST)) {
		m_cReplaceInstField->GetWindowText(str);
		if (!ParseInst(m_replaceTerm, str, true, err))
			RETURN_ERROR(err);
	}
	if (IsDlgButtonChecked(IDC_CHECK_REPLACE_VOL)) {
		m_cReplaceVolField->GetWindowText(str);
		if (!ParseVol(m_replaceTerm, str, true, err))
			RETURN_ERROR(err);
	}
	if (IsDlgButtonChecked(IDC_CHECK_REPLACE_EFF)) {
		m_cReplaceEffField->GetWindowText(str);
		if (!ParseEff(m_replaceTerm, str, true, err))
			RETURN_ERROR(err);
	}

	for (int i = 0; i <= 6; i++) {
		if (i == 6)
			RETURN_ERROR(_T("Replacement query is empty."));
		if (m_replaceTerm.Definite[i]) break;
	}
	bool Wildcard = false;
	if (m_replaceTerm.Definite[WC_NOTE] && !m_replaceTerm.Note->IsSingle()) Wildcard = true;
	if (m_replaceTerm.Definite[WC_OCT]  && !m_replaceTerm.Oct->IsSingle()) Wildcard = true;
	if (m_replaceTerm.Definite[WC_INST] && !m_replaceTerm.Inst->IsSingle()) Wildcard = true;
	if (m_replaceTerm.Definite[WC_VOL]  && !m_replaceTerm.Vol->IsSingle()) Wildcard = true;
	//if (m_replaceTerm.Definite[WC_EFF]) for (int i = 0; i < MAX_EFFECT_COLUMNS; i++) {
	//	if (!m_replaceTerm.EffNumber[i]->IsSingle()) Wildcard = false;
	//}
	if (m_replaceTerm.Definite[WC_PARAM]) for (int i = 0; i < MAX_EFFECT_COLUMNS; i++) {
		if (!m_replaceTerm.EffParam[i]->IsSingle()) Wildcard = false;
	}
	if (Wildcard)
		RETURN_ERROR(_T("Replacement query cannot contain wildcards."));

	if (IsDlgButtonChecked(IDC_CHECK_FIND_REMOVE)) {
		if (m_replaceTerm.Definite[WC_NOTE] && !m_replaceTerm.Definite[WC_OCT])
			RETURN_ERROR(_T("Simple replacement query cannot contain a note with an unspecified octave if the option \"Remove original data\" is enabled."));
		if (m_replaceTerm.Definite[WC_EFF] && !m_replaceTerm.Definite[WC_PARAM])
			RETURN_ERROR(_T("Simple replacement query cannot contain an effect with an unspecified parameter if the option \"Remove original data\" is enabled."));
	}
	
	return true;
}
#undef RETURN_ERROR

replaceTerm CFindDlg::toReplace(const searchTerm x)
{
	replaceTerm Term;
	Term.Note.Note = x.Note->Current->Min;
	Term.Note.Octave = x.Oct->Current->Min;
	Term.Note.Instrument = x.Inst->Current->Min;
	Term.Note.Vol = x.Vol->Current->Min;
	Term.rowOffset = x.rowOffset;
	Term.colOffset = x.colOffset;
	Term.NoiseChan = x.NoiseChan;
	for (int i = 0; i < MAX_EFFECT_COLUMNS; i++) {
		Term.Note.EffNumber[i] = x.EffNumber[i]->Current->Min;
		Term.Note.EffParam[i] = x.EffParam[i]->Current->Min;
	}
	for (int i = 0; i < 6; i++) {
		Term.Definite[i] = x.Definite[i];
	}

	return Term;
}

searchTerm CFindDlg::toSearch(const replaceTerm x)
{
	searchTerm Term;
	*Term.Note = x.Note.Note;
	*Term.Oct = x.Note.Octave;
	*Term.Inst = x.Note.Instrument;
	*Term.Vol = x.Note.Vol;
	Term.rowOffset = x.rowOffset;
	Term.colOffset = x.colOffset;
	Term.NoiseChan = x.NoiseChan;
	for (int i = 0; i < MAX_EFFECT_COLUMNS; i++) {
		*Term.EffNumber[i] = x.Note.EffNumber[i];
		*Term.EffParam[i] = x.Note.EffParam[i];
	}
	for (int i = 0; i < 6; i++) {
		Term.Definite[i] = x.Definite[i];
	}

	return Term;
}

bool CFindDlg::CompareFields(const stChanNote Target, bool Noise, int EffCount)
{
	int EffColumn = m_cEffectColumn->GetCurSel();
	if (EffColumn > EffCount) return false;
	bool EffectMatch = false;

	replaceTerm Term = toReplace(m_searchTerm);

	if (Term.Definite[WC_NOTE]) {
		if (Term.NoiseChan) {
			if (!Noise) return false;
			if (Term.Note.Note < NOTE_C || Term.Note.Note > NOTE_B) {
				if (!m_searchTerm.Note->IsMatch(Target.Note)) return false;
			}
			else if (MIDI_NOTE(Term.Note.Octave, Term.Note.Note) != MIDI_NOTE(Target.Octave, Target.Note) % 16) return false;
		}
		else {
			if (Noise && Term.Note.Note >= NOTE_C && Term.Note.Note <= NOTE_B) return false;
			if (!m_searchTerm.Note->IsMatch(Target.Note)) return false;
			if (Term.Definite[WC_OCT] && !m_searchTerm.Oct->IsMatch(Target.Octave)
				&& (Term.Note.Note >= NOTE_C && Term.Note.Note <= NOTE_B || Term.Note.Note == ECHO))
					return false;
		}
	}
	if (Term.Definite[WC_INST] && !m_searchTerm.Inst->IsMatch(Target.Instrument)) return false;
	if (Term.Definite[WC_VOL] && !m_searchTerm.Vol->IsMatch(Target.Vol)) return false;
	for (int i = EffColumn % MAX_EFFECT_COLUMNS; i <= std::min(std::min(EffColumn, MAX_EFFECT_COLUMNS - 1), EffCount); i++) {
		if ((!Term.Definite[WC_EFF] || m_searchTerm.EffNumber[0]->IsMatch(Target.EffNumber[i]))
		&& (!Term.Definite[WC_PARAM] || m_searchTerm.EffParam[0]->IsMatch(Target.EffParam[i])))
			EffectMatch = true;
	}
	if (!EffectMatch) return false;

	return true;
}

bool CFindDlg::Find(bool ShowEnd)
{
	m_pDocument = static_cast<CFamiTrackerDoc*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument());
	m_pView = static_cast<CFamiTrackerView*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveView());
	stChanNote Target;
	unsigned int Filter = m_cSearchArea->GetCurSel();
	int Track = static_cast<CMainFrame*>(theApp.m_pMainWnd)->GetSelectedTrack();

	if (m_bFindMacro) { m_bFound = false; return false; } // 0CC: unimplemented

	unsigned int BeginFrame = m_bVisible ? m_pView->GetSelectedFrame() : m_iFrame,
				 BeginRow   = m_bVisible ? m_pView->GetSelectedRow() : m_iRow,
				 BeginChan  = m_bVisible ? m_pView->GetSelectedChannel() : m_iChannel;
	bool bFirst = true, bSecond = false, bVertical = IsDlgButtonChecked(IDC_CHECK_VERTICAL_SEARCH) == 1;

	const unsigned int EndFrame = FIND_SINGLE_FRAME(Filter) ? BeginFrame + 1 : m_pDocument->GetFrameCount(Track);
	for (unsigned int i = FIND_SINGLE_FRAME(Filter) ? BeginFrame : 0; i <= EndFrame; i++) {
		if (!bSecond && i == (FIND_SINGLE_FRAME(Filter) ? BeginFrame + 1 : m_pDocument->GetFrameCount(Track))) {
			bSecond = true;
			i = FIND_SINGLE_FRAME(Filter) ? BeginFrame : 0;
		}
		int j, k;
		int jLimit = m_pView->GetPatternEditor()->GetCurrentPatternLength(i);
		int kStart = FIND_SINGLE_CHANNEL(Filter) ? BeginChan : 0;
		int kLimit = FIND_SINGLE_CHANNEL(Filter) ? BeginChan + 1 : m_pDocument->GetChannelCount();
		for (int jk = 0; jk < jLimit * (kLimit - kStart); jk++) {
			if (bFirst) {
				bFirst = false;
				i = BeginFrame; j = BeginRow; k = BeginChan;
				jLimit = m_pView->GetPatternEditor()->GetCurrentPatternLength(i);
				if (bVertical) jk = j + (k - kStart) * jLimit;
				else jk = j * (kLimit - kStart) + (k - kStart);
				if (m_bSkipFirst) continue;
			}
			if (bVertical) {
				j = jk % jLimit; k = jk / jLimit + kStart;
			}
			else {
				j = jk / (kLimit - kStart); k = jk % (kLimit - kStart) + kStart;
			}
			m_pDocument->GetNoteData(Track, i, k, j, &Target);

			if (CompareFields(Target, k == CHANID_NOISE, m_pDocument->GetEffColumns(Track, k))) {
				m_iFrame = i;
				m_iRow = j;
				m_iChannel = k;
				if (m_bVisible) {
					m_pView->SelectFrame(i);
					m_pView->SelectRow(j);
					m_pView->SelectChannel(k);
				}
				m_bSkipFirst = true; m_bFound = true; return m_bFound;
			}
			if (bSecond && i == BeginFrame && j == BeginRow && k == BeginChan) {
				if (ShowEnd) AfxMessageBox(IDS_FIND_NONE, MB_OK | MB_ICONINFORMATION);
				m_bFound = false; return m_bFound;
			}
		}
	}

	m_bSkipFirst = true;
	m_bFound = false; return m_bFound;
}

bool CFindDlg::Replace(bool CanUndo)
{
	m_pDocument = static_cast<CFamiTrackerDoc*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument());
	m_pView = static_cast<CFamiTrackerView*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveView());
	stChanNote Target;
	int Track = static_cast<CMainFrame*>(AfxGetMainWnd())->GetSelectedTrack();

	if (m_bReplaceMacro) return false; // 0CC: unimplemented

	replaceTerm Replace = toReplace(m_replaceTerm);

	if (m_bFound) {
		m_pDocument->GetNoteData(Track, m_iFrame, m_iChannel, m_iRow, &Target);
		int EffColumn = m_cEffectColumn->GetCurSel();
		if (EffColumn == MAX_EFFECT_COLUMNS && (Replace.Definite[WC_EFF] || Replace.Definite[WC_PARAM])) {
			for (int i = 0; i < MAX_EFFECT_COLUMNS && EffColumn != MAX_EFFECT_COLUMNS; i++)
				if (Replace.Note.EffNumber[0] == Target.EffNumber[i] && Replace.Note.EffParam[0] == Target.EffParam[i])
					EffColumn = i;
		}

		if (Replace.Definite[WC_NOTE])
			Target.Note = Replace.Note.Note;
		else if (IsDlgButtonChecked(IDC_CHECK_FIND_REMOVE))
			Target.Note = NONE;
		if (Replace.Definite[WC_OCT])
			Target.Octave = Replace.Note.Octave;
		else if (IsDlgButtonChecked(IDC_CHECK_FIND_REMOVE))
			Target.Octave = 0;
		if (Replace.Definite[WC_INST])
			Target.Instrument = Replace.Note.Instrument;
		else if (IsDlgButtonChecked(IDC_CHECK_FIND_REMOVE))
			Target.Instrument = MAX_INSTRUMENTS;
		if (Replace.Definite[WC_VOL])
			Target.Vol = Replace.Note.Vol;
		else if (IsDlgButtonChecked(IDC_CHECK_FIND_REMOVE))
			Target.Vol = MAX_VOLUME;

		if (Replace.Definite[WC_EFF]) {
			switch (m_pDocument->GetChipType(m_pView->GetSelectedChannel())) {
			case SNDCHIP_FDS:
				switch (Replace.Note.EffNumber[0]) {
				case EF_SWEEPUP: Replace.Note.EffNumber[0] = EF_FDS_MOD_DEPTH; break;
				case EF_SWEEPDOWN: Replace.Note.EffNumber[0] = EF_FDS_MOD_SPEED_HI; break;
				case EF_VOLUME: Replace.Note.EffNumber[0] = EF_FDS_VOLUME; break;
				} break;
			case SNDCHIP_S5B:
				switch (Replace.Note.EffNumber[0]) {
				case EF_SWEEPUP: Replace.Note.EffNumber[0] = EF_SUNSOFT_ENV_LO; break;
				case EF_SWEEPDOWN: Replace.Note.EffNumber[0] = EF_SUNSOFT_ENV_HI; break;
				case EF_FDS_MOD_SPEED_LO: Replace.Note.EffNumber[0] = EF_SUNSOFT_ENV_TYPE; break;
				} break;
			case SNDCHIP_N163:
				switch (Replace.Note.EffNumber[0]) {
				case EF_DAC: Replace.Note.EffNumber[0] = EF_N163_WAVE_BUFFER; break;
				} break;
			}
			Target.EffNumber[EffColumn]	= Replace.Note.EffNumber[0];
		}
		else if (IsDlgButtonChecked(IDC_CHECK_FIND_REMOVE)) Target.EffNumber[EffColumn] = EF_NONE;
		if (Replace.Definite[WC_PARAM]) Target.EffParam[EffColumn] = Replace.Note.EffParam[0];
		else if (IsDlgButtonChecked(IDC_CHECK_FIND_REMOVE)) Target.EffParam[EffColumn] = 0;
		if (CanUndo) m_pView->EditReplace(Target);
		m_pDocument->SetNoteData(Track, m_iFrame, m_iChannel, m_iRow, &Target);
		m_bFound = false;
		return true;
	}
	else {
		m_bSkipFirst = false;
		return false;
	}
}

void CFindDlg::OnBnClickedButtonFindNext()
{
	if (!GetSimpleFindTerm()) return;
	m_bVisible = true;
	Find(true);
	m_pView->SetFocus();
}

void CFindDlg::OnBnClickedButtonReplace()
{
	if (!GetSimpleFindTerm()) return;
	if (!GetSimpleReplaceTerm()) return;
	if (m_cEffectColumn->GetCurSel() == MAX_EFFECT_COLUMNS) {
		AfxMessageBox(_T("\"Any\" cannot be used as the effect column scope for replacing."), MB_OK | MB_ICONSTOP);
		return;
	}
	m_pView = static_cast<CFamiTrackerView*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveView());
	if (!m_pView->GetEditMode()) return;
	if (theApp.IsPlaying() && m_pView->GetFollowMode())
		return;

	if (IsDlgButtonChecked(IDC_CHECK_REPLACE_ALL)) {
		unsigned int Count = 0;
		CString str;
		
		unsigned int Filter = m_cSearchArea->GetCurSel();
		unsigned int ZipPos = 0, PrevPos = 0;
		m_iFrame = FIND_SINGLE_FRAME(Filter) ? m_pView->GetSelectedFrame() : 0;
		m_iChannel = FIND_SINGLE_CHANNEL(Filter) ? m_pView->GetSelectedChannel() : 0;
		m_iRow = 0;

		m_bSkipFirst = false;
		m_bVisible = false;

		Find();
		while (m_bFound) {
			Replace();
			Count++;
			Find();
			if (IsDlgButtonChecked(IDC_CHECK_VERTICAL_SEARCH))
				ZipPos = (m_iFrame << 16) + (m_iChannel << 8) + m_iRow;
			else
				ZipPos = (m_iFrame << 16) + (m_iRow << 8) + m_iChannel;
			if (ZipPos <= PrevPos) break;
			else PrevPos = ZipPos;
		}

		m_pDocument->UpdateAllViews(NULL, UPDATE_PATTERN);

		str.Format(_T("%d occurrence(s) replaced."), Count);
		AfxMessageBox(str, MB_OK | MB_ICONINFORMATION);
	}
	else {
		m_bVisible = true;
		Replace(true);
		Find();
	}
}

void CFindDlg::Reset()
{
	m_bFound = false;
}