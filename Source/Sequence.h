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

class CDocumentFile;

// // // Settings
enum seq_setting_t : unsigned int {
	SETTING_DEFAULT        = 0,

	SETTING_VOL_16_STEPS   = 0,		// // // 050B
	SETTING_VOL_64_STEPS   = 1,		// // // 050B

	SETTING_ARP_ABSOLUTE   = 0,
	SETTING_ARP_FIXED      = 1,
	SETTING_ARP_RELATIVE   = 2,
	SETTING_ARP_SCHEME     = 3,

	SETTING_PITCH_RELATIVE = 0,
	SETTING_PITCH_ABSOLUTE = 1,		// // // 050B
#ifdef _DEBUG
	SETTING_PITCH_SWEEP    = 2,		// // // 050B
#endif
};

#ifdef _DEBUG
static const unsigned int SEQ_SETTING_COUNT[] = {2, 4, 3, 1, 1};
#else
static const unsigned int SEQ_SETTING_COUNT[] = {2, 4, 2, 1, 1};
#endif

// // // Sunsoft modes
enum s5b_mode_t {
	S5B_MODE_ENVELOPE = 0x20,
	S5B_MODE_SQUARE   = 0x40,
	S5B_MODE_NOISE    = 0x80
};

// // // Arpeggio scheme modes
enum arp_scheme_mode_t {
	ARPSCHEME_MODE_X     = 0x40,
	ARPSCHEME_MODE_Y     = 0x80,
	ARPSCHEME_MODE_NEG_Y = 0xC0
};

const int ARPSCHEME_MAX = 36;		// // // highest note offset for arp schemes
const int ARPSCHEME_MIN = ARPSCHEME_MAX - 0x3F;		// // //

#include "CustomExporterInterfaces.h"		// // //

/*
** This class is used to store instrument sequences
*/
class CSequence: public CSequenceInterface {
public:
	CSequence();

	bool         operator==(const CSequence &other);		// // //

	void		 Clear();
	signed char	 GetItem(int Index) const;
	unsigned int GetItemCount() const;
	unsigned int GetLoopPoint() const;
	unsigned int GetReleasePoint() const;
	unsigned int GetSetting() const; // not seq_setting_t due to CSequenceInterface
	void		 SetItem(int Index, signed char Value);
	void		 SetItemCount(unsigned int Count);
	void		 SetLoopPoint(unsigned int Point);
	void		 SetReleasePoint(unsigned int Point);
	void		 SetSetting(seq_setting_t Setting);			// // //
	void		 Copy(const CSequence *pSeq);

private:
	// Sequence data
	unsigned int m_iItemCount;
	unsigned int m_iLoopPoint;
	unsigned int m_iReleasePoint;
	seq_setting_t m_iSetting;		// // //
	signed char	 m_cValues[MAX_SEQUENCE_ITEMS];
	int			 m_iPlaying; // unused
};
