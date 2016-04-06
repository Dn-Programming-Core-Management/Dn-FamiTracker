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

#include <memory>
#include <cstdarg>
#include <stdexcept>
#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "PatternEditor.h"
#include "FindDlg.h"

#define FIND_SINGLE_CHANNEL(x) ( ((x) & 0x01) == 0x01 )
#define FIND_SINGLE_FRAME(x) ( ((x) & 0x02) == 0x02 )
#define FIND_WILD _T("*")
#define FIND_BLANK _T("!")

// CFindDlg dialog

IMPLEMENT_DYNAMIC(CFindDlg, CDialog)

CFindDlg::CFindDlg(CWnd* pParent /*=NULL*/) : CDialog(CFindDlg::IDD, pParent),
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
	SAFE_RELEASE(m_cFindNoteField2);
	SAFE_RELEASE(m_cFindInstField);
	SAFE_RELEASE(m_cFindInstField2);
	SAFE_RELEASE(m_cFindVolField);
	SAFE_RELEASE(m_cFindVolField2);
	SAFE_RELEASE(m_cFindEffField);
	SAFE_RELEASE(m_cReplaceNoteField);
	SAFE_RELEASE(m_cReplaceInstField);
	SAFE_RELEASE(m_cReplaceVolField);
	SAFE_RELEASE(m_cReplaceEffField);
	SAFE_RELEASE(m_cSearchArea);
	SAFE_RELEASE(m_cEffectColumn);
}

void CFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CFindDlg, CDialog)
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

#pragma warning ( disable : 4351 ) // "new behaviour: elements of array [...] will be default initialized"

searchTerm::searchTerm() :
	NoiseChan(false),
	Note(new CharRange()),
	Oct(new CharRange()),
	Inst(new CharRange(0, MAX_INSTRUMENTS)),
	Vol(new CharRange(0, MAX_VOLUME)),
	EffParam(new CharRange()),
	Definite(),
	EffNumber()
{
}

searchTerm::searchTerm(searchTerm &&other) :
	NoiseChan(other.NoiseChan),
	Note(std::move(other.Note)),
	Oct(std::move(other.Oct)),
	Inst(std::move(other.Inst)),
	Vol(std::move(other.Vol)),
	EffParam(std::move(other.EffParam))
{
	memcpy(Definite, other.Definite, sizeof(Definite));
	memcpy(EffNumber, other.EffNumber, sizeof(EffNumber));
}

searchTerm& searchTerm::operator=(searchTerm &&other)
{
	NoiseChan = other.NoiseChan;
	Note.swap(other.Note);
	Oct.swap(other.Oct);
	Inst.swap(other.Inst);
	Vol.swap(other.Vol);
	EffParam.swap(other.EffParam);

	memcpy(Definite, other.Definite, sizeof(Definite));
	memcpy(EffNumber, other.EffNumber, sizeof(EffNumber));

	return *this;
}

