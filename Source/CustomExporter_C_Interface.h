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

#include "CustomExporterInterfaces.h"

//function to tell C interface where to find famitracker doc for call forwarding
void SetDoc(CFamiTrackerDocInterface* doc);

//function to fill FamitrackerDocInterface object with pointers to all the C interface functions
void GetInterface(FamitrackerDocInterface* iface);

//overall document functions
void GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data);
unsigned int GetFrameCount();
unsigned int GetPatternLength();
unsigned int GetSongSpeed();

//sequence functions
int GetSequenceCount(int Type);
SequenceHandle GetSequence(int Index, int Type);

signed char GetItem(SequenceHandle sequence, int Index);
unsigned int GetItemCount(SequenceHandle sequence);
unsigned int GetLoopPoint(SequenceHandle sequence);

//instrument functions
int GetInstrumentCount();
Instrument2A03Handle Get2A03Instrument(int Instrument);
SeqInstrumentHandle GetSeqInstrument(int Instrument);		// // //

int GetSeqEnable(SeqInstrumentHandle instrument, int Index);
int GetSeqIndex(SeqInstrumentHandle instrument, int Index);

//effect functions
unsigned int GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index);
unsigned int GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index);

//DPCM functions
int GetSampleCount();
void GetSampleName(unsigned int Index, char *Name);
int GetSampleSize(unsigned int Sample);
char GetSampleData(unsigned int Sample, unsigned int Offset);

//DPCM instrument functions
char GetSample(Instrument2A03Handle instrument, int Octave, int Note);
char GetSamplePitch(Instrument2A03Handle instrument, int Octave, int Note);
char GetSampleLoopOffset(Instrument2A03Handle instrument, int Octave, int Note);