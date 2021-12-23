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

#include "stdafx.h"
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "CommandLineExport.h"
#include "Compiler.h"
#include "SoundGen.h"
#include "TextExporter.h"
#include "CustomExporters.h"
#include "DocumentWrapper.h"

// Command line export logger
class CCommandLineLog : public CCompilerLog
{
public:
	CCommandLineLog(std::string *pText) : m_pText(pText) {};	// // !!
	void WriteLog(LPCTSTR text) {
		*m_pText += text; // somehow send text to m_pText
	};
	void Clear() {};
private:
	std::string *m_pText;
};

// Command line export function
void CCommandLineExport::CommandLineExport(const CString& fileIn, const CString& fileOut, const CString& fileLog,  const CString& fileDPCM)
{
	// open log
	bool bLog = true;
	CStdioFile fLog;
	std::string tLog = "";		// // !!

	if (fileLog.GetLength() > 0)
	{
		fLog.Open(fileLog, CFile::modeCreate | CFile::modeWrite | CFile::typeText, NULL);
	}

	// create CFamiTrackerDoc for export
	CRuntimeClass* pRuntimeClass = RUNTIME_CLASS(CFamiTrackerDoc);
	CObject* pObject = pRuntimeClass->CreateObject();
	if (pObject == NULL || !pObject->IsKindOf(RUNTIME_CLASS(CFamiTrackerDoc)))
	{
		if (bLog) tLog += _T("Error: unable to create CFamiTrackerDoc\n");
		return;
	}
	CFamiTrackerDoc* pExportDoc = static_cast<CFamiTrackerDoc*>(pObject);

	// open file
	if(!pExportDoc->OnOpenDocument(fileIn))
	{
		if (bLog)
		{
			tLog += _T("Error: unable to open document: ");
			tLog += fileIn;
			tLog += _T("\n");
		}
		return;
	}
	if (bLog)
	{
		tLog += _T("Opened: ");
		tLog += fileIn;
		tLog += _T("\n");
	}

	// find extension
	int nPos = fileOut.ReverseFind(TCHAR('.'));
	if (nPos < 0)
	{
		if (bLog)
		{
			tLog += _T("Error: export filename has no extension: ");
			tLog += fileOut;
			tLog += _T("\n");
		}
		return;
	}
	CString ext = fileOut.Mid(nPos);

	theApp.GetSoundGenerator()->GenerateVibratoTable(pExportDoc->GetVibratoStyle());

	// export
	if (0 == ext.CompareNoCase(_T(".nsf")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&tLog) : NULL);
		compiler.ExportNSF(fileOut, pExportDoc->GetMachine() );
		if (bLog)
		{
			tLog += _T("\nNSF export complete.\n");
			CommandLineMessage(tLog);
			fLog.WriteString(tLog.c_str());
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".nes")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&tLog) : NULL);
		compiler.ExportNES(fileOut, pExportDoc->GetMachine() == PAL);
		if (bLog)
		{
			tLog += _T("\nNES export complete.\n");
			CommandLineMessage(tLog);
			fLog.WriteString(tLog.c_str());
		}
		return;
	}
	// BIN export requires two files
	else if (0 == ext.CompareNoCase(_T(".bin")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&tLog) : NULL);
		compiler.ExportBIN(fileOut, fileDPCM);
		if (bLog)
		{
			tLog += _T("\nBIN export complete.\n");
			CommandLineMessage(tLog);
			fLog.WriteString(tLog.c_str());
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".prg")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&tLog) : NULL);
		compiler.ExportPRG(fileOut, pExportDoc->GetMachine() == PAL);
		if (bLog)
		{
			tLog += _T("\nPRG export complete.\n");
			CommandLineMessage(tLog);
			fLog.WriteString(tLog.c_str());
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".asm")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&tLog) : NULL);
		compiler.ExportASM(fileOut);
		if (bLog)
		{
			tLog += _T("\nASM export complete.\n");
			CommandLineMessage(tLog);
			fLog.WriteString(tLog.c_str());
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".nsfe")))		// // //
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&tLog) : NULL);
		compiler.ExportNSFE(fileOut, pExportDoc->GetMachine());
		if (bLog)
		{
			tLog += _T("\nNSFe export complete.\n");
			CommandLineMessage(tLog);
			fLog.WriteString(tLog.c_str());
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".wav")))		// // !!
	{
		if (bLog)
		{
			tLog += _T("\nWAVE export complete.\n");
			CommandLineMessage(tLog);
			fLog.WriteString(tLog.c_str());
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".txt")))
	{
		CTextExport textExport;
		CString result = textExport.ExportFile(fileOut, pExportDoc);
		if (result.GetLength() > 0)
		{
			if (bLog)
			{
				tLog += _T("Error: ");
				tLog += result;
				tLog += _T("\n");
				CommandLineMessage(tLog);
				fLog.WriteString(tLog.c_str());
			}
		}
		else if (bLog)
		{
			tLog += _T("Exported: ");
			tLog += fileOut;
			tLog += _T("\n");
			CommandLineMessage(tLog);
			fLog.WriteString(tLog.c_str());
		}
		return;
	}
	else // use first custom exporter
	{
		CCustomExporters* pExporters = theApp.GetCustomExporters();
		if (pExporters)
		{
			CStringArray sNames;
			pExporters->GetNames(sNames);
			if (sNames.GetCount())
			{
				pExporters->SetCurrentExporter(sNames[0]);
				CFamiTrackerDocWrapper documentWrapper(CFamiTrackerDoc::GetDoc(), 0);
				bool bResult = (pExporters->GetCurrentExporter().Export(&documentWrapper, fileOut));
				if (bLog)
				{
					tLog += _T("Custom exporter: ");
					tLog += sNames[0];
					tLog += _T("\n");
					tLog += _T("Export ");
					tLog += bResult ? _T("succesful: ") : _T("failed: ");
					tLog += fileOut;
					tLog += _T("\n");
					CommandLineMessage(tLog);
					fLog.WriteString(tLog.c_str());
				}
				return;
			}
		}
	}

	if (bLog)
	{
		tLog += _T("Error: unable to find matching export extension for: ");
		tLog += fileOut;
		tLog += _T("\n");
		CommandLineMessage(tLog);
		fLog.WriteString(tLog.c_str());
	}
	return;
}
void CCommandLineExport::CommandLineMessage(std::string message) {		// // !!
	FILE* cout;
	AttachConsole(ATTACH_PARENT_PROCESS);
	errno_t err = freopen_s(&cout, "CON", "w", stdout);
	std::string cLog = _T("\n") + message + _T("\nPress enter to continue . . .");
	fprintf(stdout, "%s\n", cLog.c_str());
	fclose(cout);
}