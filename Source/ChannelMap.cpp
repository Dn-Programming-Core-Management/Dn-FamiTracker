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

#include "ChannelMap.h"
#include "TrackerChannel.h"

/*
 *  This class contains the expansion chip definitions & instruments.
 *
 */

void CChannelMap::ResetChannels()
{
	m_pChannels.clear();		// // //
	m_iChannelIndices.clear();
}

void CChannelMap::RegisterChannel(CTrackerChannel &Channel)		// // //
{
	// Adds a channel to the channel map
	m_iChannelIndices.try_emplace(Channel.GetID(), m_pChannels.size());		// // //
	m_pChannels.push_back(&Channel);
}

CTrackerChannel &CChannelMap::GetChannel(int Index) const		// // //
{
	return *m_pChannels[Index];
}

int CChannelMap::GetChannelIndex(int Channel) const {		// // //
	// Translate channel ID to index, returns -1 if not found
	if (auto it = m_iChannelIndices.find(Channel); it != m_iChannelIndices.cend())
		return it->second;
	return -1;
}

int CChannelMap::GetChannelCount() const {		// // //
	return m_pChannels.size();
}

int CChannelMap::GetChannelType(int Channel) const
{
	return m_pChannels[Channel]->GetID();
}

int CChannelMap::GetChipType(int Channel) const
{
	return m_pChannels[Channel]->GetChip();
}
