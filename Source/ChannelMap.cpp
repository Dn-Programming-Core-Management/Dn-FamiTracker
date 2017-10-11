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
#include "APU/Types.h"
#include "TrackerChannel.h"

/*
 *  This class contains the expansion chip definitions & instruments.
 *
 */

int CChannelMap::GetChannelCount() const {		// // //
	return m_iRegisteredChannels;
}

int CChannelMap::GetChannelType(int Channel) const
{
	// Return channel type form channel index
	ASSERT(m_iRegisteredChannels != 0);
	ASSERT(Channel < m_iRegisteredChannels);		// // //
	return m_iChannelTypes[Channel];
}

int CChannelMap::GetChipType(int Channel) const
{
	// Return chip type from channel index
	ASSERT(m_iRegisteredChannels != 0);
	ASSERT(Channel < m_iRegisteredChannels);
	return m_pChannels[Channel]->GetChip();
}

void CChannelMap::ResetChannels()
{
	// Clears all channels from the channel map
	m_iRegisteredChannels = 0;
}

void CChannelMap::RegisterChannel(CTrackerChannel *pChannel, int ChannelType, int ChipType)
{
	// Adds a channel to the channel map
	m_pChannels[m_iRegisteredChannels] = pChannel;
	m_iChannelTypes[m_iRegisteredChannels] = ChannelType;
	m_iChannelChip[m_iRegisteredChannels] = ChipType;
	++m_iRegisteredChannels;
}

CTrackerChannel *CChannelMap::GetChannel(int Index) const
{
	// Return channel from index
	ASSERT(m_iRegisteredChannels != 0);
	ASSERT(m_pChannels[Index] != NULL);
	return m_pChannels[Index];
}

int CChannelMap::GetChannelIndex(int Channel) const {		// // //
	// Translate channel ID to index, returns -1 if not found
	for (int i = 0; i < m_iRegisteredChannels; ++i) {
		if (m_pChannels[i]->GetID() == Channel)
			return i;
	}
	return -1;
}
