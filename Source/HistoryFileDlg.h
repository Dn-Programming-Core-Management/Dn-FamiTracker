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
