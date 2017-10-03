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

#include "Chunk.h"

/**
 * CChunk - Stores NSF data
 *
 */

CChunk::CChunk(chunk_type_t Type, const std::string &label) : m_iType(Type), m_strLabel(label), m_iBank(0)
{
}

CChunk::~CChunk()
{
	Clear();
}

void CChunk::Clear()
{
	m_vChunkData.clear();
}

chunk_type_t CChunk::GetType() const
{
	return m_iType;
}

const std::string &CChunk::GetLabel() const
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
	return m_vChunkData.size();
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
	m_vChunkData.push_back(std::make_unique<CChunkDataByte>(data));
}

void CChunk::StoreWord(unsigned short data)
{
	m_vChunkData.push_back(std::make_unique<CChunkDataWord>(data));
}

void CChunk::StoreReference(const std::string &refName)
{
	m_vChunkData.push_back(std::make_unique<CChunkDataReference>(refName));
}

void CChunk::StoreBankReference(const std::string &refName, int bank)
{
	m_vChunkData.push_back(std::make_unique<CChunkDataBank>(refName, bank));
}

void CChunk::StoreString(const std::vector<char> &data)
{
	m_vChunkData.push_back(std::make_unique<CChunkDataString>(data));
}

void CChunk::ChangeByte(int index, unsigned char data)
{
	GetChunkData<CChunkDataByte>(index).m_data = data;		// // //
}

void CChunk::SetupBankData(int index, unsigned char bank)
{
	GetChunkData<CChunkDataBank>(index).m_bank = bank;
}

unsigned char CChunk::GetStringData(int index, int pos) const
{
	return GetChunkData<const CChunkDataString>(index).m_vData[pos];
}

const std::vector<char> &CChunk::GetStringData(int index) const
{
	return GetChunkData<const CChunkDataString>(index).m_vData;
}

std::string CChunk::GetDataRefName(int index) const
{	
	auto pChunkData = dynamic_cast<CChunkDataReference *>(m_vChunkData[index].get());		// // //
	return pChunkData ? pChunkData->m_refName : "";
}

void CChunk::UpdateDataRefName(int index, const std::string &name)
{
	if (auto pChunkData = dynamic_cast<CChunkDataReference *>(m_vChunkData[index].get()))		// // //
		pChunkData->m_refName = name;
}

bool CChunk::IsDataReference(int index) const 
{
	return dynamic_cast<CChunkDataReference *>(m_vChunkData[index].get()) != NULL;
}

bool CChunk::IsDataBank(int index) const
{
	return dynamic_cast<CChunkDataBank *>(m_vChunkData[index].get()) != NULL;
}

unsigned int CChunk::CountDataSize() const
{
	// Count sizes of all data items
	int Size = 0;

	for (const auto &pChunk : m_vChunkData)
		Size += pChunk->GetSize();

	return Size;
}

void CChunk::AssignLabels(std::map<std::string, int> &labelMap)		// // //
{
	for (auto &x : m_vChunkData)
		if (auto pChunkData = dynamic_cast<CChunkDataReference *>(x.get()))
			pChunkData->ref = labelMap[pChunkData->m_refName];
}

// // //
template <typename T>
T &CChunk::GetChunkData(std::size_t index) const {
#ifdef _DEBUG		// // //
	return dynamic_cast<T &>(*m_vChunkData.at(index));
#else
	return static_cast<T &>(*m_vChunkData[index]);
#endif
}
