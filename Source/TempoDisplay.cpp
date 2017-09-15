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

#include "TempoDisplay.h"
#include "TempoCounter.h"

CTempoDisplay::CTempoDisplay(const CTempoCounter &cnt, unsigned rows) :
	cnt_(&cnt),
	cache_(rows, std::make_pair(cnt_->GetTempo(), 1)),
	it_(cache_.begin())
{
}

void CTempoDisplay::Tick() {
	++current_ticks_;
}

void CTempoDisplay::StepRow() {
	it_->first = cnt_->GetTempo();		// // // 050B
	it_->second = std::exchange(current_ticks_, 0);
	if (++it_ == cache_.end())
		it_ = cache_.begin();
}

double CTempoDisplay::GetAverageBPM() const {
	double BPMtot = 0.;
	int TickTot = 0;
//	for (auto [bpm, ticks] : cache_) {
	for (const auto &x : cache_) {
		auto [bpm, ticks] = x;
		BPMtot += bpm * ticks;
		TickTot += ticks;
	}
	return BPMtot / TickTot;
}
