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