BOOL CFindDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_cFindNoteField     = new CEdit();
	m_cFindNoteField2    = new CEdit();
	m_cFindInstField     = new CEdit();
	m_cFindInstField2    = new CEdit();
	m_cFindVolField      = new CEdit();
	m_cFindVolField2     = new CEdit();
	m_cFindEffField      = new CEdit();
	m_cReplaceNoteField  = new CEdit();
	m_cReplaceInstField  = new CEdit();
	m_cReplaceVolField   = new CEdit();
	m_cReplaceEffField   = new CEdit();

	m_cFindNoteField    ->SubclassDlgItem(IDC_EDIT_FIND_NOTE, this);
	m_cFindNoteField2   ->SubclassDlgItem(IDC_EDIT_FIND_NOTE2, this);
	m_cFindInstField    ->SubclassDlgItem(IDC_EDIT_FIND_INST, this);
	m_cFindInstField2   ->SubclassDlgItem(IDC_EDIT_FIND_INST2, this);
	m_cFindVolField     ->SubclassDlgItem(IDC_EDIT_FIND_VOL, this);
	m_cFindVolField2    ->SubclassDlgItem(IDC_EDIT_FIND_VOL2, this);
	m_cFindEffField     ->SubclassDlgItem(IDC_EDIT_FIND_EFF, this);
	m_cReplaceNoteField ->SubclassDlgItem(IDC_EDIT_REPLACE_NOTE, this);
	m_cReplaceInstField ->SubclassDlgItem(IDC_EDIT_REPLACE_INST, this);
	m_cReplaceVolField  ->SubclassDlgItem(IDC_EDIT_REPLACE_VOL, this);
	m_cReplaceEffField  ->SubclassDlgItem(IDC_EDIT_REPLACE_EFF, this);

	m_cSearchArea = new CComboBox();
	m_cEffectColumn = new CComboBox();
	m_cSearchArea->SubclassDlgItem(IDC_COMBO_FIND_IN, this);
	m_cEffectColumn->SubclassDlgItem(IDC_COMBO_EFFCOLUMN, this);
	m_cSearchArea->SetCurSel(0);
	m_cEffectColumn->SetCurSel(0);

	m_cFindNoteField   ->SetLimitText(3);
	m_cFindNoteField2  ->SetLimitText(3);
	m_cFindInstField   ->SetLimitText(2);
	m_cFindInstField2  ->SetLimitText(2);
	m_cFindVolField    ->SetLimitText(1);
	m_cFindVolField2   ->SetLimitText(1);
	m_cFindEffField    ->SetLimitText(3);
	m_cReplaceNoteField->SetLimitText(3);
	m_cReplaceInstField->SetLimitText(2);
	m_cReplaceVolField ->SetLimitText(1);
	m_cReplaceEffField ->SetLimitText(3);
	
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CFindDlg::UpdateFields()
{
	m_cFindNoteField->EnableWindow(IsDlgButtonChecked(IDC_CHECK_FIND_NOTE));
	m_cFindNoteField2->EnableWindow(IsDlgButtonChecked(IDC_CHECK_FIND_NOTE));
	m_cFindInstField->EnableWindow(IsDlgButtonChecked(IDC_CHECK_FIND_INST));
	m_cFindInstField2->EnableWindow(IsDlgButtonChecked(IDC_CHECK_FIND_INST));
	m_cFindVolField->EnableWindow(IsDlgButtonChecked(IDC_CHECK_FIND_VOL));
	m_cFindVolField2->EnableWindow(IsDlgButtonChecked(IDC_CHECK_FIND_VOL));
	m_cFindEffField->EnableWindow(IsDlgButtonChecked(IDC_CHECK_FIND_EFF));
	m_cReplaceNoteField->EnableWindow(IsDlgButtonChecked(IDC_CHECK_REPLACE_NOTE));
	m_cReplaceInstField->EnableWindow(IsDlgButtonChecked(IDC_CHECK_REPLACE_INST));
	m_cReplaceVolField->EnableWindow(IsDlgButtonChecked(IDC_CHECK_REPLACE_VOL));
	m_cReplaceEffField->EnableWindow(IsDlgButtonChecked(IDC_CHECK_REPLACE_EFF));
}

void CFindDlg::OnUpdateFields(UINT nID)
{
	UpdateFields();
}

