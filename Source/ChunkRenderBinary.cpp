/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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
#include <vector>
#include <functional>
#include <algorithm>
#include "stdafx.h"
#include "FamiTrackerTypes.h"		// // //
#include "Compiler.h"
#include "Chunk.h"
#include "ChunkRenderBinary.h"
#include "DSample.h"		// // //

/**
 * Binary file writer, base class binary renderers
 */

CBinaryFileWriter::CBinaryFileWriter(CFile *pFile) : m_pFile(pFile), m_iDataWritten(0)
{
}

void CBinaryFileWriter::Store(const void *pData, unsigned int Size)
{
	m_pFile->Write(pData, Size);
	m_iDataWritten += Size;
}

void CBinaryFileWriter::Fill(unsigned int Size)
{
	char d = 0;
	for (unsigned int i = 0; i < Size; ++i)
		m_pFile->Write(&d, 1);
	m_iDataWritten += Size;
}

unsigned int CBinaryFileWriter::GetWritten() const
{
	return m_iDataWritten;
}

/**
 * Binary chunk render, used to write binary files
 *
 */

CChunkRenderBinary::CChunkRenderBinary(CFile *pFile) : CBinaryFileWriter(pFile), m_iSampleAddress(0)
{
}

void CChunkRenderBinary::StoreChunks(const std::vector<CChunk*> &Chunks) 
{
	std::for_each(Chunks.begin(), Chunks.end(), [this] (CChunk *pChunk) {
		StoreChunk(pChunk);
	});
}

void CChunkRenderBinary::StoreSamples(const std::vector<const CDSample*> &Samples)
{
	std::for_each(Samples.begin(), Samples.end(), [this] (const CDSample *pSample) {
		StoreSample(pSample);
	});
}

void CChunkRenderBinary::StoreChunk(CChunk *pChunk)
{
	for (int i = 0; i < pChunk->GetLength(); ++i) {
		if (pChunk->GetType() == CHUNK_PATTERN) {
			const std::vector<char> &vec = pChunk->GetStringData(CCompiler::PATTERN_CHUNK_INDEX);
			Store(&vec.front(), static_cast<unsigned int>(vec.size()));
		}
		else {
			unsigned short data = pChunk->GetData(i);
			unsigned short size = pChunk->GetDataSize(i);
			Store(&data, size);
		}
	}
}

void CChunkRenderBinary::StoreSample(const CDSample *pDSample)
{
	unsigned int SampleSize = pDSample->GetSize();

	Store(pDSample->GetData(), SampleSize);
	m_iSampleAddress += SampleSize;

	// Adjust size
	if ((m_iSampleAddress & 0x3F) > 0) {
		int PadSize = 0x40 - (m_iSampleAddress & 0x3F);
		m_iSampleAddress += PadSize;
		Fill(PadSize);
	}
}


/**
 * NSF chunk render, used to write NSF files
 *
 */

CChunkRenderNSF::CChunkRenderNSF(CFile *pFile, unsigned int StartAddr) : 
	CBinaryFileWriter(pFile),
	m_iStartAddr(StartAddr),
	m_iSampleAddr(0)
{
}

void CChunkRenderNSF::StoreDriver(const char *pDriver, unsigned int Size)
{
	// Store NSF driver
	Store(pDriver, Size);
}

void CChunkRenderNSF::StoreNSFDRV(const char* pNSFDRV, unsigned int Size)
{
	Store(pNSFDRV, Size);
}

void CChunkRenderNSF::StoreChunks(const std::vector<CChunk*> &Chunks)
{
	// Store chunks into NSF banks
	std::for_each(Chunks.begin(), Chunks.end(), [this] (CChunk *pChunk) {
		StoreChunk(pChunk);
	});
}

void CChunkRenderNSF::StoreChunksBankswitched(const std::vector<CChunk*> &Chunks)
{
	// Store chunks into NSF banks with bankswitching
	std::for_each(Chunks.begin(), Chunks.end(), [this] (CChunk *pChunk) {
		StoreChunkBankswitched(pChunk);
	});
}

