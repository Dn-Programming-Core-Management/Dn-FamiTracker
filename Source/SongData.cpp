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

#include "SongData.h"
#include "PatternNote.h"		// // //

// Defaults when creating new modules
const unsigned CSongData::DEFAULT_ROW_COUNT	= 64;
const std::string CSongData::DEFAULT_TITLE = "New song";		// // //
const stHighlight CSongData::DEFAULT_HIGHLIGHT = {4, 16, 0};		// // //

// This class contains pattern data
// A list of these objects exists inside the document one for each song

CSongData::CSongData(unsigned int PatternLength) :		// // //
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

CSongData::~CSongData()
{
	// // //
}

bool CSongData::IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row) const
{
	const auto &Note = GetPatternData(Channel, Pattern, Row);		// // //

	return Note.Note == NONE &&
		Note.EffNumber[0] == EF_NONE && Note.EffNumber[1] == EF_NONE &&
		Note.EffNumber[2] == EF_NONE && Note.EffNumber[3] == EF_NONE &&
		Note.Vol == MAX_VOLUME && Note.Instrument == MAX_INSTRUMENTS;
}

bool CSongData::IsPatternEmpty(unsigned int Channel, unsigned int Pattern) const
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

bool CSongData::IsPatternInUse(unsigned int Channel, unsigned int Pattern) const
{
	// Check if pattern is addressed in frame list
	for (unsigned i = 0; i < m_iFrameCount; ++i) {
		if (m_iFrameList[i][Channel] == Pattern)
			return true;
	}

	return false;
}

bool CSongData::ArePatternsSame(unsigned ch1, unsigned pat1, unsigned ch2, unsigned pat2) const {		// // //
	const auto &p1 = m_pPatternData[ch1][pat1];
	const auto &p2 = m_pPatternData[ch2][pat2];
	return (!p1 && !p2) || (p1 && p2 && *p1 == *p2);
}

stChanNote &CSongData::GetPatternData(unsigned Channel, unsigned Pattern, unsigned Row)		// // //
{
	if (!m_pPatternData[Channel][Pattern])
		AllocatePattern(Channel, Pattern);
	return (*m_pPatternData[Channel][Pattern])[Row];
}

const stChanNote &CSongData::GetPatternData(unsigned Channel, unsigned Pattern, unsigned Row) const		// // //
{
	static const auto BLANK = stChanNote { };		// // //
	if (!m_pPatternData[Channel][Pattern])
		return BLANK;
	return (*m_pPatternData[Channel][Pattern])[Row];
}

void CSongData::SetPatternData(unsigned Channel, unsigned Pattern, unsigned Row, const stChanNote &Note)		// // //
{
	if (!m_pPatternData[Channel][Pattern])		// Allocate pattern if accessed for the first time
		AllocatePattern(Channel, Pattern);
	(*m_pPatternData[Channel][Pattern])[Row] = Note;
}

void CSongData::AllocatePattern(unsigned int Channel, unsigned int Pattern)
{
	m_pPatternData[Channel][Pattern] = std::make_unique<pattern_t>();
}

void CSongData::ClearEverything()
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

void CSongData::ClearPattern(unsigned int Channel, unsigned int Pattern)
{
	// Deletes a specified pattern in a channel
	m_pPatternData[Channel][Pattern].reset();		// // //
}

const std::string &CSongData::GetTitle() const		// // //
{
	return m_sTrackName;
}

unsigned int CSongData::GetPatternLength() const
{
	return m_iPatternLength;
}

unsigned int CSongData::GetFrameCount() const
{
	return m_iFrameCount;
}

unsigned int CSongData::GetSongSpeed() const
{
	return m_iSongSpeed;
}

unsigned int CSongData::GetSongTempo() const
{
	return m_iSongTempo;
}

int CSongData::GetEffectColumnCount(int Channel) const
{
	return m_iEffectColumns[Channel];
}

bool CSongData::GetSongGroove() const		// // //
{
	return m_bUseGroove;
}

void CSongData::SetTitle(const std::string &str)		// // //
{
	m_sTrackName = str;
}

void CSongData::SetPatternLength(unsigned int Length)
{
	m_iPatternLength = Length;
}

void CSongData::SetFrameCount(unsigned int Count)
{
	m_iFrameCount = Count;
}

void CSongData::SetSongSpeed(unsigned int Speed)
{
	m_iSongSpeed = Speed;
}

void CSongData::SetSongTempo(unsigned int Tempo)
{
	m_iSongTempo = Tempo;
}

void CSongData::SetEffectColumnCount(int Channel, int Count)
{
	m_iEffectColumns[Channel] = Count;
}

void CSongData::SetSongGroove(bool Groove)		// // //
{
	m_bUseGroove = Groove;
}

unsigned int CSongData::GetFramePattern(unsigned int Frame, unsigned int Channel) const
{ 
	return m_iFrameList[Frame][Channel]; 
}

void CSongData::SetFramePattern(unsigned int Frame, unsigned int Channel, unsigned int Pattern)
{
	m_iFrameList[Frame][Channel] = Pattern;
	if (!m_pPatternData[Channel][Pattern])		// // // Allocate pattern if accessed for the first time
		AllocatePattern(Channel, Pattern);
}

void CSongData::SetHighlight(const stHighlight &Hl)		// // //
{
	m_vRowHighlight = Hl;
}

stHighlight CSongData::GetRowHighlight() const
{
	return m_vRowHighlight;
}

void CSongData::CopyPattern(unsigned Chan, unsigned Pat, const CSongData &From, unsigned ChanFrom, unsigned PatFrom) {		// // //
	if (const auto &p1 = From.m_pPatternData[ChanFrom][PatFrom])
		if (auto &p2 = m_pPatternData[Chan][Pat])
			*p2 = *p1;
		else
			p2 = std::make_unique<pattern_t>(*p1);
	else
		m_pPatternData[Chan][Pat].reset();
}

void CSongData::CopyTrack(unsigned Chan, const CSongData &From, unsigned ChanFrom) {
	SetEffectColumnCount(Chan, From.GetEffectColumnCount(ChanFrom));
	for (int f = 0; f < MAX_FRAMES; f++)
		SetFramePattern(f, Chan, From.GetFramePattern(f, ChanFrom));
	for (int p = 0; p < MAX_PATTERN; p++)
		CopyPattern(Chan, p, From, ChanFrom, p);
}

void CSongData::SwapChannels(unsigned int First, unsigned int Second)		// // //
{
	std::swap(m_iEffectColumns[First], m_iEffectColumns[Second]);
	for (int i = 0; i < MAX_FRAMES; i++)
		std::swap(m_iFrameList[i][First], m_iFrameList[i][Second]);
	std::swap(m_pPatternData[First], m_pPatternData[Second]);
}
