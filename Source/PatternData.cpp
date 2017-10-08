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

#include "PatternData.h"
#include "PatternNote.h"		// // //

// Defaults when creating new modules
const unsigned CPatternData::DEFAULT_ROW_COUNT	= 64;
const std::string CPatternData::DEFAULT_TITLE = "New song";		// // //
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
	m_vRowHighlight(DEFAULT_HIGHLIGHT)		// // //
{
	// // // Pre-allocate pattern 0 for all channels
	for (int i = 0; i < MAX_CHANNELS; ++i)
		AllocatePattern(i, 0);
}

CPatternData::~CPatternData()
{
	// // //
}

bool CPatternData::IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row) const
{
	const auto &Note = GetPatternData(Channel, Pattern, Row);		// // //

	return Note.Note == NONE &&
		Note.EffNumber[0] == EF_NONE && Note.EffNumber[1] == EF_NONE &&
		Note.EffNumber[2] == EF_NONE && Note.EffNumber[3] == EF_NONE &&
		Note.Vol == MAX_VOLUME && Note.Instrument == MAX_INSTRUMENTS;
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

bool CPatternData::ArePatternsSame(unsigned ch1, unsigned pat1, unsigned ch2, unsigned pat2) const {		// // //
	const auto &p1 = m_pPatternData[ch1][pat1];
	const auto &p2 = m_pPatternData[ch2][pat2];
	return (!p1 && !p2) || (p1 && p2 && *p1 == *p2);
}

stChanNote &CPatternData::GetPatternData(unsigned Channel, unsigned Pattern, unsigned Row)		// // //
{
	if (!m_pPatternData[Channel][Pattern])
		AllocatePattern(Channel, Pattern);
	return (*m_pPatternData[Channel][Pattern])[Row];
}

const stChanNote &CPatternData::GetPatternData(unsigned Channel, unsigned Pattern, unsigned Row) const		// // //
{
	static const auto BLANK = stChanNote { };		// // //
	if (!m_pPatternData[Channel][Pattern])
		return BLANK;
	return (*m_pPatternData[Channel][Pattern])[Row];
}

void CPatternData::SetPatternData(unsigned Channel, unsigned Pattern, unsigned Row, const stChanNote &Note)		// // //
{
	if (!m_pPatternData[Channel][Pattern])		// Allocate pattern if accessed for the first time
		AllocatePattern(Channel, Pattern);
	(*m_pPatternData[Channel][Pattern])[Row] = Note;
}

void CPatternData::AllocatePattern(unsigned int Channel, unsigned int Pattern)
{
	m_pPatternData[Channel][Pattern] = std::make_unique<pattern_t>();
}

void CPatternData::ClearEverything()
{
	// Release all patterns and clear frame list

	// Frame list
	m_iFrameList.fill({ });		// // //
	m_iFrameCount = 1;
	
	// Patterns, deallocate everything
	for (auto &x : m_pPatternData)		// // //
		for (auto &p : x)
			p.reset();
}

void CPatternData::ClearPattern(unsigned int Channel, unsigned int Pattern)
{
	// Deletes a specified pattern in a channel
	m_pPatternData[Channel][Pattern].reset();		// // //
}

const std::string &CPatternData::GetTitle() const		// // //
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

void CPatternData::SetTitle(const std::string &str)		// // //
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
	if (!m_pPatternData[Channel][Pattern])		// // // Allocate pattern if accessed for the first time
		AllocatePattern(Channel, Pattern);
}

void CPatternData::SetHighlight(const stHighlight &Hl)		// // //
{
	m_vRowHighlight = Hl;
}

stHighlight CPatternData::GetRowHighlight() const
{
	return m_vRowHighlight;
}

void CPatternData::CopyPattern(unsigned Chan, unsigned Pat, const CPatternData &From, unsigned ChanFrom, unsigned PatFrom) {		// // //
	const auto &p1 = From.m_pPatternData[ChanFrom][PatFrom];
	if (auto &p2 = m_pPatternData[Chan][Pat])
		*p2 = *p1;
	else
		p2 = std::make_unique<pattern_t>(*p1);
}

void CPatternData::SwapChannels(unsigned int First, unsigned int Second)		// // //
{
	for (int i = 0; i < MAX_FRAMES; i++)
		std::swap(m_iFrameList[i][First], m_iFrameList[i][Second]);
	std::swap(m_pPatternData[First], m_pPatternData[Second]);
}