void CChunkRenderNSF::StoreSamples(const std::vector<const CDSample*> &Samples)
{
	// Align samples to $C000
	while (GetAbsoluteAddr() < CCompiler::PAGE_SAMPLES)
		AllocateNewBank();

	// Align first sample to valid address
	int SampleAlign = CCompiler::AdjustSampleAddress(GetAbsoluteAddr());
	Fill(SampleAlign);
	std::for_each(Samples.begin(), Samples.end(), [this] (const CDSample *pSample) {
		StoreSample(pSample);
	});
}

void CChunkRenderNSF::StoreSamplesBankswitched(const std::vector<const CDSample*> &Samples)
{
	// Start samples on a clean bank
	if ((GetAbsoluteAddr() & 0xFFF) != 0)
		AllocateNewBank();

	m_iSampleAddr = CCompiler::PAGE_SAMPLES;
	std::for_each(Samples.begin(), Samples.end(), [this] (const CDSample *pSample) {
		StoreSampleBankswitched(pSample);
	});
}

void CChunkRenderNSF::StoreSample(const CDSample *pDSample)
{
	// Store sample and fill with zeros
	Store(pDSample->GetData(), pDSample->GetSize());
	int PadSample = CCompiler::AdjustSampleAddress(GetAbsoluteAddr());
	Fill(PadSample);
}

void CChunkRenderNSF::StoreSampleBankswitched(const CDSample *pDSample)
{
	unsigned int SampleSize = pDSample->GetSize();

	if (m_iSampleAddr + SampleSize >= (unsigned)CCompiler::DPCM_SWITCH_ADDRESS) {
		// Allocate new bank
		if (GetRemainingSize() != 0x1000)	// Skip if already on beginning of new bank
			AllocateNewBank();
		m_iSampleAddr = CCompiler::PAGE_SAMPLES;
	}

	int Align = CCompiler::AdjustSampleAddress(m_iSampleAddr + SampleSize);
	Store(pDSample->GetData(), SampleSize);
	Fill(Align);
	m_iSampleAddr += SampleSize + Align;
}

int CChunkRenderNSF::GetBankCount() const
{
	return GetBank() + 1;
}

int CChunkRenderNSF::GetDATAChunkSize() const
{
	return GetWritten();
}

void CChunkRenderNSF::StoreChunkBankswitched(const CChunk *pChunk)
{
	switch (pChunk->GetType()) {			
		case CHUNK_FRAME_LIST:
		case CHUNK_FRAME:
		case CHUNK_PATTERN:
			// Switchable data
			while ((GetBank() + 1) <= pChunk->GetBank() && pChunk->GetBank() > CCompiler::PATTERN_SWITCH_BANK)
				AllocateNewBank();
	}

	// Write chunk
	StoreChunk(pChunk);
}

void CChunkRenderNSF::StoreChunk(const CChunk *pChunk)
{
	for (int i = 0; i < pChunk->GetLength(); ++i) {
		if (pChunk->GetType() == CHUNK_PATTERN) {
			const std::vector<char> &vec = pChunk->GetStringData(CCompiler::PATTERN_CHUNK_INDEX);
			Store(&vec.front(), static_cast<unsigned int>(vec.size()));
		}
		else {
			unsigned short data = pChunk->GetData(i);
			unsigned short size = pChunk->GetDataSize(i);
			Store(&data, size);
		}
	}
}

int CChunkRenderNSF::GetRemainingSize() const
{
	// Return remaining bank size
	return 0x1000 - (GetWritten() & 0xFFF);
}

void CChunkRenderNSF::AllocateNewBank()
{
	// Get new NSF bank
	int Remaining = GetRemainingSize();
	Fill(Remaining);
}

int CChunkRenderNSF::GetBank() const
{
	return GetWritten() >> 12;
}

int CChunkRenderNSF::GetAbsoluteAddr() const
{
	// Return NSF address
	return m_iStartAddr + GetWritten();
}


/**
 * NES chunk render, borrows from NSF render
 *
 */

CChunkRenderNES::CChunkRenderNES(CFile *pFile, unsigned int StartAddr) : CChunkRenderNSF(pFile, StartAddr)
{
}

void CChunkRenderNES::StoreCaller(const void *pData, unsigned int Size)
{
	while (GetBank() < 7)
		AllocateNewBank();

	int FillSize = (0x10000 - GetAbsoluteAddr()) - Size;
	Fill(FillSize);
	Store(pData, Size);
}