void CFindDlg::ParseNote(searchTerm &Term, CString str, bool Half)
{
	if (!Half) Term.Definite[WC_NOTE] = Term.Definite[WC_OCT] = false;

	if (str.IsEmpty()) {
		if (!Half) {
			Term.Definite[WC_NOTE] = true;
			Term.Definite[WC_OCT] = true;
			Term.Note->Set(NONE);
			Term.Oct->Set(0);
		}
		else {
			//Term.Note->Max = Term.Note->Min;
			//Term.Oct->Max = Term.Oct->Min;
		}
		return;
	}

	RaiseIf(Half && (!Term.Note->IsSingle() || !Term.Oct->IsSingle()),
		_T("Cannot use wildcards in a range search query."));

	if (str == _T("-") || str == _T("---")) {
		RaiseIf(Half, _T("Cannot use note cut in a range search query."));
		Term.Definite[WC_NOTE] = true;
		Term.Definite[WC_OCT] = true;
		Term.Note->Set(HALT);
		Term.Oct->Min = 0; Term.Oct->Max = 7;
		return;
	}

	if (str == _T("=") || str == _T("===")) {
		RaiseIf(Half, _T("Cannot use note release in a range search query."));
		Term.Definite[WC_NOTE] = true;
		Term.Definite[WC_OCT] = true;
		Term.Note->Set(RELEASE);
		Term.Oct->Min = 0; Term.Oct->Max = 7;
		return;
	}

	if (str == _T(".")) {
		RaiseIf(Half, _T("Cannot use wildcards in a range search query."));
		Term.Definite[WC_NOTE] = true;
		Term.Note->Min = NONE + 1;
		Term.Note->Max = ECHO;
		return;
	}

	if (str.Left(1) == _T("^")) {
		RaiseIf(Half && !Term.Definite[WC_OCT], _T("Cannot use wildcards in a range search query."));
		Term.Definite[WC_NOTE] = true;
		Term.Definite[WC_OCT] = true;
		Term.Note->Set(ECHO);
		if (str.Delete(0)) {
			RaiseIf(atoi(str) > ECHO_BUFFER_LENGTH,
				_T("Echo buffer access \"^%s\" is out of range, maximum is %d."), str, ECHO_BUFFER_LENGTH);
			Term.Oct->Set(atoi(str), Half);
		}
		else {
			Term.Oct->Min = 0; Term.Oct->Max = ECHO_BUFFER_LENGTH;
		}
		return;
	}

	if (str.Mid(1, 2) != _T("-#")) for (int i = 0; i < 7; i++) {
		if (str.Left(1).MakeUpper() == m_pNoteName[i]) {
			Term.Definite[WC_NOTE] = true;
			int Note = m_iNoteOffset[i];
			int Oct = 0;
			for (int j = 0; j < 3; j++) if (str[1] == m_pNoteSign[j]) {
				Note += j - 1;
				str.Delete(0); break;
			}
			if (str.Delete(0)) {
				Term.Definite[WC_OCT] = true;
				RaiseIf(str.SpanIncluding("0123456789") != str, _T("Unknown note octave."));
				Oct = atoi(str);
				RaiseIf(Oct >= OCTAVE_RANGE || Oct < 0,
					_T("Note octave \"%s\" is out of range, maximum is %d."), str, OCTAVE_RANGE - 1);
				Term.Oct->Set(Oct, Half);
			}
			else RaiseIf(Half, _T("Cannot use wildcards in a range search query.")); 
			while (Note > NOTE_RANGE) { Note -= NOTE_RANGE; if (Term.Definite[WC_OCT]) Term.Oct->Set(++Oct, Half); }
			while (Note < NOTE_C) { Note += NOTE_RANGE; if (Term.Definite[WC_OCT]) Term.Oct->Set(--Oct, Half); }
			Term.Note->Set(Note, Half);
			RaiseIf(Term.Definite[WC_OCT] && (Oct >= OCTAVE_RANGE || Oct < 0),
				_T("Note octave \"%s\" is out of range, check if the note contains Cb or B#."), str);
			return;
		}
	}

	if (str.Right(2) == _T("-#") && str.GetLength() == 3) {
		int NoteValue = static_cast<unsigned char>(strtol(str.Left(1), NULL, 16));
		Term.Definite[WC_NOTE] = true;
		Term.Definite[WC_OCT] = true;
		if (str.Left(1) == _T(".")) {
			Term.Note->Min = 1; Term.Note->Max = 4;
			Term.Oct->Min = 0; Term.Oct->Max = 1;
		}
		else {
			Term.Note->Set(NoteValue % NOTE_RANGE + 1, Half);
			Term.Oct->Set(NoteValue / NOTE_RANGE, Half);
		}
		Term.NoiseChan = true;
		return;
	}

	if (str.SpanIncluding("0123456789") == str) {
		int NoteValue = atoi(str);
		RaiseIf(NoteValue == 0 && str.Left(1) != _T("0"), _T("Invalid note \"%s\"."), str);
		RaiseIf(NoteValue >= NOTE_COUNT || NoteValue < 0,
			_T("Note value \"%s\" is out of range, maximum is %d."), str, NOTE_COUNT - 1);
		Term.Definite[WC_NOTE] = true;
		Term.Definite[WC_OCT] = true;
		Term.Note->Set(NoteValue % NOTE_RANGE + 1, Half);
		Term.Oct->Set(NoteValue / NOTE_RANGE, Half);
		return;
	}

	RaiseIf(true, _T("Unknown note query."));
}

