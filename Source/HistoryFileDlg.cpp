/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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

#include "HistoryFileDlg.h"


HistoryFileDlg::HistoryFileDlg(PATHS historyType,
		BOOL openNotSave, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR defaultExt,
		LPCTSTR defaultName,
		DWORD dwFlags,
		LPCTSTR lpszFilter,
		CWnd* pParentWnd,
		DWORD dwSize,
		BOOL bVistaStyle) :
		CFileDialog(openNotSave, defaultExt, defaultName, dwFlags, lpszFilter,
			pParentWnd, dwSize, bVistaStyle)
	{
		this->historyType = historyType;
		this->m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(historyType);
	}


INT_PTR HistoryFileDlg::DoModal() {
	// IDOK and IDCANCEL
	INT_PTR ret = CFileDialog::DoModal();
	if (ret == IDOK) {
		theApp.GetSettings()->SetPath(GetPathName(), historyType);
	}
	return ret;
}



//bool fileDialog(CString &fileName, CFamiTrackerApp theApp,
//		PATHS pathType, BOOL openNotSave,
//		UINT idTitle, LPCTSTR defaultExt, LPCTSTR defaultName, LPCTSTR filter) {
//
//	DWORD flags = OFN_HIDEREADONLY;
//	if (openNotSave) {
//		flags |= OFN_FILEMUSTEXIST;
//	}
//	else {
//		flags |= OFN_PATHMUSTEXIST;
//	}
//
//	CFileDialog dialog(openNotSave, defaultExt, defaultName, flags, filter,
//		theApp.GetMainWnd());
//	OPENFILENAME& ofn = dialog.GetOFN();
//	ofn.lpstrInitialDir = theApp.GetSettings()->GetPath(pathType) + _T("\\");
//	ofn.Flags |= flags;
//
//
//	if (OpenFileDlg.DoModal() == IDCANCEL)
//		return false;
//
//	return true;
//}
