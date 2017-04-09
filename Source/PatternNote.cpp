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

#include <string>
#include "PatternNote.h"

using namespace std::string_literals;

const std::string stChanNote::NOTE_NAME[NOTE_RANGE] = {
	"C-"s, "C#"s, "D-"s, "D#"s, "E-"s, "F-"s,
	"F#"s, "G-"s, "G#"s, "A-"s, "A#"s, "B-"s,
};

const std::string stChanNote::NOTE_NAME_FLAT[NOTE_RANGE] = {
	"C-"s, "Db"s, "D-"s, "Eb"s, "E-"s, "F-"s,
	"Gb"s, "G-"s, "Ab"s, "A-"s, "Bb"s, "B-"s,
};

std::string stChanNote::ToString() const
{
	switch (Note) {
	case NONE:
		return "..."s;
	case HALT:
		return "---"s;
	case RELEASE:
		return "==="s;
	case ECHO:
		return "^-"s + std::to_string(Octave);
	default:
		return NOTE_NAME[Note - 1] + std::to_string(Octave);
	}
}
