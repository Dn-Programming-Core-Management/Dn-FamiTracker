/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#include <vector>
#include "../stdafx.h"
#include "../FamiTracker.h"
#include "../FamiTrackerDoc.h"
#include "../Compiler.h"
#include "../SoundGen.h"
#include "ExportTest.h"

/*
 * This class is used for export verification, it works by comparing the internal
 * register writes to the emulated NSF.
 *
 * It is not a part of the release build so there's no need for string table support.
 *
 */

#ifdef EXPORT_TEST

CExportTest::CExportTest() : m_pHeader(new stNSFHeader), m_hModule(NULL), m_bErrors(false)
{
}

CExportTest::~CExportTest()
{
	SAFE_RELEASE(m_pHeader);
	if (m_hModule != NULL) {
		::FreeLibrary(m_hModule);
	}
}

bool CExportTest::Setup(LPCTSTR lpszFile)
{
	if (m_hModule == NULL)
		m_hModule = ::LoadLibrary(_T("ExportTest.dll"));

	if (m_hModule == NULL) {
		AfxMessageBox(_T("Could not load ExportTest.dll"));
		return false;
	}

	ImportFuncs.LoadFileFunc = (LoadFile_t)GetProcAddress(m_hModule, "LoadFile");
	ImportFuncs.RunFrameFunc = (RunFrame_t)GetProcAddress(m_hModule, "RunFrame");
	ImportFuncs.ReadResultFunc = (ReadResult_t)GetProcAddress(m_hModule, "ReadResult");

	if (ImportFuncs.LoadFileFunc == NULL)
		return false;

	if (ImportFuncs.RunFrameFunc == NULL)
		return false;

	if (ImportFuncs.ReadResultFunc == NULL)
		return false;

	TCHAR TempPath[MAX_PATH];
	TCHAR TempFile[MAX_PATH];

	if (lpszFile == NULL || strlen(lpszFile) == 0) {
		// Get a temporary filename
		GetTempPath(MAX_PATH, TempPath);
		GetTempFileName(TempPath, _T("NSF-TEST"), 0, TempFile);
		TRACE("ExportTest: Creating file %s\n", TempFile);
		// Export file
		CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();
		CCompiler Compiler(pDoc, NULL);
		Compiler.ExportNSF(CString(TempFile), MACHINE_NTSC);
	}
	else {
		_tcscpy_s(TempFile, MAX_PATH, lpszFile);
	}

	CFile inFile(TempFile, CFile::modeRead);

	int size = (int)inFile.GetLength() - sizeof(stNSFHeader);

	char *pMemory = new char[size];

	inFile.Read(m_pHeader, sizeof(stNSFHeader));
	inFile.Read(pMemory, size);
	inFile.Close();

	if (lpszFile == NULL) {
		// Delete temporary file
		DeleteFile(TempFile);
	}

	m_iFileSize = size;

	// Setup memory
	ImportFuncs.LoadFileFunc(pMemory, size, m_pHeader);

	SAFE_RELEASE_ARRAY(pMemory);

	return true;
}

void CExportTest::RunInit(int Song)
{
	int cycles = ImportFuncs.RunFrameFunc(m_pHeader->InitAddr, Song);
}

void CExportTest::RunPlay()
{
	int cycles = ImportFuncs.RunFrameFunc(m_pHeader->PlayAddr, 0);
}

unsigned char CExportTest::ReadReg(int Reg, int Chip)
{
	return ImportFuncs.ReadResultFunc(Reg, Chip);
}

class CExportTestDlg : public CDHtmlDialog
{
public:
	CExportTestDlg(UINT nIDTemplate, UINT nHtmlResID, CWnd *pParentWnd) : CDHtmlDialog(nIDTemplate, nHtmlResID, pParentWnd) {}
	void OnDocumentComplete(LPDISPATCH pDisp, LPCTSTR szUrl) {
		CString str;

		str = _T("<b>Internal:</b><br>");
		for (int i = 0; i < 0x14; ++i) {
			if ((i & 3) == 0)
				str.AppendFormat(_T("<tt>$%X: </tt>"), i + 0x4000);
			
			str.Append(_T("<tt style=\"color:green\">"));
			str.AppendFormat(_T("$%02X "), m_pInternalRegs->R_2A03[i]);
			str.Append(_T("</tt>"));

			str.Append((i & 3) == 3 ? _T("<br>") : _T(""));
		}
		if (m_iChip & SNDCHIP_VRC6) {
			str.Append(_T("<br>"));
			for (int i = 0; i < 9; ++i) {
				if ((i % 3) == 0)
					str.AppendFormat(_T("<tt>$%X: </tt>"), (i / 3) * 0x1000 + 0x9000);
				str.Append(_T("<tt style=\"color:green\">"));
				str.AppendFormat(_T("$%02X "), m_pInternalRegs->R_VRC6[i]);
				str.Append(_T("</tt>"));

				str.Append((i % 3) == 2 ? _T("<br>") : _T(""));
			}
		}

		SetElementHtml(_T("resultInternal"), str.AllocSysString());

		str = _T("<b>Exported:</b><br>");
		for (int i = 0; i < 0x14; ++i) {
			if ((i & 3) == 0)
				str.AppendFormat(_T("<tt>$%X: </tt>"), i + 0x4000);

			str.AppendFormat(_T("<tt style=\"color:%s\">"), (m_pExternalRegs->R_2A03[i] == m_pInternalRegs->R_2A03[i]) ? _T("green") : _T("red"));
			str.AppendFormat("$%02X ", m_pExternalRegs->R_2A03[i]);
			str.Append(_T("</tt>"));

			str.AppendFormat((i & 3) == 3 ? _T("<br>") : _T(""));
		}
		if (m_iChip & SNDCHIP_VRC6) {
			str.Append(_T("<br>"));
			for (int i = 0; i < 9; ++i) {
				if ((i % 3) == 0)
					str.AppendFormat(_T("<tt>$%X: </tt>"), (i / 3) * 0x1000 + 0x9000);

				str.AppendFormat(_T("<tt style=\"color:%s\">"), (m_pExternalRegs->R_VRC6[i] == m_pInternalRegs->R_VRC6[i]) ? _T("green") : _T("red"));
				str.AppendFormat(_T("$%02X "), m_pExternalRegs->R_VRC6[i]);
				str.Append(_T("</tt>"));

				str.Append((i % 3) == 2 ? _T("<br>") : _T(""));
			}
		}

		SetElementHtml(_T("resultExternal"), str.AllocSysString());

		str.Format(_T("APU frames since last row: %i<br><br>"), m_iUpdateFrames);
		str.AppendFormat(_T("File size: %i kB"), m_iFileSize / 1024);
		SetElementHtml(_T("other"), str.AllocSysString());
	}

