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

#pragma once


#include "FamiTrackerTypes.h"
#include "PatternNote.h"		// // //

/*
** This class is a pure virtual interface to CSequence, which can be used by custom exporters
*/
class CSequenceInterface
{
public:
	virtual signed char		GetItem(int Index) const = 0;
	virtual unsigned int	GetItemCount() const = 0;
	virtual unsigned int    GetLoopPoint() const = 0;
	// // //
	virtual unsigned int    GetReleasePoint() const = 0;
	virtual unsigned int	GetSetting() const = 0;
};

class CSeqInstrumentInterface		// // //
{
public:
	virtual int GetSeqEnable(int Index) const = 0;
	virtual int GetSeqIndex(int Index) const = 0;
};

class CInstrument2A03Interface
{
public:
	virtual int GetSeqEnable(int Index) const = 0;
	virtual int GetSeqIndex(int Index) const = 0;

	virtual char GetSample(int Octave, int Note) const = 0;
	virtual char GetSamplePitch(int Octave, int Note) const = 0;
	virtual char GetSampleLoopOffset(int Octave, int Note) const = 0;
};

typedef const void* SequenceHandle;
typedef const void* SeqInstrumentHandle;		// // //
typedef const void* Instrument2A03Handle;

//
// Pure virtual interface to famitracker document, to make it simple for custom exporter DLLs
// to interact with the document. Also, all the getters are const so that custom exporters
// cannot screw anything up =)
//

class CFamiTrackerDocInterface
{
public:
	virtual void			  GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const = 0;
	virtual unsigned int	  GetFrameCount()			const = 0;
	virtual unsigned int	  GetPatternLength()		const = 0;
	virtual unsigned int	  GetSongSpeed()			const = 0;
	virtual CSequenceInterface const	  *GetSequence(unsigned int Index, int Type) const = 0;
	virtual int				  GetSequenceCount(int Type) const = 0;
	virtual int               GetInstrumentCount() const = 0;
	virtual CInstrument2A03Interface const *Get2A03Instrument(int Instrument) const = 0;
	virtual unsigned int	GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const = 0;
	virtual unsigned int	GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const = 0;
	virtual int				GetSampleCount() const = 0;
	virtual void			GetSampleName(unsigned int Index, char *Name) const = 0;
	virtual int				GetSampleSize(unsigned int Sample) const = 0;
	virtual char			GetSampleData(unsigned int Sample, unsigned int Offset) const = 0;
	
	virtual CSeqInstrumentInterface const *GetSeqInstrument(int Instrument) const = 0;		// // //
};

//
// Struct containing a set of C function pointers pointing to functions which forward calls to
// the currently loaded Famitracker document. This was added to allow exporters to be written with
// any win32 capable compiler.
// The functions which retrieve sequences and instruments now retrieve a void pointer. This is so
// exporters don't try to make any calls that might use the object's vtable. Instead, several function
// pointers are provided which expect these handles to be passed in in order to forward appropriate
// calls to c++ objects inside Famitracker.
//
struct FamitrackerDocInterface
{
	//overall document functions
	void (__cdecl *GetNoteData)(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data);
	unsigned int (__cdecl *GetFrameCount)();
	unsigned int (__cdecl *GetPatternLength)();
	unsigned int (__cdecl *GetSongSpeed)();

	//sequence functions
	int (__cdecl *GetSequenceCount)(int Type);
	SequenceHandle (__cdecl *GetSequence)(int Index, int Type);

	signed char (__cdecl *GetItem)(SequenceHandle sequence, int Index);
	unsigned int (__cdecl *GetItemCount)(SequenceHandle sequence);
	unsigned int (__cdecl *GetLoopPoint)(SequenceHandle sequence);

	//instrument functions
	int (__cdecl *GetInstrumentCount)();
	Instrument2A03Handle (__cdecl *Get2A03Instrument)(int Instrument);

	int (__cdecl *GetSeqEnable)(SeqInstrumentHandle instrument, int Index);
	int (__cdecl *GetSeqIndex)(SeqInstrumentHandle instrument, int Index);

	//effect functions
	unsigned int (__cdecl *GetNoteEffectType)(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index);
	unsigned int (__cdecl *GetNoteEffectParam)(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index);

	//DPCM functions
	int (__cdecl *GetSampleCount)();
	void (__cdecl *GetSampleName)(unsigned int Index, char *Name);
	int (__cdecl *GetSampleSize)(unsigned int Sample);
	char (__cdecl *GetSampleData)(unsigned int Sample, unsigned int Offset);

	//DPCM instrument functions
	char (__cdecl *GetSample)(Instrument2A03Handle instrument, int Octave, int Note);
	char (__cdecl *GetSamplePitch)(Instrument2A03Handle instrument, int Octave, int Note);
	char (__cdecl *GetSampleLoopOffset)(Instrument2A03Handle instrument, int Octave, int Note);
	
	SeqInstrumentHandle (__cdecl *GetSeqInstrument)(int Instrument);		// // //
};
