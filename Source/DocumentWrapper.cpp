/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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
	CInstrumentContainer<CInstrument2A03> instContainer(m_pDocument, Instrument);		// // //
	CInstrument2A03 *pInstrument = instContainer();
	return dynamic_cast<const CInstrument2A03Interface*>(pInstrument);
}

CSeqInstrumentInterface const *CFamiTrackerDocWrapper::GetSeqInstrument(int Instrument) const		// // //
{
	CInstrumentContainer<CSeqInstrument> instContainer(m_pDocument, Instrument);
	CSeqInstrument *pInstrument = instContainer();
	return dynamic_cast<const CSeqInstrumentInterface*>(pInstrument);
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
	strncpy(Name, m_pDocument->GetSample(Index)->GetName(), CDSample::MAX_NAME_SIZE);
}

int CFamiTrackerDocWrapper::GetSampleSize(unsigned int Sample) const
{
	return m_pDocument->GetSample(Sample)->GetSize();
}

char CFamiTrackerDocWrapper::GetSampleData(unsigned int Sample, unsigned int Offset) const
{
	return *(m_pDocument->GetSample(Sample)->GetData() + Offset);
}
