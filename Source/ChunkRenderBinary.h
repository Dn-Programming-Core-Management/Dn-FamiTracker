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

//
// Binary chunk renderers
//

class CDSample;		// // //

// Base class
class CBinaryFileWriter
{
protected:
	CBinaryFileWriter(CFile *pFile);
	void Store(const void *pData, unsigned int Size);
	void Fill(unsigned int Size);
	unsigned int GetWritten() const;
	unsigned int m_iDataWritten;

private:
	CFile		*m_pFile;
};

// Binary data render
class CChunkRenderBinary : public CBinaryFileWriter
{
public:
	CChunkRenderBinary(CFile *pFile);
	void StoreChunks(const std::vector<CChunk*> &Chunks);
	void StoreSamples(const std::vector<const CDSample*> &Samples);

private:
	void StoreChunk(CChunk *pChunk);
	void StoreSample(const CDSample *pDSample);

private:
	int m_iSampleAddress;
};

// NSF render
class CChunkRenderNSF : public CBinaryFileWriter
{
public:
	CChunkRenderNSF(CFile *pFile, unsigned int StartAddr);

	void StoreDriver(const char *pDriver, unsigned int Size);
	void StoreNSFDRV(const char *pNSFDRV, unsigned int Size);
	void StoreChunks(const std::vector<CChunk*> &Chunks);
	void StoreChunksBankswitched(const std::vector<CChunk*> &Chunks);
	void StoreSamples(const std::vector<const CDSample*> &Samples);
	void StoreSamplesBankswitched(const std::vector<const CDSample*> &Samples);
	int  GetBankCount() const;
	int  GetDATAChunkSize() const;

protected:
	void StoreChunk(const CChunk *pChunk);
	void StoreChunkBankswitched(const CChunk *pChunk);
	void StoreSample(const CDSample *pDSample);
	void StoreSampleBankswitched(const CDSample *pDSample);

	int  GetRemainingSize() const;
	void AllocateNewBank();
	int  GetBank() const;
	int	 GetAbsoluteAddr() const;

protected:
	unsigned int m_iStartAddr;
	unsigned int m_iSampleAddr;
};

// NES render
class CChunkRenderNES : public CChunkRenderNSF
{
public:
	CChunkRenderNES(CFile *pFile, unsigned int StartAddr);
	void StoreCaller(const void *pData, unsigned int Size);
};
