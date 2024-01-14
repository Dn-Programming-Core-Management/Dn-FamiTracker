/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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
