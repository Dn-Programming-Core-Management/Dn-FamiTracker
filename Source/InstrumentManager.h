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

#include "InstrumentManagerInterface.h"
#include <vector>
#include <memory>
#include <mutex>

class CInstrument;
class CDSample;
class CSequenceManager;
class CDSampleManager;
class CFTMComponentInterface;

enum inst_type_t : int;

/*!
	\brief A container of FamiTracker instruments.
	\details This class implements common facilities for manipulating a fixed-length array of
	instrument objects. 
*/
class CInstrumentManager : public CInstrumentManagerInterface
{
public:
	CInstrumentManager(CFTMComponentInterface *pInterface = nullptr);
	virtual ~CInstrumentManager();

	void ClearAll();

	std::shared_ptr<CInstrument> GetInstrument(unsigned int Index) const;
	bool InsertInstrument(unsigned int Index, std::unique_ptr<CInstrument> pInst);
	bool InsertInstrument(unsigned int Index, std::shared_ptr<CInstrument> pInst);
	bool RemoveInstrument(unsigned int Index);
	void SwapInstruments(unsigned int IndexA, unsigned int IndexB);
	
	bool IsInstrumentUsed(unsigned int Index) const;
	unsigned int GetInstrumentCount() const;
	unsigned int GetFirstUnused() const;
	int GetFreeSequenceIndex(inst_type_t InstType, int Type, CSeqInstrument *pInst = nullptr) const;

	inst_type_t GetInstrumentType(unsigned int Index) const;
	
	void CloneInstrumentShallow(unsigned int Old, unsigned int New);
	void CloneInstrumentDeep(unsigned int Old, unsigned int New);

	CSequenceManager *const GetSequenceManager(int InstType) const;
	CDSampleManager *const GetDSampleManager() const;

	// from interface
	CSequence *GetSequence(int InstType, int SeqType, int Index) const override; // TODO: use SetSequence and provide const getter
	void SetSequence(int InstType, int SeqType, int Index, CSequence *pSeq) override;
	int AddSequence(int InstType, int SeqType, CSequence *pSeq, CSeqInstrument *pInst = nullptr) override;
	const CDSample *GetDSample(int Index) const override;
	void SetDSample(int Index, CDSample *pSamp) override;
	int AddDSample(CDSample *pSamp) override;
	void InstrumentChanged() const override;

public:
	static std::unique_ptr<CInstrument> CreateNew(inst_type_t InstType);
	static const int MAX_INSTRUMENTS;

private:
	std::vector<std::shared_ptr<CInstrument>> m_pInstruments;
	std::vector<std::unique_ptr<CSequenceManager>> m_pSequenceManager;
	std::unique_ptr<CDSampleManager> m_pDSampleManager;

	mutable std::mutex m_InstrumentLock;		// // //
	CFTMComponentInterface *m_pDocInterface;

private:
	static const int SEQ_MANAGER_COUNT;
};
