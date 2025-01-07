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

#pragma once

// Document wrapper class for custom exporters

#include "DSample.h"		// // //
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
