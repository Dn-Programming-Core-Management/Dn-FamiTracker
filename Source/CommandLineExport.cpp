/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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
	CCommandLineLog(std::string *pText) : m_pText(pText) {};
	void WriteLog(LPCTSTR text)
	{
		*m_pText += text;
	};
	void Clear() {};
private:
	std::string *m_pText;
};

// Command line export function
void CCommandLineExport::CommandLineExport(const CString& fileIn, const CString& fileOut, const CString& fileLog,  const CString& fileDPCM)
{
	// open log
	bool bLog = false;
	CStdioFile LogFile;
	std::string LogText = "";

	if (fileLog.GetLength() > 0)
		bLog = (LogFile.Open(fileLog, CFile::modeCreate | CFile::modeWrite | CFile::typeText, NULL));
	
	// create CFamiTrackerDoc for export
	CRuntimeClass* pRuntimeClass = RUNTIME_CLASS(CFamiTrackerDoc);
	CObject* pObject = pRuntimeClass->CreateObject();
	if (pObject == NULL || !pObject->IsKindOf(RUNTIME_CLASS(CFamiTrackerDoc)))
	{
		LogText += "Error: unable to create CFamiTrackerDoc\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}
	CFamiTrackerDoc* pExportDoc = static_cast<CFamiTrackerDoc*>(pObject);

	// open file
	if(!pExportDoc->OnOpenDocument(fileIn)) {
		LogText += "Error: unable to open document: ";
		LogText += fileIn;
		LogText += "\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

		LogText += "Opened: ";
		LogText += fileIn;
		LogText += "\n";

	// find extension
	int nPos = fileOut.ReverseFind(TCHAR('.'));
	if (nPos < 0)
	{
		LogText += "Error: export filename has no extension: ";
		LogText += fileOut;
		LogText += "\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}
	CString ext = fileOut.Mid(nPos);

	theApp.GetSoundGenerator()->GenerateVibratoTable(pExportDoc->GetVibratoStyle());

	// export
	CCompiler compiler(pExportDoc, new CCommandLineLog(&LogText));
	if (0 == ext.CompareNoCase(_T(".nsf"))) {
		compiler.ExportNSF(fileOut, pExportDoc->GetMachine());
		LogText += "\nNSF export complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

	else if (0 == ext.CompareNoCase(_T(".nsfe"))) {
		compiler.ExportNSFE(fileOut, pExportDoc->GetMachine());
		LogText += "\nNSFe export complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

	else if (0 == ext.CompareNoCase(_T(".nsf2"))) {
		CString actualFileOut = fileOut;
		actualFileOut.Delete(nPos, ext.GetLength());
		actualFileOut += ".nsf";
		compiler.ExportNSF2(actualFileOut, pExportDoc->GetMachine());
		LogText += "\nNSF2 export complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

	else if (0 == ext.CompareNoCase(_T(".nes"))) {
		compiler.ExportNES(fileOut, pExportDoc->GetMachine() == PAL);
		LogText += "\nNES export complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}
	// BIN export requires two files
	else if (0 == ext.CompareNoCase(_T(".bin"))) {
		compiler.ExportBIN(fileOut, fileDPCM, pExportDoc->GetMachine(), false);
		LogText += "\nBIN export complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

	else if (0 == ext.CompareNoCase(_T(".bin_aux"))) {
		CString actualFileOut = fileOut;
		actualFileOut.Delete(nPos, ext.GetLength());
		actualFileOut += ".bin";
		compiler.ExportBIN(actualFileOut, fileDPCM, pExportDoc->GetMachine(), true);
		LogText += "\nBIN export with auxiliary data complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

	else if (0 == ext.CompareNoCase(_T(".prg"))) {
		compiler.ExportPRG(fileOut, pExportDoc->GetMachine() == PAL);
		LogText += "\nPRG export complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

	else if (0 == ext.CompareNoCase(_T(".asm"))) {
		compiler.ExportASM(fileOut, pExportDoc->GetMachine(), false);
		LogText += "\nASM export complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

	else if (0 == ext.CompareNoCase(_T(".asm_aux"))) {
		CString actualFileOut = fileOut;
		actualFileOut.Delete(nPos, ext.GetLength());
		actualFileOut += ".asm";
		compiler.ExportASM(actualFileOut, pExportDoc->GetMachine(), true);
		LogText += "\nASM export with auxiliary data complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}
	else if (0 == ext.CompareNoCase(_T(".wav")))		// // !!
	{
		LogText += "\nWAVE export complete.\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

	else if (0 == ext.CompareNoCase(_T(".txt"))) {
		CTextExport textExport;
		CString result = textExport.ExportFile(fileOut, pExportDoc);
		if (result.GetLength() > 0) {
			LogText += "Error: ";
			LogText += result;
			LogText += "\n";
			PrintCommandlineMessage(LogFile, LogText, bLog);
		}

		LogText += "Exported: ";
		LogText += fileOut;
		LogText += "\n";
		LogText += "Press enter to continue . . .";
		PrintCommandlineMessage(LogFile, LogText, bLog);
		return;
	}

	else {		// use first custom exporter
		CCustomExporters* pExporters = theApp.GetCustomExporters();
		if (pExporters) {
			CStringArray sNames;
			pExporters->GetNames(sNames);
			if (sNames.GetCount())
			{
				pExporters->SetCurrentExporter(sNames[0]);
				CFamiTrackerDocWrapper documentWrapper(CFamiTrackerDoc::GetDoc(), 0);
				bool bResult = (pExporters->GetCurrentExporter().Export(&documentWrapper, fileOut));
				LogText += "Custom exporter: ";
				LogText += sNames[0];
				LogText += "\n";
				LogText += "Export ";
				LogText += bResult ? "succesful: " : "failed: ";
				LogText += fileOut;
				LogText += "\n";
				LogText += "Press enter to continue . . .";
				PrintCommandlineMessage(LogFile, LogText, bLog);
				return;
			}
		}
	}

	LogText += "Error: unable to find matching export extension for: ";
	LogText += fileOut;
	LogText += "\n";
	LogText += "Press enter to continue . . .";
	PrintCommandlineMessage(LogFile, LogText, bLog);

	return;
}

void CCommandLineExport::PrintCommandlineMessage(CStdioFile &LogFile, std::string &text, bool writelog)
{
	if (writelog)
		LogFile.WriteString(text.c_str());
	fprintf(stdout, "%s\n", text.c_str());
}
