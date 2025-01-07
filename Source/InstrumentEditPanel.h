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

#include <string>		// // //
#include <memory>		// // //

// List control states
#define LCTRL_CHECKBOX_STATE		0x3000
#define LCTRL_CHECKBOX_CHECKED		0x2000
#define LCTRL_CHECKBOX_UNCHECKED	0x1000

class CSequence;
class CSeqInstrument;
class CFamiTrackerDoc;
class CInstrumentManager;		// // //
class CSequenceParser;		// // //

class CInstrumentEditPanel : public CDialog
{
	DECLARE_DYNAMIC(CInstrumentEditPanel)
public:
	CInstrumentEditPanel(UINT nIDTemplate, CWnd* pParent = NULL);   // standard constructor
	virtual ~CInstrumentEditPanel();
	virtual int GetIDD() const = 0;
	virtual TCHAR *GetTitle() const = 0;

	// Select instrument for the editing
	virtual void SelectInstrument(std::shared_ptr<CInstrument> pInst) = 0;		// // //
	void SetInstrumentManager(CInstrumentManager *pManager);		// // //

protected:
	CFamiTrackerDoc *GetDocument() const;

	virtual void PreviewNote(unsigned char Key);
	virtual void PreviewRelease(unsigned char Key);
	virtual void OnKeyReturn();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

protected:
	CInstrumentManager *m_pInstManager;		// // //

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
};

class CSequenceEditor;

// Adds some functions for sequences
class CSequenceInstrumentEditPanel : public CInstrumentEditPanel 
{
	DECLARE_DYNAMIC(CSequenceInstrumentEditPanel)
public:
	CSequenceInstrumentEditPanel(UINT nIDTemplate, CWnd* pParent);
	virtual ~CSequenceInstrumentEditPanel();

	virtual void UpdateSequenceString(bool Changed) = 0;		// // //

	// Static methods
public:
	static int ReadStringValue(const std::string &str, bool Signed);		// // //

	// Member variables
protected:
	CSequenceEditor	*m_pSequenceEditor;
	CSequence *m_pSequence;
	CWnd *m_pParentWin;
	std::shared_ptr<CSeqInstrument> m_pInstrument;		// // //
	CSequenceParser *m_pParser;		// // //

	unsigned int m_iSelectedSetting;

protected:
	// Setup default sequence dialog
	void SetupDialog(LPCTSTR *pListItems);
	
	// Virtual methods
	virtual void SetupParser() const = 0;		// // //
	virtual void TranslateMML(CString String) const;		// // //
	virtual void PreviewNote(unsigned char Key);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRClickInstSettings(NMHDR* pNMHDR, LRESULT* pResult);

};
