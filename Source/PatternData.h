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

#include <optional>
#include <array>
#include "FamiTrackerTypes.h"
#include "PatternNote.h"

class stChanNote;

// // // the real pattern class

class CPatternData {
	static constexpr unsigned max_size = MAX_PATTERN_LENGTH;

public:
	CPatternData() = default;
	CPatternData(const CPatternData &other) = default;
	CPatternData(CPatternData &&other) noexcept;
	CPatternData &operator=(const CPatternData &other) = default;
	CPatternData &operator=(CPatternData &&other) noexcept;

	stChanNote &GetNoteOn(unsigned row);
	const stChanNote &GetNoteOn(unsigned row) const;
	void SetNoteOn(unsigned row, const stChanNote &note);

	bool operator==(const CPatternData &other) const noexcept;
	bool operator!=(const CPatternData &other) const noexcept;
//	explicit operator bool() const noexcept;

	unsigned GetMaximumSize() const noexcept;
	unsigned GetNoteCount(int maxrows = max_size) const;
	bool IsEmpty() const;

	// void (*F)(stChanNote &note, unsigned row)
	template <typename F>
	void VisitRows(F f) {
		if (data_) {
			unsigned row = 0;
			for (auto &x : *data_)
				f(x, row++);
		}
	}
	// void (*F)(const stChanNote &note, unsigned row)
	template <typename F>
	void VisitRows(F f) const {
		if (data_) {
			unsigned row = 0;
			for (auto &x : *data_)
				f(x, row++);
		}
	}

	// void (*F)(stChanNote &note, unsigned row)
	template <typename F>
	void VisitRows(unsigned rows, F f) {
		if (data_)
			for (unsigned row = 0; row < rows; ++row)
				f((*data_)[row], row);
	}
	// void (*F)(const stChanNote &note, unsigned row)
	template <typename F>
	void VisitRows(unsigned rows, F f) const {
		if (data_)
			for (unsigned row = 0; row < rows; ++row)
				f((*data_)[row], row);
	}

private:
	void Allocate();

private:
	using elem_t = std::array<stChanNote, max_size>;
	std::optional<elem_t> data_;
};
