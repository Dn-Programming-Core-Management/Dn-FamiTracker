/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
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
