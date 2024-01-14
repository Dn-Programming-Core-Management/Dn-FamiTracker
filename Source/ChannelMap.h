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

#pragma once


// CChannelMap

class CChannelMap
{
public:
	CChannelMap();
	~CChannelMap();
	void SetupSoundChips();

	int			GetChipCount() const;
	LPCTSTR		GetChipName(int Index) const;
	int			GetChipIdent(int Index) const;
	int			GetChipIndex(int Ident) const;
	CInstrument	*GetChipInstrument(int Chip) const;

	// Active channel map
	void			ResetChannels();
	void			RegisterChannel(CTrackerChannel *pChannel, int ChannelType, int ChipType);
	CTrackerChannel	*GetChannel(int Index) const;
	int				GetChannelType(int Channel) const;
	int				GetChipType(int Channel) const;

public:
	static const int CHIP_COUNT = 8;	// Number of allowed expansion chips

protected:
	void AddChip(int Ident, inst_type_t Inst, LPCTSTR pName);

protected:
	// Chips
	int				m_iAddedChips;
	int				m_iChipIdents[CHIP_COUNT];
	LPCTSTR			m_pChipNames[CHIP_COUNT];
	inst_type_t		m_iChipInstType[CHIP_COUNT];		// // //

	// Current set
	CTrackerChannel	*m_pChannels[CHANNELS];
	int				m_iRegisteredChannels;
	int				m_iChannelTypes[CHANNELS];
	int				m_iChannelChip[CHANNELS];

};