void CFindDlg::ParseInst(searchTerm &Term, CString str, bool Half)
{
	Term.Definite[WC_INST] = true;
	if (str.IsEmpty() && !Half) {
		Term.Inst->Set(MAX_INSTRUMENTS);
		return;
	}
	RaiseIf(Half && !Term.Inst->IsSingle(), _T("Cannot use wildcards in a range search query."));

	if (str == _T(".")) {
		RaiseIf(Half, _T("Cannot use wildcards in a range search query."));
		Term.Inst->Min = 0;
		Term.Inst->Max = MAX_INSTRUMENTS - 1;
	}
	else if (!str.IsEmpty()) {
		unsigned char Val = static_cast<unsigned char>(strtol(str, NULL, 16));
		RaiseIf(Val >= MAX_INSTRUMENTS,
			_T("Instrument \"%s\" is out of range, maximum is %X."), str, MAX_INSTRUMENTS - 1);
		Term.Inst->Set(Val, Half);
	}
}

void CFindDlg::ParseVol(searchTerm &Term, CString str, bool Half)
{
	Term.Definite[WC_VOL] = true;
	if (str.IsEmpty() && !Half) {
		Term.Vol->Set(MAX_VOLUME);
		return;
	}
	RaiseIf(Half && !Term.Vol->IsSingle(), _T("Cannot use wildcards in a range search query."));

	if (str == _T(".")) {
		RaiseIf(Half, _T("Cannot use wildcards in a range search query."));
		Term.Vol->Min = 0;
		Term.Vol->Max = MAX_VOLUME - 1;
	}
	else if (!str.IsEmpty()) {
		unsigned char Val = static_cast<unsigned char>(strtol(str, NULL, 16));
		RaiseIf(Val >= MAX_VOLUME,
			_T("Channel volume \"%s\" is out of range, maximum is %X."), str, MAX_VOLUME - 1);
		Term.Vol->Set(Val, Half);
	}
}

void CFindDlg::ParseEff(searchTerm &Term, CString str, bool Half)
{
	RaiseIf(str.GetLength() == 2, _T("Effect \"%s\" is too short."), str.Left(1));

	if (str.IsEmpty()) {
		Term.Definite[WC_EFF] = true;
		Term.Definite[WC_PARAM] = true;
		Term.EffNumber[EF_NONE] = true;
		Term.EffParam->Set(0);
	}
	else if (str == _T(".")) {
		Term.Definite[WC_EFF] = true;
		for (size_t i = 1; i < EF_COUNT; i++)
			Term.EffNumber[i] = true;
	}
	else {
		char Name = str[0];
		for (size_t i = 1; i < EF_COUNT; i++) {
			if (Name == EFF_CHAR[i - 1]) {
				Term.Definite[WC_EFF] = true;
				Term.EffNumber[i] = true;
			}
		}
		RaiseIf(Term.EffNumber[EF_NONE], _T("Unknown effect \"%s\" found in search query."), str.Left(1));
	}
	if (str.GetLength() > 1) {
		Term.Definite[WC_PARAM] = true;
		Term.EffParam->Set(static_cast<unsigned char>(strtol(str.Right(2), NULL, 16)));
	}
}

