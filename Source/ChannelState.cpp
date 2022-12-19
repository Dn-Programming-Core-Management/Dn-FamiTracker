/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Library General Public License for more details. To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
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
