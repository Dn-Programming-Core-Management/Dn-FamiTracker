/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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
	CCommandLineLog(CStdioFile *pFile) : m_pFile(pFile) {};
	void WriteLog(LPCTSTR text) {
		m_pFile->WriteString(text);
	};
	void Clear() {};
private:
	CStdioFile *m_pFile;
};

// Command line export function
void CCommandLineExport::CommandLineExport(const CString& fileIn, const CString& fileOut, const CString& fileLog,  const CString& fileDPCM)
{
	// open log
	bool bLog = false;
	CStdioFile fLog;
	if (fileLog.GetLength() > 0)
	{
		if(fLog.Open(fileLog, CFile::modeCreate | CFile::modeWrite | CFile::typeText, NULL))
			bLog = true;
	}

	// create CFamiTrackerDoc for export
	CRuntimeClass* pRuntimeClass = RUNTIME_CLASS(CFamiTrackerDoc);
	CObject* pObject = pRuntimeClass->CreateObject();
	if (pObject == NULL || !pObject->IsKindOf(RUNTIME_CLASS(CFamiTrackerDoc)))
	{
		if (bLog) fLog.WriteString(_T("Error: unable to create CFamiTrackerDoc\n"));
		return;
	}
	CFamiTrackerDoc* pExportDoc = static_cast<CFamiTrackerDoc*>(pObject);

	// open file
	if(!pExportDoc->OnOpenDocument(fileIn))
	{
		if (bLog)
		{
			fLog.WriteString(_T("Error: unable to open document: "));
			fLog.WriteString(fileIn);
			fLog.WriteString(_T("\n"));
		}
		return;
	}
	if (bLog)
	{
		fLog.WriteString(_T("Opened: "));
		fLog.WriteString(fileIn);
		fLog.WriteString(_T("\n"));
	}

	// find extension
	int nPos = fileOut.ReverseFind(TCHAR('.'));
	if (nPos < 0)
	{
		if (bLog)
		{
			fLog.WriteString(_T("Error: export filename has no extension: "));
			fLog.WriteString(fileOut);
			fLog.WriteString(_T("\n"));
		}
		return;
	}
	CString ext = fileOut.Mid(nPos);

	theApp.GetSoundGenerator()->GenerateVibratoTable(pExportDoc->GetVibratoStyle());

	// export
	if      (0 == ext.CompareNoCase(_T(".nsf")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&fLog) : NULL);
		compiler.ExportNSF(fileOut, pExportDoc->GetMachine() );
		if (bLog)
		{
			fLog.WriteString(_T("\nNSF export complete.\n"));
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".nes")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&fLog) : NULL);
		compiler.ExportNES(fileOut, pExportDoc->GetMachine() == PAL);
		if (bLog)
		{
			fLog.WriteString(_T("\nNES export complete.\n"));
		}
		return;
	}
	// BIN export requires two files
	else if (0 == ext.CompareNoCase(_T(".bin")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&fLog) : NULL);
		compiler.ExportBIN(fileOut, fileDPCM);
		if (bLog)
		{
			fLog.WriteString(_T("\nBIN export complete.\n"));
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".prg")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&fLog) : NULL);
		compiler.ExportPRG(fileOut, pExportDoc->GetMachine() == PAL);
		if (bLog)
		{
			fLog.WriteString(_T("\nPRG export complete.\n"));
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".asm")))
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&fLog) : NULL);
		compiler.ExportASM(fileOut);
		if (bLog)
		{
			fLog.WriteString(_T("\nASM export complete.\n"));
		}
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".nsfe")))		// // //
	{
		CCompiler compiler(pExportDoc, bLog ? new CCommandLineLog(&fLog) : NULL);
		compiler.ExportNSFE(fileOut, pExportDoc->GetMachine());
		if (bLog)
		{
			fLog.WriteString(_T("\nNSFe export complete.\n"));
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
				fLog.WriteString(_T("Error: "));
				fLog.WriteString(result);
				fLog.WriteString(_T("\n"));
			}
		}
		else if (bLog)
		{
			fLog.WriteString(_T("Exported: "));
			fLog.WriteString(fileOut);
			fLog.WriteString(_T("\n"));
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
					fLog.WriteString(_T("Custom exporter: "));
					fLog.WriteString(sNames[0]);
					fLog.WriteString(_T("\n"));
					fLog.WriteString(_T("Export "));
					fLog.WriteString(bResult ? _T("succesful: ") : _T("failed: "));
					fLog.WriteString(fileOut);
					fLog.WriteString(_T("\n"));
				}
				return;
			}
		}
	}

	if (bLog)
	{
		fLog.WriteString(_T("Error: unable to find matching export extension for: "));
		fLog.WriteString(fileOut);
		fLog.WriteString(_T("\n"));
	}
	return;
}
