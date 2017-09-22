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

#include "FamiTrackerTypes.h" // constants
#include <memory>
#include <string>

class CFamiTrackerDoc;

template <typename T>
constexpr char hex(T x) noexcept {
	return (x & 0x0F) + ((x & 0x0F) > 0x09 ? '7' : '0');
}

std::string MakeCommandString(effect_t Effect, unsigned char Param);		// // //

// // // Channel state information
class stChannelState {
public:
	stChannelState();

	/*!	\brief Obtains a human-readable form of a channel state object.
	\warning The output of this method is neither guaranteed nor required to match that of
	CChannelHandler::GetStateString.
	\param State A reference to the channel state object.
	\return A string representing the channel's state.
	\relates CChannelHandler
	*/
	std::string GetStateString() const;

	int ChannelIndex = -1;
	int Instrument = MAX_INSTRUMENTS;
	int Volume = MAX_VOLUME;
	int Effect[EF_COUNT];
	int Effect_LengthCounter = -1;
	int Effect_AutoFMMult = -1;
	int Echo[ECHO_BUFFER_LENGTH + 1];
};

class CSongState {
public:
	explicit CSongState(int Count = MAX_CHANNELS);

	void Retrieve(const CFamiTrackerDoc &doc, unsigned Track, unsigned Frame, unsigned Row);
	std::string GetChannelStateString(const CFamiTrackerDoc &doc, int chan) const;

	std::unique_ptr<stChannelState[]> State;
	int Tempo = -1;
	int Speed = -1;
	int GroovePos = -1; // -1: disable groove
};
