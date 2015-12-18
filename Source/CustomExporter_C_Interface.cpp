#include <afx.h>
#include "CustomExporterInterfaces.h"
#include "CustomExporter_C_Interface.h"

CFamiTrackerDocInterface* _doc = NULL;

//function to tell C interface where to find famitracker doc for call forwarding
void SetDoc(CFamiTrackerDocInterface* doc)
{
	_doc = doc;
}

//function to fill FamitrackerDocInterface object with pointers to all the C interface functions
void GetInterface(FamitrackerDocInterface* iface)
{
	//overall document functions
	iface->GetNoteData = GetNoteData;
	iface->GetFrameCount = GetFrameCount;
	iface->GetPatternLength = GetPatternLength;
	iface->GetSongSpeed = GetSongSpeed;

	//sequence functions
	iface->GetSequenceCount = GetSequenceCount;
	iface->GetSequence = GetSequence;

	iface->GetItem = GetItem;
	iface->GetItemCount = GetItemCount;
	iface->GetLoopPoint = GetLoopPoint;

	//instrument functions
	iface->GetInstrumentCount = GetInstrumentCount;
	iface->Get2A03Instrument = Get2A03Instrument;
	iface->GetSeqInstrument = GetSeqInstrument;		// // //

	iface->GetSeqEnable = GetSeqEnable;
	iface->GetSeqIndex = GetSeqIndex;

	//effect functions
	iface->GetNoteEffectType = GetNoteEffectType;
	iface->GetNoteEffectParam = GetNoteEffectParam;

	//DPCM functions
	iface->GetSampleCount = GetSampleCount;
	iface->GetSampleName = GetSampleName;
	iface->GetSampleSize = GetSampleSize;
	iface->GetSampleData = GetSampleData;

	//DPCM instrument functions
	iface->GetSample = GetSample;
	iface->GetSamplePitch = GetSamplePitch;
	iface->GetSampleLoopOffset = GetSampleLoopOffset;
}

//overall document functions
void GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data)
{
	if (NULL == _doc)
	{
		return;
	}

	_doc->GetNoteData(Frame, Channel, Row, Data);
}

unsigned int GetFrameCount()
{
	if (NULL == _doc)
	{
		return 0;
	}

	return _doc->GetFrameCount();
}

unsigned int GetPatternLength()
{
	if (NULL == _doc)
	{
		return 0;
	}

	return _doc->GetPatternLength();
}

unsigned int GetSongSpeed()
{
	if (NULL == _doc)
	{
		return -1;
	}

	return _doc->GetSongSpeed();
}

//sequence functions
int GetSequenceCount(int Type)
{
	if (NULL == _doc)
	{
		return 0;
	}

	return _doc->GetSequenceCount(Type);
}

SequenceHandle GetSequence(int Index, int Type)
{
	if (NULL == _doc)
	{
		return NULL;
	}
	
	return static_cast<SequenceHandle>(_doc->GetSequence(Index, Type));
}

signed char GetItem(SequenceHandle sequence, int Index)
{
	if (NULL == sequence)
	{
		return 0;
	}

	CSequenceInterface const* sequenceInterface = static_cast<CSequenceInterface const*>(sequence);

	return sequenceInterface->GetItem(Index);
}

unsigned int GetItemCount(SequenceHandle sequence)
{
	if (NULL == sequence)
	{
		return 0;
	}

	CSequenceInterface const* sequenceInterface = static_cast<CSequenceInterface const*>(sequence);

	return sequenceInterface->GetItemCount();
}

unsigned int GetLoopPoint(SequenceHandle sequence)
{
	if (NULL == sequence)
	{
		return 0;
	}

	CSequenceInterface const* sequenceInterface = static_cast<CSequenceInterface const*>(sequence);
	
	return sequenceInterface->GetLoopPoint();
}

//instrument functions
int GetInstrumentCount()
{
	if (NULL == _doc)
	{
		return 0;
	}
	
	return _doc->GetInstrumentCount();
}

Instrument2A03Handle Get2A03Instrument(int Instrument)
{
	if (NULL == _doc)
	{
		return NULL;
	}

	return static_cast<Instrument2A03Handle>(_doc->Get2A03Instrument(Instrument));
}

SeqInstrumentHandle GetSeqInstrument(int Instrument)		// // //
{
	if (NULL == _doc)
	{
		return NULL;
	}

	return static_cast<SeqInstrumentHandle>(_doc->GetSeqInstrument(Instrument));
}

int GetSeqEnable(SeqInstrumentHandle instrument, int Index)		// // //
{
	if (NULL == instrument)
	{
		return 0;
	}

	CSeqInstrumentInterface const* instrumentInterface = static_cast<CSeqInstrumentInterface const*>(instrument);
	
	return instrumentInterface->GetSeqEnable(Index);
}

int GetSeqIndex(SeqInstrumentHandle instrument, int Index)		// // //
{
	if (NULL == instrument)
	{
		return 0;
	}

	CSeqInstrumentInterface const* instrumentInterface = static_cast<CSeqInstrumentInterface const*>(instrument);

	return instrumentInterface->GetSeqIndex(Index);
}

//effect functions
unsigned int GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index)
{
	if (NULL == _doc)
	{
		return -1;
	}

	return _doc->GetNoteEffectType(Frame, Channel, Row, Index);
}

unsigned int GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index)
{
	if (NULL == _doc)
	{
		return -1;
	}

	return _doc->GetNoteEffectParam(Frame, Channel, Row, Index);
}

//DPCM functions
int GetSampleCount()
{
	if (NULL == _doc)
	{
		return -1;
	}

	return _doc->GetSampleCount();
}

void GetSampleName(unsigned int Index, char *Name)
{
	if (NULL == _doc)
	{
		return;
	}

	_doc->GetSampleName(Index, Name);
}

int GetSampleSize(unsigned int Sample)
{
	if (NULL == _doc)
	{
		return -1;
	}

	return _doc->GetSampleSize(Sample);
}

char GetSampleData(unsigned int Sample, unsigned int Offset)
{
	if (NULL == _doc)
	{
		return 0;
	}

	return _doc->GetSampleData(Sample, Offset);
}

//DPCM instrument functions
char GetSample(Instrument2A03Handle instrument, int Octave, int Note)
{
	if (NULL == instrument)
	{
		return 0;
	}

	CInstrument2A03Interface const* instrumentInterface = static_cast<CInstrument2A03Interface const*>(instrument);

	return instrumentInterface->GetSample(Octave, Note);
}

char GetSamplePitch(Instrument2A03Handle instrument, int Octave, int Note)
{
	if (NULL == instrument)
	{
		return 0;
	}

	CInstrument2A03Interface const* instrumentInterface = static_cast<CInstrument2A03Interface const*>(instrument);

	return instrumentInterface->GetSamplePitch(Octave, Note);
}

char GetSampleLoopOffset(Instrument2A03Handle instrument, int Octave, int Note)
{
	if (NULL == instrument)
	{
		return 0;
	}

	CInstrument2A03Interface const* instrumentInterface = static_cast<CInstrument2A03Interface const*>(instrument);

	return instrumentInterface->GetSampleLoopOffset(Octave, Note);
}
