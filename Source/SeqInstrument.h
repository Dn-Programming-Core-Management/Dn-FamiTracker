/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#pragma once


class CSequence;
class CInstrument;
class CSeqInstrumentInterface;

class CSeqInstrument : public CInstrument, public CSeqInstrumentInterface		// // //
{
public:
	CSeqInstrument(inst_type_t type);
	virtual CInstrument* Clone() const;
	virtual void	Setup();
	virtual void	Store(CDocumentFile *pDocFile);
	virtual bool	Load(CDocumentFile *pDocFile);
	virtual void	SaveFile(CInstrumentFile *pFile);
	virtual bool	LoadFile(CInstrumentFile *pFile, int iVersion);
	virtual int		Compile(CChunk *pChunk, int Index);
	virtual bool	CanRelease() const;

	virtual int		GetSeqEnable(int Index) const;
	virtual int		GetSeqIndex(int Index) const;
	virtual void	SetSeqIndex(int Index, int Value);
	virtual void	SetSeqEnable(int Index, int Value);

	virtual CSequence *GetSequence(int SeqType) const;		// // //
	virtual void	SetSequence(int SeqType, CSequence *pSeq);		// // // register sequence in document

	// static const int SEQUENCE_TYPES[] = {SEQ_VOLUME, SEQ_ARPEGGIO, SEQ_PITCH, SEQ_HIPITCH, SEQ_DUTYCYCLE};
	virtual LPCTSTR	GetSequenceName(int Index) const { return nullptr; }		// // //

protected:
	virtual void	CloneFrom(const CInstrument *pSeq);		// // //
	CSeqInstrument *CopySequences(const CSeqInstrument *const src);		// // //
	int		m_iSeqEnable[SEQ_COUNT];
	int		m_iSeqIndex[SEQ_COUNT];
};
