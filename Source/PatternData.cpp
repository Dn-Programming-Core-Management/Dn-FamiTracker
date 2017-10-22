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

#include "PatternData.h"

namespace {

const auto BLANK = stChanNote { };

} // namespace

CPatternData::CPatternData(CPatternData &&other) noexcept : data_(std::move(other.data_)) {
	other.data_.reset();
}

CPatternData &CPatternData::operator=(CPatternData &&other) noexcept {
	data_.swap(other.data_);
	return *this;
}

stChanNote &CPatternData::GetNoteOn(unsigned row) {
	Allocate();
	return (*data_)[row];
}

const stChanNote &CPatternData::GetNoteOn(unsigned row) const {
	return data_ ? (*data_)[row] : BLANK;
}

void CPatternData::SetNoteOn(unsigned row, const stChanNote &note) {
	Allocate();
	(*data_)[row] = note;
}

bool CPatternData::operator==(const CPatternData &other) const noexcept {
	return (!data_ && !other.data_) || (data_ && other.data_ && *data_ == *other.data_);
}

bool CPatternData::operator!=(const CPatternData &other) const noexcept {
	return !operator==(other);
}
/*
CPatternData::operator bool() const noexcept {
	return data_.has_value();
}
*/

unsigned CPatternData::GetMaximumSize() const noexcept {
	return std::tuple_size_v<elem_t>;
}

unsigned CPatternData::GetNoteCount(int maxrows) const {
	unsigned count = 0;
	VisitRows(maxrows, [&] (const stChanNote &note, unsigned row) {
		if (note != BLANK)
			++count;
	});
	return count;
}

bool CPatternData::IsEmpty() const {
	if (!data_)
		return true;
	for (const auto &x : *data_)
		if (x != BLANK)
			return false;
	return true;
}

void CPatternData::Allocate() {
	if (!data_)
		data_.emplace();
}
