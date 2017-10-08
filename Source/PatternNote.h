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

#include "FamiTrackerTypes.h"
#include <string>

// Channel note struct, holds the data for each row in patterns
class stChanNote {
public:
	std::string ToString() const;

	constexpr bool operator==(const stChanNote &other) const noexcept {
		for (int i = 0; i < MAX_EFFECT_COLUMNS; ++i)
			if (EffNumber[i] != other.EffNumber[i] || EffNumber[i] != EF_NONE && EffParam[i] != other.EffParam[i])
				return false;
		return Note == other.Note && Vol == other.Vol && Instrument == other.Instrument &&
			(Note == NONE || Octave == other.Octave || Note == HALT || Note == RELEASE);
	}

public:
	unsigned char Note = NONE;
	unsigned char Octave = 0U;
	unsigned char Vol = MAX_VOLUME;
	unsigned char Instrument = MAX_INSTRUMENTS;
	effect_t      EffNumber[MAX_EFFECT_COLUMNS] = {EF_NONE, EF_NONE, EF_NONE, EF_NONE};		// // //
	unsigned char EffParam[MAX_EFFECT_COLUMNS] = {0U, 0U, 0U, 0U};

public:
	static const std::string NOTE_NAME[NOTE_RANGE];
	static const std::string NOTE_NAME_FLAT[NOTE_RANGE];
};
