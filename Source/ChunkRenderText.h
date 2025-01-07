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
// Text chunk renderer
//

class CChunkRenderText;
class CDSample;		// // //

typedef void (CChunkRenderText:: *renderFunc_t)(CChunk *pChunk, CFile *pFile);

struct stNSFHeader;

struct stChunkRenderFunc {
	chunk_type_t type;
	renderFunc_t function;
};

class CChunkRenderText
{
public:
	CChunkRenderText(CFile *pFile);
	void StoreChunks(const std::vector<CChunk *> &Chunks);
	void StoreSamples(const std::vector<const CDSample *> &Samples, CChunk *pChunk = nullptr);
	void WriteFileString(const CStringA &str, CFile *pFile) const;
	void StoreNSFStub(unsigned char Header, vibrato_t VibratoStyle, bool LinearPitch, int ActualNamcoChannels, bool UseAllChips, bool IsAssembly = false) const;
	void StoreNSFHeader(stNSFHeader Header) const;
	void StoreNSFConfig(unsigned int DPCMSegment, stNSFHeader Header) const;
	void StorePeriods(unsigned int *pLUTNTSC, unsigned int *pLUTPAL, unsigned int *pLUTSaw, unsigned int *pLUTVRC7, unsigned int *pLUTFDS, unsigned int *pLUTN163) const;
	void StoreVibrato(unsigned int *pLUTVibrato) const;
	void StoreUpdateExt(unsigned char Expansion) const;
	void StoreEnableExt(unsigned char Expansion) const;
	void SetExtraDataFiles(CFile *pFileNSFStub, CFile *pFileNSFHeader, CFile *pFileNSFConfig, CFile *pFilePeriods, CFile *pVibrato, CFile *pFileMultiChipEnable, CFile *pFileMultiChipUpdate);
	void SetBankSwitching(bool bBankSwitched = false);

	// Labels
	// // // moved from CCompiler
	static const char LABEL_SONG_LIST[];
	static const char LABEL_INSTRUMENT_LIST[];
	static const char LABEL_SAMPLES_LIST[];
	static const char LABEL_SAMPLES[];
	static const char LABEL_GROOVE_LIST[];		// // //
	static const char LABEL_GROOVE[];		// // //
	static const char LABEL_WAVETABLE[];
	static const char LABEL_SAMPLE[];
	static const char LABEL_WAVES[];
	static const char LABEL_SEQ_2A03[];
	static const char LABEL_SEQ_VRC6[];
	static const char LABEL_SEQ_FDS[];
	static const char LABEL_SEQ_N163[];
	static const char LABEL_SEQ_S5B[];		// // //
	static const char LABEL_INSTRUMENT[];
	static const char LABEL_SONG[];
	static const char LABEL_SONG_FRAMES[];
	static const char LABEL_SONG_FRAME[];
	static const char LABEL_PATTERN[];

private:
	static const stChunkRenderFunc RENDER_FUNCTIONS[];

private:
	void DumpStrings(const CStringA &preStr, const CStringA &postStr, CStringArray &stringArray, CFile *pFile) const;
	void StoreByteString(const char *pData, int Len, CStringA &str, int LineBreak) const;
	void StoreByteString(const CChunk *pChunk, CStringA &str, int LineBreak) const;

private:
	void StoreHeaderChunk(CChunk *pChunk, CFile *pFile);
	void StoreInstrumentListChunk(CChunk *pChunk, CFile *pFile);
	void StoreInstrumentChunk(CChunk *pChunk, CFile *pFile);
	void StoreSequenceChunk(CChunk *pChunk, CFile *pFile);
	void StoreSampleListChunk(CChunk *pChunk, CFile *pFile);
	void StoreSamplePointersChunk(CChunk *pChunk, CFile *pFile);
	void StoreGrooveListChunk(CChunk *pChunk, CFile *pFile);		// // //
	void StoreGrooveChunk(CChunk *pChunk, CFile *pFile);		// // //
	void StoreSongListChunk(CChunk *pChunk, CFile *pFile);
	void StoreSongChunk(CChunk *pChunk, CFile *pFile);
	void StoreFrameListChunk(CChunk *pChunk, CFile *pFile);
	void StoreFrameChunk(CChunk *pChunk, CFile *pFile);
	void StorePatternChunk(CChunk *pChunk, CFile *pFile);
	void StoreWavetableChunk(CChunk *pChunk, CFile *pFile);
	void StoreWavesChunk(CChunk *pChunk, CFile *pFile);
	void StoreMusicBankSegment(unsigned char bank, CStringA &str);
	void StoreDPCMBankSegment(unsigned char bank, CStringA &str);

	// taken from ChunkRenderBinary.cpp

private:
	CStringArray m_headerStrings;
	CStringArray m_instrumentListStrings;
	CStringArray m_instrumentStrings;
	CStringArray m_sequenceStrings;
	CStringArray m_sampleListStrings;
	CStringArray m_samplePointersStrings;
	CStringArray m_grooveListStrings;		// // //
	CStringArray m_grooveStrings;		// // //
	CStringArray m_songListStrings;
	CStringArray m_songStrings;
	CStringArray m_songDataStrings;
	CStringArray m_wavetableStrings;
	CStringArray m_wavesStrings;
	std::vector<CStringA> m_configMemoryAreaStrings;
	std::vector<CStringA> m_configSegmentStrings;

	CFile *m_pFile;
	CFile *m_pFileNSFStub;
	CFile *m_pFileNSFHeader;
	CFile *m_pFileNSFConfig;
	CFile *m_pFilePeriods;
	CFile *m_pFileVibrato;
	CFile *m_pFileMultiChipEnable;
	CFile *m_pFileMultiChipUpdate;

	bool m_bBankSwitched;
	unsigned int m_iDataWritten;
};
