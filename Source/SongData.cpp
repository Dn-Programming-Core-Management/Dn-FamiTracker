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
#include "PatternData.h"		// // //

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
//	for (int i = 0; i < MAX_CHANNELS; ++i)
//		m_pPatternData[i][0].Allocate();		// // //
}

bool CSongData::IsCellFree(unsigned int Channel, unsigned int Pattern, unsigned int Row) const
{
	const auto &Note = GetPatternData(Channel, Pattern, Row);		// // //
	constexpr const auto BLANK = stChanNote { };
	return Note == BLANK;
}

bool CSongData::IsPatternEmpty(unsigned int Channel, unsigned int Pattern) const
{
	return m_pPatternData[Channel][Pattern].IsEmpty();		// // //
}

bool CSongData::IsPatternInUse(unsigned int Channel, unsigned int Pattern) const
{
	// Check if pattern is addressed in frame list
	for (unsigned i = 0; i < m_iFrameCount; ++i)
		if (m_iFrameList[i][Channel] == Pattern)
			return true;
	return false;
}

bool CSongData::ArePatternsSame(unsigned ch1, unsigned pat1, unsigned ch2, unsigned pat2) const {		// // //
	return GetPattern(ch1, pat1) == GetPattern(ch2, pat2);
}

stChanNote &CSongData::GetPatternData(unsigned Channel, unsigned Pattern, unsigned Row)		// // //
{
	return GetPattern(Channel, Pattern).GetNoteOn(Row);
}

const stChanNote &CSongData::GetPatternData(unsigned Channel, unsigned Pattern, unsigned Row) const		// // //
{
	return GetPattern(Channel, Pattern).GetNoteOn(Row);
}

void CSongData::SetPatternData(unsigned Channel, unsigned Pattern, unsigned Row, const stChanNote &Note)		// // //
{
	GetPattern(Channel, Pattern).SetNoteOn(Row, Note);
}

CPatternData &CSongData::GetPattern(unsigned Channel, unsigned Pattern) {
	return m_pPatternData[Channel][Pattern];
}

const CPatternData &CSongData::GetPattern(unsigned Channel, unsigned Pattern) const {
	return m_pPatternData[Channel][Pattern];
}

CPatternData &CSongData::GetPatternOnFrame(unsigned Channel, unsigned Frame) {
	return GetPattern(Channel, GetFramePattern(Frame, Channel));
}

const CPatternData &CSongData::GetPatternOnFrame(unsigned Channel, unsigned Frame) const {
	return GetPattern(Channel, GetFramePattern(Frame, Channel));
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
			p = CPatternData { };
}

void CSongData::ClearPattern(unsigned int Channel, unsigned int Pattern)
{
	// Deletes a specified pattern in a channel
	GetPattern(Channel, Pattern) = CPatternData { };		// // //
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
}

unsigned CSongData::GetFrameSize(unsigned Frame, unsigned MaxChans) const {		// // //
	const unsigned PatternLength = GetPatternLength();	// default length
	unsigned HaltPoint = PatternLength;

	for (int i = 0; i < MaxChans; ++i) {
		unsigned halt = [&] {
			const int Columns = GetEffectColumnCount(i) + 1;
			const auto &pat = GetPatternOnFrame(i, Frame);
			for (unsigned j = 0; j < PatternLength - 1; ++j) {
				const auto &Note = pat.GetNoteOn(j);
				for (int k = 0; k < Columns; ++k)
					switch (Note.EffNumber[k])
					case EF_SKIP: case EF_JUMP: case EF_HALT:
						return j + 1;
			}
			return PatternLength;
		}();
		if (halt < HaltPoint)
			HaltPoint = halt;
	}

	return HaltPoint;
}

void CSongData::SetHighlight(const stHighlight &Hl)		// // //
{
	m_vRowHighlight = Hl;
}

const stHighlight &CSongData::GetRowHighlight() const
{
	return m_vRowHighlight;
}

void CSongData::CopyTrack(unsigned Chan, const CSongData &From, unsigned ChanFrom) {
	SetEffectColumnCount(Chan, From.GetEffectColumnCount(ChanFrom));
	for (int f = 0; f < MAX_FRAMES; f++)
		SetFramePattern(f, Chan, From.GetFramePattern(f, ChanFrom));
	for (int p = 0; p < MAX_PATTERN; p++)
		GetPattern(Chan, p) = From.GetPattern(ChanFrom, p);
}

void CSongData::SwapChannels(unsigned int First, unsigned int Second)		// // //
{
	std::swap(m_iEffectColumns[First], m_iEffectColumns[Second]);
	for (int i = 0; i < MAX_FRAMES; i++)
		std::swap(m_iFrameList[i][First], m_iFrameList[i][Second]);
	std::swap(m_pPatternData[First], m_pPatternData[Second]);
}
