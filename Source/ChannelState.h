/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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
#pragma warning ( disable : 4351 )		// // // "new behaviour: elements of array [...] will be default initialized"


#include "FamiTrackerTypes.h" // constants

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
