/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

#include "stdafx.h"
#include "APU/Types.h"
#include "Instrument.h"
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
	for (int i = 0; i < m_iAddedChips; ++i) {
		m_pChipInst[i]->Release();
		m_pChipInst[i] = NULL;
	}
}

void CChannelMap::SetupSoundChips()
{
	// Add available chips
#ifdef _DEBUG
	// Under development
	AddChip(SNDCHIP_NONE, new CInstrument2A03(), _T("NES channels only"));
	AddChip(SNDCHIP_VRC6, new CInstrumentVRC6(), _T("Konami VRC6"));
	AddChip(SNDCHIP_VRC7, new CInstrumentVRC7(), _T("Konami VRC7"));
	AddChip(SNDCHIP_FDS,  new CInstrumentFDS(),  _T("Nintendo FDS sound"));
	AddChip(SNDCHIP_MMC5, new CInstrument2A03(), _T("Nintendo MMC5"));
	AddChip(SNDCHIP_N163, new CInstrumentN163(), _T("Namco 163"));
	AddChip(SNDCHIP_S5B,  new CInstrumentS5B(),  _T("Sunsoft 5B"));
#else /* _DEBUG */
	// Ready for use
	AddChip(SNDCHIP_NONE, new CInstrument2A03(), _T("NES channels only"));
	AddChip(SNDCHIP_VRC6, new CInstrumentVRC6(), _T("Konami VRC6"));
	AddChip(SNDCHIP_VRC7, new CInstrumentVRC7(), _T("Konami VRC7"));
	AddChip(SNDCHIP_FDS,  new CInstrumentFDS(),  _T("Nintendo FDS sound"));
	AddChip(SNDCHIP_MMC5, new CInstrument2A03(), _T("Nintendo MMC5"));
	AddChip(SNDCHIP_N163, new CInstrumentN163(), _T("Namco 163"));
	AddChip(SNDCHIP_S5B,  new CInstrumentS5B(),  _T("Sunsoft 5B"));		// // //
#endif /* _DEBUG */
}

void CChannelMap::AddChip(int Ident, CInstrument *pInst, LPCTSTR pName)
{
	ASSERT(m_iAddedChips < CHIP_COUNT);

	m_pChipNames[m_iAddedChips] = pName;
	m_iChipIdents[m_iAddedChips] = Ident;
	m_pChipInst[m_iAddedChips] = pInst;
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

	if (m_pChipInst[Index] == NULL)
		return NULL;

	return m_pChipInst[Index]->CreateNew();
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
