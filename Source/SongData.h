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


#pragma once

#include <array>		// // //
#include <memory>		// // //
#include <string>		// // //
#include "FamiTrackerTypes.h"		// // //
#include "PatternData.h"		// // //

class stChanNote;		// // //

// // // Highlight settings
struct stHighlight {
	int First;
	int Second;
	int Offset;
};

// // // moved from FamiTrackerDoc.h
const unsigned int DEFAULT_TEMPO_NTSC = 150;
const unsigned int DEFAULT_TEMPO_PAL  = 125;
const unsigned int DEFAULT_SPEED      = 6;

// CSongData holds all notes in the patterns
class CSongData
{
public:
	CSongData(unsigned int PatternLength = DEFAULT_ROW_COUNT);		// // //

	bool IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row) const;
	bool IsPatternEmpty(unsigned int Channel, unsigned int Pattern) const;
	bool IsPatternInUse(unsigned int Channel, unsigned int Pattern) const;
	bool ArePatternsSame(unsigned ch1, unsigned pat1, unsigned ch2, unsigned pat2) const;		// // //

	void ClearEverything();
	void ClearPattern(unsigned int Channel, unsigned int Pattern);

	stChanNote &GetPatternData(unsigned Channel, unsigned Pattern, unsigned Row);		// // //
	const stChanNote &GetPatternData(unsigned Channel, unsigned Pattern, unsigned Row) const;		// // //
	void SetPatternData(unsigned Channel, unsigned Pattern, unsigned Row, const stChanNote &Note);		// // //

	CPatternData &GetPattern(unsigned Channel, unsigned Pattern);		// // //
	const CPatternData &GetPattern(unsigned Channel, unsigned Pattern) const;		// // //
	CPatternData &GetPatternOnFrame(unsigned Channel, unsigned Frame);		// // //
	const CPatternData &GetPatternOnFrame(unsigned Channel, unsigned Frame) const;		// // //

	const std::string &GetTitle() const;		// // //
	unsigned int GetPatternLength() const;
	unsigned int GetFrameCount() const;
	unsigned int GetSongSpeed() const;
	unsigned int GetSongTempo() const;
	int GetEffectColumnCount(int Channel) const;;
	bool GetSongGroove() const;		// // //

	void SetTitle(const std::string &str);		// // //
	void SetPatternLength(unsigned int Length);
	void SetFrameCount(unsigned int Count);
	void SetSongSpeed(unsigned int Speed);
	void SetSongTempo(unsigned int Tempo);
	void SetEffectColumnCount(int Channel, int Count);;
	void SetSongGroove(bool Groove);		// // //

	unsigned int GetFramePattern(unsigned int Frame, unsigned int Channel) const;
	void SetFramePattern(unsigned int Frame, unsigned int Channel, unsigned int Pattern);

	unsigned GetFrameSize(unsigned Frame, unsigned MaxChans) const;		// // //

	void SetHighlight(const stHighlight &Hl);		// // //
	const stHighlight &GetRowHighlight() const;

	void CopyTrack(unsigned Chan, const CSongData &From, unsigned ChanFrom);		// // //
	void SwapChannels(unsigned int First, unsigned int Second);		// // //

public:
	// // // moved from CFamiTrackerDoc
	static const std::string DEFAULT_TITLE;
	static const stHighlight DEFAULT_HIGHLIGHT;

private:
	static const unsigned DEFAULT_ROW_COUNT;

	// Track parameters
	std::string	 m_sTrackName;				// // // moved
	unsigned int m_iPatternLength;			// Amount of rows in one pattern
	unsigned int m_iFrameCount;				// Number of frames
	unsigned int m_iSongSpeed;				// Song speed
	unsigned int m_iSongTempo;				// Song tempo
	bool		 m_bUseGroove;				// // // Groove

	// Row highlight settings
	stHighlight  m_vRowHighlight;			// // //

	// Number of visible effect columns for each channel
	std::array<unsigned char, MAX_CHANNELS> m_iEffectColumns = { };		// // //

	// List of the patterns assigned to frames
	std::array<std::array<unsigned char, MAX_CHANNELS>, MAX_FRAMES> m_iFrameList = { };		// // //

	// All accesses to m_pPatternData must go through GetPatternData()
	std::array<std::array<CPatternData, MAX_PATTERN>, MAX_CHANNELS> m_pPatternData;		// // //
};
