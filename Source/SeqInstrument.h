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

#include "Instrument.h"
#include "FamiTrackerTypes.h"

class CSequence;

class CSeqInstrument : public CInstrument		// // //
{
public:
	CSeqInstrument(inst_type_t type);
	CInstrument* Clone() const override;
	void	Store(CDocumentFile *pDocFile) const override;
	bool	Load(CDocumentFile *pDocFile) override;
	void	SaveFile(CSimpleFile *pFile) const override;
	bool	LoadFile(CSimpleFile *pFile, int iVersion) override;
	int		Compile(CChunk *pChunk, int Index) const override;
	bool	CanRelease() const override;

	virtual int		GetSeqEnable(int Index) const;
	virtual int		GetSeqIndex(int Index) const;
	virtual void	SetSeqIndex(int Index, int Value);
	virtual void	SetSeqEnable(int Index, int Value);

	virtual CSequence *GetSequence(int SeqType) const;		// // //
	virtual void	SetSequence(int SeqType, CSequence *pSeq);		// // // register sequence in document

	// static const int SEQUENCE_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};
	virtual const char *GetSequenceName(int Index) const { return nullptr; }		// // //

protected:
	void	OnRegisterManager() override;		// // //
	void	CloneFrom(const CInstrument *pSeq) override;		// // //
	CSeqInstrument *CopySequences(const CSeqInstrument *const src);		// // //
	int		m_iSeqEnable[SEQ_COUNT];
	int		m_iSeqIndex[SEQ_COUNT];
};