	BOOL OnInitDialog() {
		SetHostFlags(DOCHOSTUIFLAG_NO3DBORDER);
		CDHtmlDialog::OnInitDialog();
		return TRUE;
	}

	unsigned int m_iFileSize;
	unsigned int m_iUpdateFrames;

	unsigned char m_iChip;

	stRegs *m_pInternalRegs;
	stRegs *m_pExternalRegs;

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedContinue() {
		EndDialog(1);
	}

	afx_msg void OnBnClickedAbort() {
		EndDialog(0);
	}
};

BEGIN_MESSAGE_MAP(CExportTestDlg, CDHtmlDialog)
	ON_BN_CLICKED(IDC_CONTINUE, OnBnClickedContinue)
	ON_BN_CLICKED(IDC_ABORT, OnBnClickedAbort)
END_MESSAGE_MAP()

void CExportTest::ReportSuccess()
{
	if (theApp.IsExportTest()) {
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN);
		printf("\nExport verification completed without errors\n");
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
		ExitProcess(0);
	}
	else {
		if (m_bErrors)
			theApp.ThreadDisplayMessage(_T("Export verification completed with errors"));
		else
			theApp.ThreadDisplayMessage(_T("Export verification completed without errors"));
	}
}

bool CExportTest::ReportError(stRegs *pInternalRegs, stRegs *pExternalRegs, int UpdateFrames, int Chip)
{
	m_bErrors = true;

	if (theApp.IsExportTest()) {
		PrintErrorReport(pInternalRegs, pExternalRegs, UpdateFrames, Chip);
		ExitProcess(1);
	}
	else {
		CExportTestDlg dlg(IDD_EXPORT_TEST, IDR_EXPORT_TEST_REPORT, theApp.m_pMainWnd);
		
		dlg.m_pInternalRegs = pInternalRegs;
		dlg.m_pExternalRegs = pExternalRegs;
		dlg.m_iFileSize = m_iFileSize;
		dlg.m_iUpdateFrames = UpdateFrames;
		dlg.m_iChip = Chip;

		return dlg.DoModal() == 0;
	}

	return true;
}

const WORD CON_COLOR_RED	= FOREGROUND_RED;
const WORD CON_COLOR_GREEN	= FOREGROUND_GREEN;
const WORD CON_COLOR_WHITE	= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

static void SetConsoleColor(WORD Color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), Color);
}

void CExportTest::PrintErrorReport(stRegs *pInternalRegs, stRegs *pExternalRegs, int UpdateFrames, int Chip)
{
	// Print error report to the console
	SetConsoleColor(CON_COLOR_RED);
	printf("\nExport verification failed\n\n");

	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	SetConsoleColor(CON_COLOR_WHITE);
	printf("Internal:\n");

	for (int i = 0; i < 0x14; ++i) {
		if ((i & 3) == 0) {
			SetConsoleColor(CON_COLOR_WHITE);
			printf("$%X: ", i + 0x4000);
		}
		
		SetConsoleColor(CON_COLOR_GREEN);
		printf("$%02X ", pInternalRegs->R_2A03[i]);

		if ((i & 3) == 3)
			printf("\n");
	}

	SetConsoleColor(CON_COLOR_WHITE);
	printf("\nExternal:\n");

	for (int i = 0; i < 0x14; ++i) {
		if ((i & 3) == 0) {
			SetConsoleColor(CON_COLOR_WHITE);
			printf("$%X: ", i + 0x4000);
		}
		
		if (pInternalRegs->R_2A03[i] == pExternalRegs->R_2A03[i])
			SetConsoleColor(CON_COLOR_GREEN);
		else
			SetConsoleColor(CON_COLOR_RED);

		printf("$%02X ", pExternalRegs->R_2A03[i]);

		if ((i & 3) == 3)
			printf("\n");
	}

	SetConsoleColor(CON_COLOR_WHITE);
	printf("\nOn frame: %i, row: %i (%i update frames)\n", pSoundGen->GetPlayerFrame(), pSoundGen->GetPlayerRow(), UpdateFrames);
}

#endif /* EXPORT_TEST */
