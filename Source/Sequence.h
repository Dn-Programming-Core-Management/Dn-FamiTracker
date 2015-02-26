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
	unsigned int GetSetting() const;
	void		 SetItem(int Index, signed char Value);
	void		 SetItemCount(unsigned int Count);
	void		 SetLoopPoint(unsigned int Point);
	void		 SetReleasePoint(unsigned int Point);
	void		 SetSetting(unsigned int Setting); 
	void		 Copy(const CSequence *pSeq);

private:
	// Sequence data
	unsigned int m_iItemCount;
	unsigned int m_iLoopPoint;
	unsigned int m_iReleasePoint;
	unsigned int m_iSetting;
	signed char	 m_cValues[MAX_SEQUENCE_ITEMS];
	int			 m_iPlaying;
};


// Settings
enum arp_setting_t {
	ARP_SETTING_ABSOLUTE = 0,
	ARP_SETTING_FIXED = 1,
	ARP_SETTING_RELATIVE = 2,
	ARP_SETTING_SCHEME = 3			// // //
};

// Sunsoft modes
enum s5b_mode_t {		// // //
	S5B_MODE_ENVELOPE = 32,
	S5B_MODE_SQUARE = 64,
	S5B_MODE_NOISE = 128
};

// Arpeggio scheme modes			// // //
enum arp_scheme_mode_t {
	ARPSCHEME_MODE_X = 64,
	ARPSCHEME_MODE_Y = 128,
	ARPSCHEME_MODE_NEG_Y = 192
};