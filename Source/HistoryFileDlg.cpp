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
