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

class CFamiTrackerDoc; // forward declaration
class Tokenizer;

class CTextExport : public CObject
{
public:
	CTextExport();
	virtual ~CTextExport();

	static const CString& ExportCellText(const stChanNote& stCell, unsigned int nEffects, bool bNoise);		// // //

	// returns an empty string on success, otherwise returns a descriptive error
	const CString& ImportFile(LPCTSTR FileName, CFamiTrackerDoc *pDoc);
	const CString& ExportFile(LPCTSTR FileName, CFamiTrackerDoc *pDoc);
	const CString& ExportRows(LPCTSTR FileName, CFamiTrackerDoc *pDoc);		// // //
private:		// // //
	bool ImportHex(CString& sToken, int& i, int line, int column, CString& sResult);
	CString ExportString(const CString& s);
	bool ImportCellText(CFamiTrackerDoc* pDoc, Tokenizer &t, unsigned int track, unsigned int pattern, unsigned int channel, unsigned int row, CString& sResult);
	const char* Charify(CString& s);
};
