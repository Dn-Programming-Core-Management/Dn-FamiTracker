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
#include "FamiTrackerDoc.h"
#include "PatternData.h"

// This class contains pattern data
// A list of these objects exists inside the document one for each song

CPatternData::CPatternData(unsigned int PatternLength) :		// // //
	m_iPatternLength(PatternLength),
	m_iFrameCount(1),
	m_iSongSpeed(DEFAULT_SPEED),
	m_iSongTempo(DEFAULT_TEMPO_NTSC),
	m_bUseGroove(false),		// // //
	m_vRowHighlight(CFamiTrackerDoc::DEFAULT_HIGHLIGHT)		// // //
{
	// Clear memory
	memset(m_iFrameList, 0, sizeof(char) * MAX_FRAMES * MAX_CHANNELS);
	memset(m_pPatternData, 0, sizeof(stChanNote*) * MAX_CHANNELS * MAX_PATTERN);
	memset(m_iEffectColumns, 0, sizeof(char) * MAX_CHANNELS);

	// // // Pre-allocate pattern 0 for all channels
	for (int i = 0; i < MAX_CHANNELS; ++i)
		AllocatePattern(i, 0);
}

CPatternData::~CPatternData()
{
	// Deallocate memory
	for (int i = 0; i < MAX_CHANNELS; ++i) {
		for (int j = 0; j < MAX_PATTERN; ++j) {
			SAFE_RELEASE_ARRAY(m_pPatternData[i][j]);
		}
	}
}

bool CPatternData::IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row) const
{
	stChanNote *pNote = GetPatternData(Channel, Pattern, Row);

	if (pNote == NULL)
		return true;

	bool IsFree = pNote->Note == NONE && 
		pNote->EffNumber[0] == 0 && pNote->EffNumber[1] == 0 && 
		pNote->EffNumber[2] == 0 && pNote->EffNumber[3] == 0 && 
		pNote->Vol == MAX_VOLUME && pNote->Instrument == MAX_INSTRUMENTS;

	return IsFree;
}

bool CPatternData::IsPatternEmpty(unsigned int Channel, unsigned int Pattern) const
{
	// Unallocated pattern means empty
	if (!m_pPatternData[Channel][Pattern])
		return true;

	// Check if allocated pattern is empty
	for (unsigned int i = 0; i < m_iPatternLength; ++i) {
		if (!IsCellFree(Channel, Pattern, i))
			return false;
	}

	return true;
}

bool CPatternData::IsPatternInUse(unsigned int Channel, unsigned int Pattern) const
{
	// Check if pattern is addressed in frame list
	for (unsigned i = 0; i < m_iFrameCount; ++i) {
		if (m_iFrameList[i][Channel] == Pattern)
			return true;
	}

	return false;
}

stChanNote *CPatternData::GetPatternData(unsigned int Channel, unsigned int Pattern, unsigned int Row) const
{
	// Private method, may return NULL
	if (!m_pPatternData[Channel][Pattern])
		return NULL;

	return m_pPatternData[Channel][Pattern] + Row;
}

stChanNote *CPatternData::GetPatternData(unsigned int Channel, unsigned int Pattern, unsigned int Row)
{
	if (!m_pPatternData[Channel][Pattern])		// Allocate pattern if accessed for the first time
		AllocatePattern(Channel, Pattern);

	return m_pPatternData[Channel][Pattern] + Row;
}

void CPatternData::AllocatePattern(unsigned int Channel, unsigned int Pattern)
{
	// Allocate memory
	m_pPatternData[Channel][Pattern] = new stChanNote[MAX_PATTERN_LENGTH];

	// Clear memory
	for (int i = 0; i < MAX_PATTERN_LENGTH; ++i)
		*(m_pPatternData[Channel][Pattern] + i) = BLANK_NOTE;		// // //
}

void CPatternData::ClearEverything()
{
	// Release all patterns and clear frame list

	// Frame list
	memset(m_iFrameList, 0, sizeof(char) * MAX_FRAMES * MAX_CHANNELS);
	m_iFrameCount = 1;
	
	// Patterns, deallocate everything
	for (int i = 0; i < MAX_CHANNELS; ++i) {
		for (int j = 0; j < MAX_PATTERN; ++j) {
			ClearPattern(i, j);
		}
	}
}

void CPatternData::ClearPattern(unsigned int Channel, unsigned int Pattern)
{
	// Deletes a specified pattern in a channel
	if (m_pPatternData[Channel][Pattern] != NULL) {
		SAFE_RELEASE_ARRAY(m_pPatternData[Channel][Pattern]);
	}
}

unsigned int CPatternData::GetFramePattern(unsigned int Frame, unsigned int Channel) const
{ 
	return m_iFrameList[Frame][Channel]; 
}

void CPatternData::SetFramePattern(unsigned int Frame, unsigned int Channel, unsigned int Pattern)
{
	m_iFrameList[Frame][Channel] = Pattern;
}

void CPatternData::SetHighlight(const stHighlight Hl)		// // //
{
	m_vRowHighlight = Hl;
}

stHighlight CPatternData::GetRowHighlight() const
{
	return m_vRowHighlight;
}

void CPatternData::CopyChannel(unsigned int Target, unsigned int Source)		// // //
{
	if (Source == Target) return;
	SetEffectColumnCount(Target, GetEffectColumnCount(Source));
	SetEffectColumnCount(Source, 0);
	for (unsigned int i = 0; i < m_iFrameCount; i++) {
		m_iFrameList[i][Target] = m_iFrameList[i][Source];
		m_iFrameList[i][Source] = 0;
	}
	for (unsigned int i = 0; i < MAX_PATTERN; i++) {
		for (unsigned int j = 0; j < m_iPatternLength; j++) {
			stChanNote *s = GetPatternData(Source, i, j);
			stChanNote *t = GetPatternData(Target, i, j);
			memcpy(t, s, sizeof(stChanNote));
			*s = BLANK_NOTE;		// // //
		}
	}
}
