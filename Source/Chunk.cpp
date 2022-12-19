/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2022 D.P.C.M.
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

#include <map>
#include "stdafx.h"
#include "Chunk.h"

/**
 * CChunk - Stores NSF data
 *
 */

CChunk::CChunk(chunk_type_t Type, CStringA label) : m_iType(Type), m_strLabel(label), m_iBank(0)
{
}

CChunk::~CChunk()
{
	Clear();
}

void CChunk::Clear()
{
	for (auto x : m_vChunkData)
		delete x;

	m_vChunkData.clear();
}

chunk_type_t CChunk::GetType() const
{
	return m_iType;
}

LPCSTR CChunk::GetLabel() const
{
	return m_strLabel;
}

void CChunk::SetBank(unsigned char Bank)
{
	m_iBank = Bank;
}

unsigned char CChunk::GetBank() const
{
	return m_iBank;
}

int CChunk::GetLength() const
{
	// Return number of data items in the collection
	return static_cast<int>(m_vChunkData.size());
}

unsigned short CChunk::GetData(int index) const
{
	return m_vChunkData[index]->GetData();
}

unsigned short CChunk::GetDataSize(int index) const
{
	return m_vChunkData[index]->GetSize();
}

void CChunk::StoreByte(unsigned char data)
{
	m_vChunkData.push_back(new CChunkDataByte(data));
}

void CChunk::StoreWord(unsigned short data)
{
	m_vChunkData.push_back(new CChunkDataWord(data));
}

void CChunk::StoreReference(CStringA refName)
{
	m_vChunkData.push_back(new CChunkDataReference(refName));
}

void CChunk::StoreBankReference(CStringA refName, int bank)
{
	m_vChunkData.push_back(new CChunkDataBank(refName, bank));
}

void CChunk::StoreString(const std::vector<char> &data)
{
	m_vChunkData.push_back(new CChunkDataString(data));
}

void CChunk::ChangeByte(int index, unsigned char data)
{
	ASSERT(index < (int)m_vChunkData.size());
	static_cast<CChunkDataByte*>(m_vChunkData[index])->m_data = data;
}

void CChunk::SetupBankData(int index, unsigned char bank)
{
	ASSERT(index < (int)m_vChunkData.size());
	static_cast<CChunkDataBank*>(m_vChunkData[index])->m_bank = bank;
}

unsigned char CChunk::GetStringData(int index, int pos) const
{
	return static_cast<CChunkDataString*>(m_vChunkData[index])->m_vData[pos];
}

const std::vector<char> &CChunk::GetStringData(int index) const
{
	return (static_cast<CChunkDataString*>(m_vChunkData[index]))->m_vData;
}

LPCSTR CChunk::GetDataRefName(int index) const
{	
	CChunkDataReference *pChunkData = dynamic_cast<CChunkDataReference*>(m_vChunkData[index]);

	if (pChunkData != NULL)
		return pChunkData->m_refName;

	return "";
}

void CChunk::UpdateDataRefName(int index, CStringA &name)
{
	CChunkDataReference *pChunkData = dynamic_cast<CChunkDataReference*>(m_vChunkData[index]);

	if (pChunkData != NULL)
		pChunkData->m_refName = name;
}

bool CChunk::IsDataReference(int index) const 
{
	return dynamic_cast<CChunkDataReference*>(m_vChunkData[index]) != NULL;
}

bool CChunk::IsDataBank(int index) const
{
	return dynamic_cast<CChunkDataBank*>(m_vChunkData[index]) != NULL;
}

unsigned int CChunk::CountDataSize() const
{
	// Count sizes of all data items
	int Size = 0;

	for (const auto pChunk : m_vChunkData)
		Size += pChunk->GetSize();

	return Size;
}

void CChunk::AssignLabels(CMap<CStringA, LPCSTR, int, int> &labelMap)
{
	for (auto x : m_vChunkData) if (auto pChunkData = dynamic_cast<CChunkDataReference*>(x))
		pChunkData->ref = labelMap[pChunkData->m_refName];
}
