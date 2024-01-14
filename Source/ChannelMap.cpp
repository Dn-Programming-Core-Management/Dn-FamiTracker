/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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

#include <vector>		// // //
#include <memory>		// // //
#include "stdafx.h"
#include "APU/Types.h"
#include "InstrumentFactory.h"		// // //
#include "TrackerChannel.h"
#include "ChannelMap.h"

/*
 *  This class contains the expansion chip definitions & instruments.
 *
 */

CChannelMap::CChannelMap() :
	m_iAddedChips(0)
{
	SetupSoundChips();
}

CChannelMap::~CChannelMap()
{
}

void CChannelMap::SetupSoundChips()
{
	// Add available chips
	AddChip(SNDCHIP_NONE, INST_2A03, _T("NES channels only"));
	AddChip(SNDCHIP_VRC6, INST_VRC6, _T("Konami VRC6"));
	AddChip(SNDCHIP_VRC7, INST_VRC7, _T("Konami VRC7"));
	AddChip(SNDCHIP_FDS,  INST_FDS,  _T("Nintendo FDS sound"));
	AddChip(SNDCHIP_MMC5, INST_2A03, _T("Nintendo MMC5"));
	AddChip(SNDCHIP_N163, INST_N163, _T("Namco 163"));
	AddChip(SNDCHIP_S5B,  INST_S5B,  _T("Sunsoft 5B"));
}

void CChannelMap::AddChip(int Ident, inst_type_t Inst, LPCTSTR pName)
{
	ASSERT(m_iAddedChips < CHIP_COUNT);

	m_pChipNames[m_iAddedChips] = pName;
	m_iChipIdents[m_iAddedChips] = Ident;
	m_iChipInstType[m_iAddedChips] = Inst;
	++m_iAddedChips;
}

int CChannelMap::GetChipCount() const
{
	// Return number of available chips
	return m_iAddedChips;
}

LPCTSTR CChannelMap::GetChipName(int Index) const
{
	// Get chip name from index
	return m_pChipNames[Index];
}

int CChannelMap::GetChipIdent(int Index) const
{
	// Get chip ID from index
	return m_iChipIdents[Index];
}

int	CChannelMap::GetChipIndex(int Ident) const
{
	// Get index from chip ID
	for (int i = 0; i < m_iAddedChips; ++i) {
		if (Ident == m_iChipIdents[i])
			return i;
	}
	return 0;
}

CInstrument* CChannelMap::GetChipInstrument(int Chip) const
{
	// Get instrument from chip ID
	int Index = GetChipIndex(Chip);

	return CInstrumentFactory::CreateNew(m_iChipInstType[Index]);		// // //
}

// Todo move enabled module channels here

int CChannelMap::GetChannelType(int Channel) const
{
	// Return channel type form channel index
	ASSERT(m_iRegisteredChannels != 0);
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
