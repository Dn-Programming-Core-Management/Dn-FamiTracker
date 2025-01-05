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

#include "stdafx.h"

#include "FamiTracker.h"
//#include "FamiTrackerDoc.h"
#include "Settings.h"

class HistoryFileDlg : public CFileDialog {
private:
	PATHS historyType;

public:
	HistoryFileDlg(PATHS historyType,
		BOOL openNotSave, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR defaultExt = NULL,
		LPCTSTR defaultName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		DWORD dwSize = 0,
		BOOL bVistaStyle = TRUE);
	virtual INT_PTR DoModal() override;
};




// void fileDialog(CFamiTrackerApp theApp, CString & fileName, UINT idTitle, DWORD flags, BOOL openNotSave, PATHS pathType);
