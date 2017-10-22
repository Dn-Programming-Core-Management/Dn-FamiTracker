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


#pragma once

#include <string>

class CFamiTrackerDoc;
class CDocumentFile;

enum module_error_level_t; // Settings.h

class CFamiTrackerDocIO {
public:
	explicit CFamiTrackerDocIO(CDocumentFile &file);

	void Load(CFamiTrackerDoc &doc);
	bool Save(const CFamiTrackerDoc &doc);

//private:
	void SaveParams(const CFamiTrackerDoc &doc, int ver);

	void LoadSongInfo(CFamiTrackerDoc &doc, int ver);
	void SaveSongInfo(const CFamiTrackerDoc &doc, int ver);

	void LoadHeader(CFamiTrackerDoc &doc, int ver);
	void SaveHeader(const CFamiTrackerDoc &doc, int ver);

	void LoadInstruments(CFamiTrackerDoc &doc, int ver);
	void SaveInstruments(const CFamiTrackerDoc &doc, int ver);

	void SaveSequences(const CFamiTrackerDoc &doc, int ver);

	void LoadFrames(CFamiTrackerDoc &doc, int ver);
	void SaveFrames(const CFamiTrackerDoc &doc, int ver);

	void SavePatterns(const CFamiTrackerDoc &doc, int ver);

	void LoadDSamples(CFamiTrackerDoc &doc, int ver);
	void SaveDSamples(const CFamiTrackerDoc &doc, int ver);

	void LoadComments(CFamiTrackerDoc &doc, int ver);
	void SaveComments(const CFamiTrackerDoc &doc, int ver);

	void LoadSequencesVRC6(CFamiTrackerDoc &doc, int ver);
	void SaveSequencesVRC6(const CFamiTrackerDoc &doc, int ver);

	void LoadSequencesN163(CFamiTrackerDoc &doc, int ver);
	void SaveSequencesN163(const CFamiTrackerDoc &doc, int ver);

	void LoadSequencesS5B(CFamiTrackerDoc &doc, int ver);
	void SaveSequencesS5B(const CFamiTrackerDoc &doc, int ver);

	void LoadParamsExtra(CFamiTrackerDoc &doc, int ver);
	void SaveParamsExtra(const CFamiTrackerDoc &doc, int ver);

	void LoadDetuneTables(CFamiTrackerDoc &doc, int ver);
	void SaveDetuneTables(const CFamiTrackerDoc &doc, int ver);

	void LoadGrooves(CFamiTrackerDoc &doc, int ver);
	void SaveGrooves(const CFamiTrackerDoc &doc, int ver);

	void LoadBookmarks(CFamiTrackerDoc &doc, int ver);
	void SaveBookmarks(const CFamiTrackerDoc &doc, int ver);

private:
	template <module_error_level_t l = MODULE_ERROR_DEFAULT>
	void AssertFileData(bool Cond, const std::string &Msg) const;		// // //

	template <module_error_level_t l = MODULE_ERROR_DEFAULT, typename T, typename U, typename V>
	T AssertRange(T Value, U Min, V Max, const std::string &Desc) const;

	CDocumentFile &file_;
};
