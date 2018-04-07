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