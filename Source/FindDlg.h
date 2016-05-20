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

#include "PatternNote.h"
#include "PatternEditorTypes.h"

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
	searchTerm(searchTerm &&other);
	searchTerm& operator=(searchTerm &&other);

	std::unique_ptr<CharRange> Note, Oct, Inst, Vol;
	bool EffNumber[EF_COUNT];
	std::unique_ptr<CharRange> EffParam;
	bool Definite[6];
	bool NoiseChan;
};

struct replaceTerm
{
	stChanNote Note;
	bool Definite[6];
	bool NoiseChan;
};

class CFamiTrackerDoc;
class CFamiTrackerView;
class CCompoundAction;

/*!
	\brief An extension of the pattern iterator that allows constraining the cursor position within
	a given selection.
*/
class CFindCursor : public CPatternIterator
{
public:
	/*!	\brief An enumeration representing the directions that the cursor can traverse. */
	enum class direction_t { UP, DOWN, LEFT, RIGHT };

	/*!	\brief Constructor of the find / replace cursor.
		\param pEditor Pointer to the pattern editor.
		\param Track The current song number.
		\param Pos The cursor position at which searching begins.
		\param Scope The area that the cursor operates on. */
	CFindCursor(CPatternEditor *pEditor, int Track, const CCursorPos &Pos, const CSelection &Scope);

	CFindCursor(const CFindCursor &other) = default;
	CFindCursor &operator=(const CFindCursor &other) = default;

	/*!	\brief Moves the cursor in the given direction while limiting the cursor within the scope.
		\details If the cursor was outside its scope when this method is called, it will be moved to
		an appropriate initial position.
		\param Dir The direction. */
	void Move(direction_t Dir);

	/*!	\brief Checks whether the cursor reaches its starting position, as given in the
		constructor.
		\return True if the cursor is at the starting position. */
	bool AtStart() const;

	/*!	\brief Copies a note from the current song.
		\details Similar to CPatternIterator::Get, but accepts no arguments.
		\param pNote Pointer to the output note. */
	void Get(stChanNote *pNote) const;

	/*!	\brief Writes a note to the current song.
		\details Similar to CPatternIterator::Set, but accepts no arguments.
		\param pNote Pointer to the input note. */
	void Set(const stChanNote *pNote);

	/*!	\brief Resets the cursor to an appropriate initial position if it does not lie within its
		scope.
		\param Dir The movement direction. */
	void ResetPosition(direction_t Dir);

	/*!	\brief Checks whether the cursor lies within the scope provided in the constructor.
		\details This method is similar to CPatternEditor::IsInRange but ignores the column index
		of the cursor.
		\return True if the scope contains the cursor itself. */
	bool Contains() const;

private:
	CCursorPos m_cpBeginPos;
	const CSelection m_Scope;
};

// Exception for find dialog

class CFindException : public std::runtime_error
{
public:
	CFindException(const char *msg) : std::runtime_error(msg) { }
};

// CFindResultsBox dialog

class CFindResultsBox : public CDialog
{
	DECLARE_DYNAMIC(CFindResultsBox)
public:
	CFindResultsBox(CWnd* pParent = NULL); // standard constructor
	virtual ~CFindResultsBox();
	
	virtual void DoDataExchange(CDataExchange* pDX);

	void AddResult(const stChanNote *pNote, const CFindCursor *pCursor, bool Noise) const;
	void ClearResults() const;

protected:
	CListCtrl *m_cListResults;

	enum result_column_t
	{
		ID,
		CHANNEL, PATTERN, FRAME, ROW,
		NOTE, INST, VOL,
		EFFECT,
		COUNT = EFFECT + MAX_EFFECT_COLUMNS
	};

	static result_column_t m_iLastsortColumn;
	static bool m_bLastSortDescending;

	static int CALLBACK IntCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static int CALLBACK HexCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static int CALLBACK StringCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static int CALLBACK ChannelCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	static int CALLBACK NoteCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	void SelectItem(int Index) const;
	void UpdateCount() const;

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	afx_msg void OnNMDblclkListFindresults(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnColumnClickFindResults(NMHDR *pNMHDR, LRESULT *pResult);
};

// CFindDlg dialog

class CFindDlg : public CDialog
{
	DECLARE_DYNAMIC(CFindDlg)

public:
	CFindDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CFindDlg();

	void Reset();

// Dialog Data
	enum { IDD = IDD_FIND };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	void ParseNote(searchTerm &Term, CString str, bool Half);
	void ParseInst(searchTerm &Term, CString str, bool Half);
	void ParseVol(searchTerm &Term, CString str, bool Half);
	void ParseEff(searchTerm &Term, CString str, bool Half);
	void GetFindTerm();
	void GetReplaceTerm();

	bool CompareFields(const stChanNote Target, bool Noise, int EffCount);

	void RaiseIf(bool Check, LPCTSTR Str, ...);

	replaceTerm toReplace(const searchTerm *x);

	bool PrepareFind();
	bool PrepareReplace();
	void PrepareCursor(bool ReplaceAll);

	bool Find(bool ShowEnd);
	bool Replace(CCompoundAction *pAction = nullptr);
	
	CFamiTrackerDoc *m_pDocument;
	CFamiTrackerView *m_pView;

	CEdit *m_cFindNoteField, *m_cFindInstField, *m_cFindVolField, *m_cFindEffField;
	CEdit *m_cFindNoteField2, *m_cFindInstField2, *m_cFindVolField2;
	CEdit *m_cReplaceNoteField, *m_cReplaceInstField, *m_cReplaceVolField, *m_cReplaceEffField;
	CComboBox *m_cSearchArea, *m_cEffectColumn;

	searchTerm m_searchTerm;
	replaceTerm m_replaceTerm;
	bool m_bFound, m_bSkipFirst, m_bReplacing;

	CFindCursor *m_pFindCursor;
	CFindCursor::direction_t m_iSearchDirection;

	CFindResultsBox *m_cResultsBox;

	static const CString m_pNoteName[7];
	static const CString m_pNoteSign[3];
	static const int m_iNoteOffset[7];

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void UpdateFields();
	afx_msg void OnUpdateFields(UINT nID);
	afx_msg void OnBnClickedButtonFindNext();
	afx_msg void OnBnClickedButtonFindPrevious();
	afx_msg void OnBnClickedButtonFindAll();
	afx_msg void OnBnClickedButtonReplaceNext();
	afx_msg void OnBnClickedButtonReplacePrevious();
	afx_msg void OnBnClickedButtonReplaceall();
};
