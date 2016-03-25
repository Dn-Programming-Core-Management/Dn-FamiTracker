/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#pragma once

// std::string is required by this header file

// List control states
#define LCTRL_CHECKBOX_STATE		0x3000
#define LCTRL_CHECKBOX_CHECKED		0x2000
#define LCTRL_CHECKBOX_UNCHECKED	0x1000

class CSequence;
class CSeqInstrument;
class CFamiTrackerDoc;

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

protected:
	CFamiTrackerDoc *GetDocument() const;

	virtual void PreviewNote(unsigned char Key);
	virtual void PreviewRelease(unsigned char Key);
	virtual void OnKeyReturn();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

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

	virtual void SetSequenceString(CString Sequence, bool Changed) = 0;

	// Static methods
public:
	static int ReadStringValue(const std::string &str, bool Signed);		// // //

	// Member variables
protected:
	CSequenceEditor	*m_pSequenceEditor;
	CSequence *m_pSequence;
	CWnd *m_pParentWin;
	std::shared_ptr<CSeqInstrument> m_pInstrument;		// // //

	unsigned int m_iSelectedSetting;

protected:
	// Setup default sequence dialog
	void SetupDialog(LPCTSTR *pListItems);
	
	// Virtual methods
	virtual void TranslateMML(CString String, CSequence *pSequence, int Max, int Min) const;
	virtual void PreviewNote(unsigned char Key);
	virtual void PreviewRelease(unsigned char Key);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnRClickInstSettings(NMHDR* pNMHDR, LRESULT* pResult);

};
