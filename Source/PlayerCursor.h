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

class CFamiTrackerDoc;

// // // TODO: integrate this with CCursorPos
class CPlayerCursor {
public:
	CPlayerCursor(const CFamiTrackerDoc &doc, unsigned track);
	CPlayerCursor(const CFamiTrackerDoc &doc, unsigned track, unsigned frame, unsigned row);

	void QueueFrame(unsigned frame);
	void EnableFrameLoop();

	void Tick();
	void StepRow();
	void SetPosition(unsigned frame, unsigned row);

	void DoBxx(unsigned frame);
	void DoCxx();
	void DoDxx(unsigned row);

	unsigned GetCurrentSong() const noexcept;
	unsigned GetCurrentFrame() const noexcept;
	unsigned GetCurrentRow() const noexcept;
	unsigned GetCurrentTick() const noexcept;
	unsigned GetTotalFrames() const noexcept;
	unsigned GetTotalRows() const noexcept;
	unsigned GetTotalTicks() const noexcept;

	std::optional<unsigned> GetQueuedFrame() const noexcept;

private:
	void MoveToRow(unsigned Row);
	void MoveToFrame(unsigned frame);
	void MoveToCheckedFrame(unsigned frame);
	unsigned DequeueFrame();

	const CFamiTrackerDoc &doc_;

	unsigned track_ = 0;
	unsigned frame_ = 0;
	unsigned row_ = 0;
	unsigned tick_ = 0;
	unsigned total_frames_ = 0;
	unsigned total_rows_ = 0;
	unsigned total_ticks_ = 0;

	std::optional<unsigned> queue_;
	bool loop_ = false;
};
