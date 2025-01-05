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