void CFindDlg::GetFindTerm()
{
	CString str = _T("");
	searchTerm newTerm;

	if (IsDlgButtonChecked(IDC_CHECK_FIND_NOTE)) {
		m_cFindNoteField->GetWindowText(str);
		bool empty = str.IsEmpty();
		ParseNote(newTerm, str, false);
		m_cFindNoteField2->GetWindowText(str);
		ParseNote(newTerm, str, !empty);
		RaiseIf((newTerm.Note->Min == ECHO && newTerm.Note->Max >= NOTE_C && newTerm.Note->Max <= NOTE_B ||
			newTerm.Note->Max == ECHO && newTerm.Note->Min >= NOTE_C && newTerm.Note->Min <= NOTE_B) &&
			newTerm.Definite[WC_OCT],
			_T("Cannot use both notes and echo buffer in a range search query."));
	}
	if (IsDlgButtonChecked(IDC_CHECK_FIND_INST)) {
		m_cFindInstField->GetWindowText(str);
		bool empty = str.IsEmpty();
		ParseInst(newTerm, str, false);
		m_cFindInstField2->GetWindowText(str);
		ParseInst(newTerm, str, !empty);
	}
	if (IsDlgButtonChecked(IDC_CHECK_FIND_VOL)) {
		m_cFindVolField->GetWindowText(str);
		bool empty = str.IsEmpty();
		ParseVol(newTerm, str, false);
		m_cFindVolField2->GetWindowText(str);
		ParseVol(newTerm, str, !empty);
	}
	if (IsDlgButtonChecked(IDC_CHECK_FIND_EFF)) {
		m_cFindEffField->GetWindowText(str);
		ParseEff(newTerm, str, false);
	}

	for (int i = 0; i <= 6; i++) {
		RaiseIf(i == 6, _T("Search query is empty."));
		if (newTerm.Definite[i]) break;
	}

	m_searchTerm = std::move(newTerm);
}

void CFindDlg::GetReplaceTerm()
{
	CString str = _T("");
	searchTerm newTerm;

	if (IsDlgButtonChecked(IDC_CHECK_REPLACE_NOTE)) {
		m_cReplaceNoteField->GetWindowText(str);
		ParseNote(newTerm, str, false);
	}
	if (IsDlgButtonChecked(IDC_CHECK_REPLACE_INST)) {
		m_cReplaceInstField->GetWindowText(str);
		ParseInst(newTerm, str, false);
	}
	if (IsDlgButtonChecked(IDC_CHECK_REPLACE_VOL)) {
		m_cReplaceVolField->GetWindowText(str);
		ParseVol(newTerm, str, false);
	}
	if (IsDlgButtonChecked(IDC_CHECK_REPLACE_EFF)) {
		m_cReplaceEffField->GetWindowText(str);
		ParseEff(newTerm, str, false);
	}

	for (int i = 0; i <= 6; i++) {
		RaiseIf(i == 6, _T("Replacement query is empty."));
		if (newTerm.Definite[i]) break;
	}

	RaiseIf(newTerm.Definite[WC_NOTE] && !newTerm.Note->IsSingle() ||
			newTerm.Definite[WC_OCT] && !newTerm.Oct->IsSingle() ||
			newTerm.Definite[WC_INST] && !newTerm.Inst->IsSingle() ||
			newTerm.Definite[WC_VOL] && !newTerm.Vol->IsSingle() ||
			newTerm.Definite[WC_PARAM] && !newTerm.EffParam->IsSingle(),
			_T("Replacement query cannot contain wildcards."));
		
	if (IsDlgButtonChecked(IDC_CHECK_FIND_REMOVE)) {
		RaiseIf(newTerm.Definite[WC_NOTE] && !newTerm.Definite[WC_OCT],
				_T("Replacement query cannot contain a note with an unspecified octave if ")
				_T("the option \"Remove original data\" is enabled."));
		RaiseIf(newTerm.Definite[WC_EFF] && !newTerm.Definite[WC_PARAM],
				_T("Replacement query cannot contain an effect with an unspecified parameter if ")
				_T("the option \"Remove original data\" is enabled."));
	}

	m_replaceTerm = std::move(newTerm);
}

