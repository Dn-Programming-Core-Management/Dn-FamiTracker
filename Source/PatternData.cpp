/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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

#include "stdafx.h"
#include "FamiTrackerTypes.h"		// // //
#include "PatternData.h"
#include <algorithm>		// // // std::swap

// Defaults when creating new modules
const unsigned CPatternData::DEFAULT_ROW_COUNT	= 64;
const CString CPatternData::DEFAULT_TITLE = _T("New song");		// // //
const stHighlight CPatternData::DEFAULT_HIGHLIGHT = {4, 16, 0};		// // //

// This class contains pattern data
// A list of these objects exists inside the document one for each song

CPatternData::CPatternData(unsigned int PatternLength) :		// // //
	m_sTrackName(DEFAULT_TITLE),		// // //
	m_iPatternLength(PatternLength),
	m_iFrameCount(1),
	m_iSongSpeed(DEFAULT_SPEED),
	m_iSongTempo(DEFAULT_TEMPO_NTSC),
	m_bUseGroove(false),		// // //
	m_vRowHighlight(DEFAULT_HIGHLIGHT),		// // //
	m_iFrameList(),		// // //
	m_pPatternData(),
	m_iEffectColumns()
{
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
	const stChanNote *pNote = GetPatternData(Channel, Pattern, Row);

	return !pNote || pNote->Note == NONE &&		// // //
		pNote->EffNumber[0] == EF_NONE && pNote->EffNumber[1] == EF_NONE &&
		pNote->EffNumber[2] == EF_NONE && pNote->EffNumber[3] == EF_NONE &&
		pNote->Vol == MAX_VOLUME && pNote->Instrument == MAX_INSTRUMENTS;
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
		return nullptr;

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
	stChanNote Blank { };		// // //
	for (int i = 0; i < MAX_PATTERN_LENGTH; ++i)
		memcpy(m_pPatternData[Channel][Pattern] + i, &Blank, sizeof(stChanNote));
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
	SAFE_RELEASE_ARRAY(m_pPatternData[Channel][Pattern]);
}

CString CPatternData::GetTitle() const
{
	return m_sTrackName;
}

unsigned int CPatternData::GetPatternLength() const
{
	return m_iPatternLength;
}

unsigned int CPatternData::GetFrameCount() const
{
	return m_iFrameCount;
}

unsigned int CPatternData::GetSongSpeed() const
{
	return m_iSongSpeed;
}

unsigned int CPatternData::GetSongTempo() const
{
	return m_iSongTempo;
}

int CPatternData::GetEffectColumnCount(int Channel) const
{
	return m_iEffectColumns[Channel];
}

bool CPatternData::GetSongGroove() const		// // //
{
	return m_bUseGroove;
}

void CPatternData::SetTitle(CString str)
{
	m_sTrackName = str;
}

void CPatternData::SetPatternLength(unsigned int Length)
{
	m_iPatternLength = Length;
}

void CPatternData::SetFrameCount(unsigned int Count)
{
	m_iFrameCount = Count;
}

void CPatternData::SetSongSpeed(unsigned int Speed)
{
	m_iSongSpeed = Speed;
}

void CPatternData::SetSongTempo(unsigned int Tempo)
{
	m_iSongTempo = Tempo;
}

void CPatternData::SetEffectColumnCount(int Channel, int Count)
{
	m_iEffectColumns[Channel] = Count;
}

void CPatternData::SetSongGroove(bool Groove)		// // //
{
	m_bUseGroove = Groove;
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

void CPatternData::SwapChannels(unsigned int First, unsigned int Second)		// // //
{
	for (int i = 0; i < MAX_FRAMES; i++) {
		std::swap(m_iFrameList[i][First], m_iFrameList[i][Second]);
	}
	for (int i = 0; i < MAX_PATTERN; i++) {
		std::swap(m_pPatternData[First][i], m_pPatternData[Second][i]);
	}
}
