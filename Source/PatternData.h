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


#include "PatternNote.h"		// // //

// // // Highlight settings
struct stHighlight {
	int First;
	int Second;
	int Offset;
};

// // // moved from FamiTrackerDoc.h
const unsigned int DEFAULT_TEMPO_NTSC		 = 150;
const unsigned int DEFAULT_TEMPO_PAL		 = 125;
const unsigned int DEFAULT_SPEED			 = 6;

// TODO rename to CTrack perhaps?

// CPatternData holds all notes in the patterns
class CPatternData
{
public:
	CPatternData(unsigned int PatternLength = DEFAULT_ROW_COUNT);		// // //
	~CPatternData();
	/*
	char GetNote(unsigned int Channel, unsigned int Pattern, unsigned int Row) const {
		stChanNote *pNote = GetPatternData(Channel, Pattern, Row);
		return pNote == NULL ? 0 : pNote->Note;
	};

	char GetOctave(unsigned int Channel, unsigned int Pattern, unsigned int Row) const {
		stChanNote *pNote = GetPatternData(Channel, Pattern, Row);
		return pNote == NULL ? 0 : pNote->Octave;
	};

	char GetInstrument(unsigned int Channel, unsigned int Pattern, unsigned int Row) const {
		stChanNote *pNote = GetPatternData(Channel, Pattern, Row);
		return pNote == NULL ? MAX_INSTRUMENTS : pNote->Instrument;
	};

	char GetVolume(unsigned int Channel, unsigned int Pattern, unsigned int Row) const {
		stChanNote *pNote = GetPatternData(Channel, Pattern, Row);
		return pNote == NULL ? MAX_VOLUME : pNote->Vol;
	};

	char GetEffect(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column) const {
		stChanNote *pNote = GetPatternData(Channel, Pattern, Row);
		return pNote == NULL ? EF_NONE : pNote->EffNumber[Column];
	};

	char GetEffectParam(unsigned int Channel, unsigned int Pattern, unsigned int Row, unsigned int Column) const {
		stChanNote *pNote = GetPatternData(Channel, Pattern, Row);
		return pNote == NULL ? 0 : pNote->EffParam[Column];
	};
	*/
	bool IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row) const;
	bool IsPatternEmpty(unsigned int Channel, unsigned int Pattern) const;
	bool IsPatternInUse(unsigned int Channel, unsigned int Pattern) const;

	int GetEffectColumnCount(int Channel) const
	{
		return m_iEffectColumns[Channel];
	};

	void SetEffectColumnCount(int Channel, int Count)
	{
		m_iEffectColumns[Channel] = Count;
	};

	void ClearEverything();
	void ClearPattern(unsigned int Channel, unsigned int Pattern);

	stChanNote *GetPatternData(unsigned int Channel, unsigned int Pattern, unsigned int Row);

	CString GetTitle() const {		// // //
		return m_sTrackName;
	}

	unsigned int GetPatternLength() const { 
		return m_iPatternLength;
	}

	unsigned int GetFrameCount() const { 
		return m_iFrameCount;
	}

	unsigned int GetSongSpeed() const { 
		return m_iSongSpeed;
	}

	unsigned int GetSongTempo() const { 
		return m_iSongTempo;
	}

	bool GetSongGroove() const {		// // //
		return m_bUseGroove;
	}

	void SetTitle(CString str) {		// // //
		m_sTrackName = str;
	}

	void SetPatternLength(unsigned int Length) {
		m_iPatternLength = Length; 
	}

	void SetFrameCount(unsigned int Count) {
		m_iFrameCount = Count;
	}

	void SetSongSpeed(unsigned int Speed) {
		m_iSongSpeed = Speed;
	}

	void SetSongTempo(unsigned int Tempo) {
		m_iSongTempo = Tempo;
	}

	void SetSongGroove(bool Groove) {		// // //
		m_bUseGroove = Groove;
	}

	unsigned int GetFramePattern(unsigned int Frame, unsigned int Channel) const;
	void SetFramePattern(unsigned int Frame, unsigned int Channel, unsigned int Pattern);

	void SetHighlight(const stHighlight Hl);		// // //
	stHighlight GetRowHighlight() const;

	void SwapChannels(unsigned int First, unsigned int Second);		// // //

private:
	stChanNote *GetPatternData(unsigned int Channel, unsigned int Pattern, unsigned int Row) const;
	void AllocatePattern(unsigned int Channel, unsigned int Patterns);

public:
	// // // moved from CFamiTrackerDoc
	static const CString DEFAULT_TITLE;
	static const stHighlight DEFAULT_HIGHLIGHT;

private:
	static const unsigned DEFAULT_ROW_COUNT;

	// Track parameters
	CString      m_sTrackName;				// // // moved
	unsigned int m_iPatternLength;			// Amount of rows in one pattern
	unsigned int m_iFrameCount;				// Number of frames
	unsigned int m_iSongSpeed;				// Song speed
	unsigned int m_iSongTempo;				// Song tempo
	bool		 m_bUseGroove;				// // // Groove

	// Row highlight settings
	stHighlight  m_vRowHighlight;			// // //

	// Number of visible effect columns for each channel
	unsigned char m_iEffectColumns[MAX_CHANNELS];

	// List of the patterns assigned to frames
	unsigned char m_iFrameList[MAX_FRAMES][MAX_CHANNELS];		

	// All accesses to m_pPatternData must go through GetPatternData()
	stChanNote *m_pPatternData[MAX_CHANNELS][MAX_PATTERN];
};
