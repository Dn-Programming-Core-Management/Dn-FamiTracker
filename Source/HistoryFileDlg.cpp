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
