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

#include "stdafx.h"
#include "FTMComponentInterface.h"
#include "Instrument.h"
#include "SeqInstrument.h"
#include "InstrumentFactory.h"
#include "InstrumentManager.h"
#include "Sequence.h"
#include "SequenceCollection.h"
#include "SequenceManager.h"
#include "DSampleManager.h"

const int CInstrumentManager::MAX_INSTRUMENTS = 64;
const int CInstrumentManager::SEQ_MANAGER_COUNT = 5;

CInstrumentManager::CInstrumentManager(CFTMComponentInterface *pInterface) :
	m_pDSampleManager(new CDSampleManager()),
	m_pInstruments(MAX_INSTRUMENTS),
	m_pDocInterface(pInterface)
{
	for (int i = 0; i < SEQ_MANAGER_COUNT; i++)
		m_pSequenceManager.push_back(std::unique_ptr<CSequenceManager>(new CSequenceManager(i == 2 ? 3 : SEQ_COUNT)));
}

CInstrumentManager::~CInstrumentManager()
{
	const auto End = m_pInstruments.end();
	for (auto it = m_pInstruments.begin(); it < End; ++it)
		if (*it != nullptr)
			(*it)->RegisterManager(nullptr);
}

//
// Instrument methods
//

std::shared_ptr<CInstrument> CInstrumentManager::GetInstrument(unsigned int Index) const
{
	m_InstrumentLock.Lock();
	auto ptr = m_pInstruments[Index];
	m_InstrumentLock.Unlock();
	return ptr;
}

std::shared_ptr<CInstrument> CInstrumentManager::CreateNew(inst_type_t InstType)
{
	return std::shared_ptr<CInstrument>(CInstrumentFactory::CreateNew(InstType));
}

bool CInstrumentManager::InsertInstrument(unsigned int Index, CInstrument *pInst)
{
	return InsertInstrument(Index, std::shared_ptr<CInstrument>(pInst));
}

bool CInstrumentManager::InsertInstrument(unsigned int Index, std::shared_ptr<CInstrument> pInst)
{
	bool Changed = false;
	m_InstrumentLock.Lock();
	if (m_pInstruments[Index] != pInst) Changed = true;
	m_pInstruments[Index] = pInst;
	pInst->RegisterManager(static_cast<CInstrumentManagerInterface*>(this));
	m_InstrumentLock.Unlock();
	return true;
}

bool CInstrumentManager::RemoveInstrument(unsigned int Index)
{
	bool Changed = false;
	m_InstrumentLock.Lock();
	if (m_pInstruments[Index]) {
		Changed = true;
		m_pInstruments[Index]->RegisterManager(nullptr);
	}
	m_pInstruments[Index].reset();
	m_InstrumentLock.Unlock();
	return Changed;
}

void CInstrumentManager::ClearAll()
{
	const auto End = m_pInstruments.end();
	for (auto it = m_pInstruments.begin(); it < End; ++it) {
		if (*it != nullptr)
			(*it)->RegisterManager(nullptr);
		it->reset();
	}
	for (int i = 0; i < SEQ_MANAGER_COUNT; i++)
		m_pSequenceManager[i].reset(new CSequenceManager(i == 2 ? 3 : SEQ_COUNT));
	m_pDSampleManager.reset(new CDSampleManager());
}

bool CInstrumentManager::IsInstrumentUsed(unsigned int Index) const
{
	return Index < MAX_INSTRUMENTS && m_pInstruments[Index];
}

unsigned int CInstrumentManager::GetInstrumentCount() const
{
	unsigned x = 0;
	for (int i = 0; i < MAX_INSTRUMENTS; i++)
		if (m_pInstruments[i])
			++x;
	return x;
}

unsigned int CInstrumentManager::GetFirstUnused() const
{
	for (int i = 0; i < MAX_INSTRUMENTS; i++)
		if (!m_pInstruments[i])
			return i;
	return -1;
}

int CInstrumentManager::GetFreeSequenceIndex(inst_type_t InstType, int Type, CSeqInstrument *pInst) const
{
	// moved from CFamiTrackerDoc
	std::vector<bool> Used(CSequenceCollection::MAX_SEQUENCES, false);
	for (int i = 0; i < MAX_INSTRUMENTS; i++) if (GetInstrumentType(i) == InstType) {		// // //
		auto pInstrument = std::static_pointer_cast<CSeqInstrument>(GetInstrument(i));
		if (pInstrument->GetSeqEnable(Type) && (pInst && pInst->GetSequence(Type)->GetItemCount() || pInst != pInstrument.get()))
			Used[pInstrument->GetSeqIndex(Type)] = true;
	}
	for (int i = 0; i < CSequenceCollection::MAX_SEQUENCES; ++i) if (!Used[i]) {
		const CSequence *pSeq = GetSequence(InstType, Type, i);
		if (!pSeq || !pSeq->GetItemCount())
			return i;
	}
	return -1;
}

inst_type_t CInstrumentManager::GetInstrumentType(unsigned int Index) const
{
	return !IsInstrumentUsed(Index) ? INST_NONE : m_pInstruments[Index]->GetType();
}

void CInstrumentManager::CloneInstrumentShallow(unsigned int Old, unsigned int New)
{
}

void CInstrumentManager::CloneInstrumentDeep(unsigned int Old, unsigned int New)
{
}

//
// Sequence methods
//

CSequenceManager *const CInstrumentManager::GetSequenceManager(int InstType) const
{
	int Index = -1;
	switch (InstType) {
	case INST_2A03: Index = 0; break;
	case INST_VRC6: Index = 1; break;
	case INST_FDS:  Index = 2; break;
	case INST_N163: Index = 3; break;
	case INST_S5B:  Index = 4; break;
	default: return nullptr;
	}
	return m_pSequenceManager[Index].get();
}

CDSampleManager *const CInstrumentManager::GetDSampleManager() const
{
	return m_pDSampleManager.get();
}

//
// from interface
//

CSequence *CInstrumentManager::GetSequence(int InstType, int SeqType, int Index) const
{
	auto pManager = GetSequenceManager(InstType);
	if (!pManager) return nullptr;
	auto pCol = pManager->GetCollection(SeqType);
	if (!pCol) return nullptr;
	return pCol->GetSequence(Index);
}

void CInstrumentManager::SetSequence(int InstType, int SeqType, int Index, CSequence *pSeq)
{
	if (auto pManager = GetSequenceManager(InstType))
		if (auto pCol = pManager->GetCollection(SeqType))
			pCol->SetSequence(Index, pSeq);
}

int CInstrumentManager::AddSequence(int InstType, int SeqType, CSequence *pSeq, CSeqInstrument *pInst)
{
	int Index = GetFreeSequenceIndex(static_cast<inst_type_t>(InstType), SeqType, pInst);
	if (Index != -1) SetSequence(InstType, SeqType, Index, pSeq);
	return Index;
}

const CDSample *CInstrumentManager::GetDSample(int Index) const
{
	return m_pDSampleManager->GetDSample(Index);
}

void CInstrumentManager::SetDSample(int Index, CDSample *pSamp)
{
	m_pDSampleManager->SetDSample(Index, pSamp);
}

int CInstrumentManager::AddDSample(CDSample *pSamp)
{
	int Index = m_pDSampleManager->GetFirstFree();
	if (Index != -1) SetDSample(Index, pSamp);
	return Index;
}

void CInstrumentManager::InstrumentChanged() const
{
	if (m_pDocInterface)
		m_pDocInterface->ModifyIrreversible();
}