replaceTerm CFindDlg::toReplace(const searchTerm *x)
{
	replaceTerm Term;
	Term.Note.Note = x->Note->Min;
	Term.Note.Octave = x->Oct->Min;
	Term.Note.Instrument = x->Inst->Min;
	Term.Note.Vol = x->Vol->Min;
	Term.NoiseChan = x->NoiseChan;
	for (size_t i = 0; i < EF_COUNT; i++)
		if (x->EffNumber[i]) {
			Term.Note.EffNumber[0] = static_cast<effect_t>(i);
			break;
		}
	Term.Note.EffParam[0] = x->EffParam->Min;
	memcpy(Term.Definite, x->Definite, sizeof(bool) * 6);

	return Term;
}

bool CFindDlg::CompareFields(const stChanNote Target, bool Noise, int EffCount)
{
	int EffColumn = m_cEffectColumn->GetCurSel();
	if (EffColumn > EffCount && EffColumn != 4) EffColumn = EffCount;
	bool Negate = IsDlgButtonChecked(IDC_CHECK_FIND_NEGATE) == BST_CHECKED;
	bool EffectMatch = false;

	bool Melodic = m_searchTerm.Note->Min >= NOTE_C && m_searchTerm.Note->Min <= NOTE_B && // ||
				   m_searchTerm.Note->Max >= NOTE_C && m_searchTerm.Note->Max <= NOTE_B;

	if (m_searchTerm.Definite[WC_NOTE]) {
		if (m_searchTerm.NoiseChan) {
			if (!Noise && Melodic) return false;
			if (m_searchTerm.Note->Min < NOTE_C || m_searchTerm.Note->Min > NOTE_B ||
				m_searchTerm.Note->Max < NOTE_C || m_searchTerm.Note->Max > NOTE_B) {
				if (!m_searchTerm.Note->IsMatch(Target.Note)) return Negate;
			}
			else {
				int NoiseNote = MIDI_NOTE(Target.Octave, Target.Note) % 16;
				int Low = MIDI_NOTE(m_searchTerm.Oct->Min, m_searchTerm.Note->Min) % 16;
				int High = MIDI_NOTE(m_searchTerm.Oct->Max, m_searchTerm.Note->Max) % 16;
				if ((NoiseNote < Low && NoiseNote < High) || (NoiseNote > Low && NoiseNote > High))
					return Negate;
			}
		}
		else {
			if (Noise && Melodic) return false;
			if (Melodic) {
				int NoteValue = MIDI_NOTE(Target.Octave, Target.Note);
				int Low = MIDI_NOTE(m_searchTerm.Oct->Min, m_searchTerm.Note->Min);
				int High = MIDI_NOTE(m_searchTerm.Oct->Max, m_searchTerm.Note->Max);
				if ((NoteValue < Low && NoteValue < High) || (NoteValue > Low && NoteValue > High))
					return Negate;
			}
			else {
				if (!m_searchTerm.Note->IsMatch(Target.Note)) return Negate;
				if (m_searchTerm.Definite[WC_OCT] && !m_searchTerm.Oct->IsMatch(Target.Octave))
					return Negate;
			}
		}
	}
	if (m_searchTerm.Definite[WC_INST] && !m_searchTerm.Inst->IsMatch(Target.Instrument)) return Negate;
	if (m_searchTerm.Definite[WC_VOL] && !m_searchTerm.Vol->IsMatch(Target.Vol)) return Negate;
	int Limit = MAX_EFFECT_COLUMNS - 1;
	if (EffCount < Limit) Limit = EffCount;
	if (EffColumn < Limit) Limit = EffColumn;
	for (int i = EffColumn % MAX_EFFECT_COLUMNS; i <= Limit; i++) {
		if ((!m_searchTerm.Definite[WC_EFF] || m_searchTerm.EffNumber[Target.EffNumber[i]])
		&& (!m_searchTerm.Definite[WC_PARAM] || m_searchTerm.EffParam->IsMatch(Target.EffParam[i])))
			EffectMatch = true;
	}
	if (!EffectMatch) return Negate;

	return !Negate;
}

