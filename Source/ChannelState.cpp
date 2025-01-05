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

#include <memory>
#include "ChannelState.h"

stChannelState::stChannelState() :
	ChannelIndex(-1),
	Instrument(MAX_INSTRUMENTS),
	Volume(MAX_VOLUME),
	Effect_LengthCounter(-1),
	Effect_AutoFMMult(-1)
{
	memset(Effect, -1, EF_COUNT * sizeof(int));
	std::fill(std::begin(Echo), std::end(Echo), -1);
}

stFullState::stFullState(int Count) :
	State(new stChannelState[Count]()),
	Tempo(-1),
	Speed(-1),
	GroovePos(-1)
{
}
