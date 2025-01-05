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

#include "FamiTrackerTypes.h" // constants
#include <memory>

// // // Channel state information
class stChannelState {
public:
	stChannelState();

	int ChannelIndex;
	int Instrument;
	int Volume;
	int Effect[EF_COUNT];
	int Effect_LengthCounter;
	int Effect_AutoFMMult;
	int Echo[ECHO_BUFFER_LENGTH + 1];
};

class stFullState {
public:
	stFullState(int Count = MAX_CHANNELS);
	stFullState(const stFullState &other) = delete;
	stFullState& operator=(const stFullState &other) = delete;

	std::unique_ptr<stChannelState[]> State;
	int Tempo;
	int Speed;
	int GroovePos; // -1: disable groove
};