void CFindDlg::RaiseIf(bool Check, LPCTSTR Str, ...)
{
	if (!Check) return;
	static TCHAR buf[512];
	va_list arg;
	va_start(arg, Str);
	_vsntprintf_s(buf, sizeof(buf), _TRUNCATE, Str, arg);
	va_end(arg);
	throw new CFindException(buf);
}

bool CFindDlg::Find(bool ShowEnd)
{
	m_pDocument = static_cast<CFamiTrackerDoc*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveDocument());
	m_pView = static_cast<CFamiTrackerView*>(((CFrameWnd*)AfxGetMainWnd())->GetActiveView());
	stChanNote Target;
	unsigned int Filter = m_cSearchArea->GetCurSel();
	int Track = static_cast<CMainFrame*>(theApp.m_pMainWnd)->GetSelectedTrack();

	unsigned int BeginFrame = m_bVisible ? m_pView->GetSelectedFrame() : m_iFrame,
				 BeginRow   = m_bVisible ? m_pView->GetSelectedRow() : m_iRow,
				 BeginChan  = m_bVisible ? m_pView->GetSelectedChannel() : m_iChannel;
	bool bFirst = true, bSecond = false, bVertical = IsDlgButtonChecked(IDC_CHECK_VERTICAL_SEARCH) == BST_CHECKED;

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

	replaceTerm Replace = toReplace(&m_replaceTerm);

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
				for (int i = 0; i < sizeof(FDS_EFFECTS); i++)
					if (EFF_CHAR[Replace.Note.EffNumber[0] - 1] == EFF_CHAR[FDS_EFFECTS[i] - 1]) {
						Replace.Note.EffNumber[0] = FDS_EFFECTS[i];
						break;
					}
				break;
			case SNDCHIP_S5B:
				for (int i = 0; i < sizeof(S5B_EFFECTS); i++)
					if (EFF_CHAR[Replace.Note.EffNumber[0] - 1] == EFF_CHAR[S5B_EFFECTS[i] - 1]) {
						Replace.Note.EffNumber[0] = S5B_EFFECTS[i];
						break;
					}
				break;
			case SNDCHIP_N163:
				for (int i = 0; i < sizeof(N163_EFFECTS); i++)
					if (EFF_CHAR[Replace.Note.EffNumber[0] - 1] == EFF_CHAR[N163_EFFECTS[i] - 1]) {
						Replace.Note.EffNumber[0] = N163_EFFECTS[i];
						break;
					}
				break;
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
	try {
		GetFindTerm();
	}
	catch (CFindException *e) {
		AfxMessageBox(e->what(), MB_OK | MB_ICONSTOP);
		delete e;
		return;
	}

	m_bVisible = true;
	Find(true);
	m_pView->SetFocus();
}

void CFindDlg::OnBnClickedButtonReplace()
{
	try {
		RaiseIf(m_cEffectColumn->GetCurSel() == MAX_EFFECT_COLUMNS,
				_T("\"Any\" cannot be used as the effect column scope for replacing."));
		GetFindTerm();
		GetReplaceTerm();
	}
	catch (CFindException *e) {
		AfxMessageBox(e->what(), MB_OK | MB_ICONSTOP);
		delete e;
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
		static_cast<CMainFrame*>(AfxGetMainWnd())->ResetUndo();

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