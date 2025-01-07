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

#include "FamiTrackerTypes.h"

class CFamiTrackerDoc;

// CModuleImportDlg dialog

class CModuleImportDlg : public CDialog
{
	DECLARE_DYNAMIC(CModuleImportDlg)

public:
	CModuleImportDlg(CFamiTrackerDoc *pDoc);
	virtual ~CModuleImportDlg();

// Dialog Data
	enum { IDD = IDD_IMPORT };

public:
	bool LoadFile(CString Path, CFamiTrackerDoc *pDoc);

private:
	CFamiTrackerDoc *m_pDocument;
	CFamiTrackerDoc *m_pImportedDoc;

	int m_iInstrumentTable[MAX_INSTRUMENTS];
	int m_iGrooveMap[MAX_GROOVE];		 // // //

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	bool ImportInstruments();
	bool ImportGrooves();		// // //
	bool ImportDetune();		// // //
	bool ImportTracks();

protected:
	CCheckListBox m_ctlTrackList;

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedCancel();
};
