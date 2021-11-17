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


#include <vector>		// // //
#include <memory>		// // //

#include "Sequence.h"		// // //
#include "SeqInstrument.h"		// // //

class CInstrumentVRC7 : public CSeqInstrument {		// // //
public:
	CInstrumentVRC7();
	CInstrument* Clone() const;
	void	Setup();
	void	Store(CDocumentFile *pDocFile);
	bool	Load(CDocumentFile *pDocFile);
	void	SaveFile(CInstrumentFile *pFile);
	bool	LoadFile(CInstrumentFile *pFile, int iVersion);
	int		Compile(CChunk *pChunk, int Index);
	bool	CanRelease() const;

public:
	void		 SetPatch(unsigned int Patch);
	unsigned int GetPatch() const;
	void		 SetCustomReg(int Reg, unsigned char Value);		// // //
	unsigned char GetCustomReg(int Reg) const;		// // //

private:
	void StoreSequence(CDocumentFile *pDocFile, const CSequence *pSeq);		// // //
	CSequence *LoadSequence(CDocumentFile *pDocFile) const;		// // //
	void StoreInstSequence(CInstrumentFile *pDocFile, const CSequence *pSeq);		// // //
	CSequence *LoadInstSequence(CInstrumentFile *pFile) const;		// // //

protected:
	virtual void	CloneFrom(const CInstrument *pInst);		// // //

public:
	static const int SEQUENCE_COUNT = 5;		// // //
	static LPCTSTR SEQUENCE_NAME[];		// // //
	LPCTSTR	GetSequenceName(int Index) const { return SEQUENCE_NAME[Index]; }		// // //

private:
	// Instrument data
	std::vector<std::unique_ptr<CSequence>> m_pSequence;		// // //
	unsigned int m_iPatch;
	unsigned char m_iRegs[8];		// // // Custom patch settings

public: // // // porting CSeqInstrument
	virtual int		GetSeqEnable(int Index) const;		// // //
	virtual int		GetSeqIndex(int Index) const;		// // //
	virtual void	SetSeqIndex(int Index, int Value);		// // //
	CSequence* GetSequence(int SeqType) const;		// // //
	virtual void	SetSequence(int SeqType, CSequence* pSeq);		// // //
};
