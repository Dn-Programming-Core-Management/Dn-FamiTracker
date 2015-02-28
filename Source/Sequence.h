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

#pragma once


#include "CustomExporterInterfaces.h"

class CDocumentFile;

// // // Settings
enum seq_setting_t : unsigned int {
	SETTING_DEFAULT        = 0x0000,
	SETTING_ARP_ABSOLUTE   = 0x0000,
	SETTING_ARP_FIXED      = 0x0001,
	SETTING_ARP_RELATIVE   = 0x0002,
	SETTING_ARP_SCHEME     = 0x0003,
	SETTING_PITCH_DEFAULT  = 0x0000,
	SETTING_PITCH_ABSOLUTE = 0x0010,
	SETTING_PITCH_RELATIVE = 0x0020,
};

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

/*
** This class is used to store instrument sequences
*/
class CSequence: public CSequenceInterface {
public:
	CSequence();

	void		 Clear();
	signed char	 GetItem(int Index) const;
	unsigned int GetItemCount() const;
	unsigned int GetLoopPoint() const;
	unsigned int GetReleasePoint() const;
	seq_setting_t GetSetting() const;		// // //
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
	int			 m_iPlaying;
};
