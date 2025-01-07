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

#include "FamiTrackerTypes.h"

// Channel note struct, holds the data for each row in patterns
class stChanNote {
public:
	CString ToString() const;

public:
	unsigned char Note = NONE;
	unsigned char Octave = 0U;
	unsigned char Vol = MAX_VOLUME;
	unsigned char Instrument = MAX_INSTRUMENTS;
	effect_t      EffNumber[MAX_EFFECT_COLUMNS] = {EF_NONE, EF_NONE, EF_NONE, EF_NONE};		// // //
	unsigned char EffParam[MAX_EFFECT_COLUMNS] = {0U, 0U, 0U, 0U};

public:
	static const CString NOTE_NAME[NOTE_RANGE];
	static const CString NOTE_NAME_FLAT[NOTE_RANGE];

	bool operator==(const stChanNote& other) const {
		if (Note != other.Note
			|| Octave != other.Octave
			|| Vol != other.Vol
			|| Instrument != other.Instrument)
		{
			return false;
		}
		for (int i = 0; i < MAX_EFFECT_COLUMNS; ++i)
		{
			if (EffNumber[i] != other.EffNumber[i] || EffParam[i] != other.EffParam[i])
			{
				return false;
			}
		}

		return true;
	}

	bool operator!=(const stChanNote& other) const {
		return !(*this == other);
	}
};
