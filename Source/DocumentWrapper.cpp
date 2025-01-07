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

#include "stdafx.h"
#include "DocumentWrapper.h"

// This will implement the CFamiTrackerDocInterface for export plugins

CFamiTrackerDocWrapper::CFamiTrackerDocWrapper(CFamiTrackerDoc *pDocument, int iTrack) :
	m_pDocument(pDocument),
	m_iTrack(iTrack)
{
}

void CFamiTrackerDocWrapper::GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const
{
	m_pDocument->GetNoteData(m_iTrack, Frame, Channel, Row, Data);
}

unsigned int CFamiTrackerDocWrapper::GetFrameCount() const
{
	return m_pDocument->GetFrameCount(m_iTrack);
}

unsigned int CFamiTrackerDocWrapper::GetPatternLength() const
{
	return m_pDocument->GetPatternLength(m_iTrack);
}

unsigned int CFamiTrackerDocWrapper::GetSongSpeed() const
{
	return m_pDocument->GetSongSpeed(m_iTrack);
}

CSequenceInterface const *CFamiTrackerDocWrapper::GetSequence(unsigned int Index, int Type) const
{
	return m_pDocument->GetSequence(INST_2A03, Index, Type);		// // //
}

int CFamiTrackerDocWrapper::GetSequenceCount(int Type) const
{
	return m_pDocument->GetSequenceCount(INST_2A03, Type);		// // //
}

int CFamiTrackerDocWrapper::GetInstrumentCount() const
{
	return m_pDocument->GetInstrumentCount();
}

CInstrument2A03Interface const *CFamiTrackerDocWrapper::Get2A03Instrument(int Instrument) const
{
	return std::dynamic_pointer_cast<const CInstrument2A03Interface>(m_pDocument->GetInstrument(Instrument)).get();
}

CSeqInstrumentInterface const *CFamiTrackerDocWrapper::GetSeqInstrument(int Instrument) const		// // //
{
	return std::dynamic_pointer_cast<const CSeqInstrumentInterface>(m_pDocument->GetInstrument(Instrument)).get();
}

unsigned int CFamiTrackerDocWrapper::GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	stChanNote Note;
	m_pDocument->GetNoteData(m_iTrack, Frame, Channel, Row, &Note);
	return Note.EffNumber[Index];
}

unsigned int CFamiTrackerDocWrapper::GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const
{
	stChanNote Note;
	m_pDocument->GetNoteData(m_iTrack, Frame, Channel, Row, &Note);
	return Note.EffParam[Index];
}

int CFamiTrackerDocWrapper::GetSampleCount() const
{
	return m_pDocument->GetSampleCount();
}

void CFamiTrackerDocWrapper::GetSampleName(unsigned int Index, char *Name) const
{
	if (const CDSample *pSamp = m_pDocument->GetSample(Index))		// // //
		strncpy(Name, pSamp->GetName(), CDSample::MAX_NAME_SIZE);
}

int CFamiTrackerDocWrapper::GetSampleSize(unsigned int Sample) const
{
	const CDSample *pSamp = m_pDocument->GetSample(Sample);		// // //
	if (!pSamp) return 0;
	return pSamp->GetSize();
}

char CFamiTrackerDocWrapper::GetSampleData(unsigned int Sample, unsigned int Offset) const
{
	const CDSample *pSamp = m_pDocument->GetSample(Sample);		// // //
	if (!pSamp) return 0;
	return *(pSamp->GetData() + Offset);
}
