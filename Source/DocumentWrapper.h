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

#pragma once

// Document wrapper class for custom exporters

#include "FamiTrackerDoc.h"

class CFamiTrackerDocWrapper : public CFamiTrackerDocInterface
{
public:
	CFamiTrackerDocWrapper(CFamiTrackerDoc *pDocument, int iTrack);

	// Export interface implementation
public:
	virtual void			  GetNoteData(unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *Data) const;
	virtual unsigned int	  GetFrameCount()			const;
	virtual unsigned int	  GetPatternLength()		const;
	virtual unsigned int	  GetSongSpeed()			const;
	virtual CSequenceInterface const	  *GetSequence(unsigned int Index, int Type) const;
	virtual int				  GetSequenceCount(int Type) const;
	virtual int               GetInstrumentCount() const;
	virtual CInstrument2A03Interface const *Get2A03Instrument(int Instrument) const;
	virtual CSeqInstrumentInterface const *GetSeqInstrument(int Instrument) const;		// // //
	virtual unsigned int	GetNoteEffectType(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const;
	virtual unsigned int	GetNoteEffectParam(unsigned int Frame, unsigned int Channel, unsigned int Row, int Index) const;
	virtual int				GetSampleCount() const;
	virtual void			GetSampleName(unsigned int Index, char *Name) const;
	virtual int				GetSampleSize(unsigned int Sample) const;
	virtual char			GetSampleData(unsigned int Sample, unsigned int Offset) const;

	// Attributes
private:
	CFamiTrackerDoc *m_pDocument;
	int m_iTrack;
};
