/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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

#include "stdafx.h"
#include <vector>
#include <memory>
#include <afxmt.h>
#include "Instrument.h"
#include "InstrumentManager.h"
#include "SequenceManager.h"

const int CInstrumentManager::MAX_INSTRUMENTS = 64;
const int CInstrumentManager::SEQ_MANAGER_COUNT = 5;

CInstrumentManager::CInstrumentManager()
{
	m_pInstruments.resize(MAX_INSTRUMENTS);

	for (int i = 0; i < SEQ_MANAGER_COUNT; i++)
		m_pSequenceManager.push_back(std::unique_ptr<CSequenceManager>(new CSequenceManager(i == 2 ? 3 : SEQ_COUNT)));
}

CInstrumentManager::~CInstrumentManager()
{
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
	return std::shared_ptr<CInstrument>(CInstrument::CreateNew(InstType));
}

bool CInstrumentManager::InsertInstrument(unsigned int Index, CInstrument *pInst)
{
	bool Changed = false;
	m_InstrumentLock.Lock();
	if (m_pInstruments[Index].get() != pInst) Changed = true;
	m_pInstruments[Index].reset(pInst);
	m_InstrumentLock.Unlock();
	return Changed;
}

bool CInstrumentManager::InsertInstrument(unsigned int Index, std::shared_ptr<CInstrument> pInst)
{
	bool Changed = false;
	m_InstrumentLock.Lock();
	if (m_pInstruments[Index] != pInst) Changed = true;
	m_pInstruments[Index] = pInst;
	m_InstrumentLock.Unlock();
	return true;
}

bool CInstrumentManager::RemoveInstrument(unsigned int Index)
{
	bool Changed = false;
	m_InstrumentLock.Lock();
	if (m_pInstruments[Index]) Changed = true;
	m_pInstruments[Index].reset();
	m_InstrumentLock.Unlock();
	return Changed;
}

void CInstrumentManager::ClearAll()
{
	const auto End = m_pInstruments.end();
	for (auto it = m_pInstruments.begin(); it < End; ++it)
		it->reset();
	for (int i = 0; i < SEQ_MANAGER_COUNT; i++)
		m_pSequenceManager[i].reset(new CSequenceManager(i == 2 ? 3 : SEQ_COUNT));
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
