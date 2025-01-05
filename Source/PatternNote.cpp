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

#include "stdafx.h"
#include "PatternNote.h"

const CString stChanNote::NOTE_NAME[NOTE_RANGE] = {
	_T("C-"), _T("C#"), _T("D-"), _T("D#"),
	_T("E-"), _T("F-"), _T("F#"), _T("G-"),
	_T("G#"), _T("A-"), _T("A#"), _T("B-"),
};

const CString stChanNote::NOTE_NAME_FLAT[NOTE_RANGE] = {
	_T("C-"), _T("Db"), _T("D-"), _T("Eb"),
	_T("E-"), _T("F-"), _T("Gb"), _T("G-"),
	_T("Ab"), _T("A-"), _T("Bb"), _T("B-"),
};

CString stChanNote::ToString() const
{
	switch (Note) {
	case NONE: return _T("...");
	case HALT: return _T("---");
	case RELEASE: return _T("===");
	case ECHO:
		{
			CString str;
			str.Format(_T("^-%d"), Octave);
			return str;
		}
	default:
		{
			CString str;
			str.Format(_T("%s%d"), NOTE_NAME[Note - 1], Octave);
			return str;
		}
	}
}
