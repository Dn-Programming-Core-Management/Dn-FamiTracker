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

#include <memory>
#include <string>

// NSF file header
struct stNSFHeader {
	unsigned char	Ident[5];
	unsigned char	Version;
	unsigned char	TotalSongs;
	unsigned char	StartSong;
	unsigned short	LoadAddr;
	unsigned short	InitAddr;
	unsigned short	PlayAddr;
	unsigned char	SongName[32];
	unsigned char	ArtistName[32];
	unsigned char	Copyright[32];
	unsigned short	Speed_NTSC;
	unsigned char	BankValues[8];
	unsigned short	Speed_PAL;
	unsigned char	Flags;
	unsigned char	SoundChip;
	unsigned char	NSF2Flags;
	unsigned char	NSFDataLength[3];
};

struct stNSFeHeader {		// // //
	unsigned char	NSFeIdent[4];
	unsigned int	InfoSize;
	unsigned char	InfoIdent[4];
	unsigned short	LoadAddr;
	unsigned short	InitAddr;
	unsigned short	PlayAddr;
	unsigned char	Flags;
	unsigned char	SoundChip;
	unsigned char	TotalSongs;
	unsigned char	StartSong;
	unsigned int	BankSize;
	unsigned char	BankIdent[4];
	unsigned char	BankValues[8];
	unsigned int	RateSize;
	unsigned char	RateIdent[4];
	unsigned short	Speed_NTSC;
	unsigned short	Speed_PAL;
};

struct stNSFeChunk {
	uint8_t Ident[4];
	uint8_t Size[4];
	std::vector<uint8_t> Data;
};

struct stNSFeFooter {		// !! !!
	// DATA chunk is written manually
	stNSFeChunk VRC7;
	stNSFeChunk time;
	stNSFeChunk auth;
	stNSFeChunk tlbl;
	stNSFeChunk text;
	stNSFeChunk mixe;
	// NEND has no data yet
	stNSFeChunk NEND;
};

struct driver_t;
class CChunk;
enum chunk_type_t;
class CDSample;		 // // //
class CFamiTrackerDoc;		// // //
class CSequence;		// // //
class CInstrumentFDS;		// // //

/*
 * Logger class
 */
class CCompilerLog
{
public:
	virtual ~CCompilerLog() {}
	virtual void WriteLog(std::string_view text) = 0;
	virtual void Clear() = 0;
};

/*
 * The compiler
 */
class CCompiler
{
public:
	CCompiler(CFamiTrackerDoc *pDoc, CCompilerLog *pLogger);
	~CCompiler();
	
	void	ExportNSF(LPCTSTR lpszFileName, int MachineType);
	void	ExportNSFE(LPCTSTR lpszFileName, int MachineType);		// // //
	void	ExportNSF2(LPCTSTR lpszFileName, int MachineType);		// // //
	void	ExportNES(LPCTSTR lpszFileName, bool EnablePAL);
	void	ExportBIN(LPCTSTR lpszBIN_File, LPCTSTR lpszDPCM_File, int MachineType, bool ExtraData);
	void	ExportPRG(LPCTSTR lpszFileName, bool EnablePAL);
	void	ExportASM(LPCTSTR lpszFileName, int MachineType, bool ExtraData);

private:
	bool	OpenFile(LPCTSTR lpszFileName, CFile &file) const;

	void	CreateHeader(stNSFHeader *pHeader, int MachineType, unsigned int NSF2Flags, bool NSF2) const;
	void	CreateNSFeHeader(stNSFeHeader *pHeader, int MachineType);		// // //
	void	CreateNSFeFooter(stNSFeFooter *pHeader);		// !! !!
	void	SetDriverSongAddress(char *pDriver, unsigned short Address) const;
#if 0
	void	WriteChannelMap();
	void	WriteChannelTypes();
#endif

	void	PatchVibratoTable(char *pDriver) const;

	char*	LoadDriver(const driver_t *pDriver, unsigned short Origin) const;
	char*	LoadNSFDRV(const driver_t *pDriver) const;

	// Compiler
	bool	CompileData(bool bUseNSFDRV = false, bool UseAllExp = true);
	void	ResolveLabels();
	bool	ResolveLabelsBankswitched();
	void	CollectLabels(CMap<CStringA, LPCSTR, int, int> &labelMap) const;
	bool	CollectLabelsBankswitched(CMap<CStringA, LPCSTR, int, int> &labelMap);
	void	AssignLabels(CMap<CStringA, LPCSTR, int, int> &labelMap);
	void	AddBankswitching();
	void	Cleanup();
	void	CalculateLoadAddresses(unsigned short &MusicDataAddress, bool &bCompressedMode, bool ForceDecompress = false);
	void	SetNSFDRVHeaderSize(bool bUseNSFDRV);

	void	ScanSong();
	int		GetSampleIndex(int SampleNumber);
	bool	IsPatternAddressed(unsigned int Track, int Pattern, int Channel) const;
	bool	IsInstrumentInPattern(int index) const;

	void	CreateMainHeader(bool UseAllExp);
	void	CreateSequenceList();
	void	CreateInstrumentList();
	void	CreateSampleList();
	void	CreateFrameList(unsigned int Track);

	int		StoreSequence(const CSequence *pSeq, CStringA &label);
	void	StoreSamples();
	void	StoreGrooves();		// // //
	void	StoreSongs(bool bUseAllExp = true);
	void	StorePatterns(unsigned int Track, bool bUseAllExp = true);

	// Bankswitching functions
	void	UpdateSamplePointers(unsigned int Origin);
	void	UpdateFrameBanks();
	void	UpdateSongBanks();
	void	ClearSongBanks();
	void	EnableBankswitching();

	// FDS
	void	AddWavetable(CInstrumentFDS *pInstrument, CChunk *pChunk);

	// File writing

	using CFilePtrArray = std::vector<std::unique_ptr<CFile>>;

	void	WriteAssembly(CFilePtrArray &files, bool bExtraData, stNSFHeader Header, int MachineType = 0,
		size_t OutputFileASMIndex = 0,
		size_t FileNSFStubIndex = 0,
		size_t FileNSFHeaderIndex = 0,
		size_t FileNSFConfigIndex = 0,
		size_t FilePeriodsIndex = 0,
		size_t FileVibratoIndex = 0,
		size_t FileMultiChipEnableIndex = 0,
		size_t FileMultiChipUpdateIndex = 0);
	void	WriteBinary(CFilePtrArray &files, bool bExtraData, stNSFHeader Header, int MachineType = 0,
		size_t OutputFileBINIndex = 0,
		size_t FileNSFStubIndex = 0,
		size_t FileNSFHeaderIndex = 0,
		size_t FileNSFConfigIndex = 0,
		size_t FilePeriodsIndex = 0,
		size_t FileVibratoIndex = 0,
		size_t FileMultiChipEnableIndex = 0,
		size_t FileMultiChipUpdateIndex = 0);
	void	WriteSamplesBinary(CFile *pFile);
	void	ReadPeriodVibratoTables(int MachineType, unsigned int *LUTNTSC,
		unsigned int *LUTPAL,
		unsigned int *LUTSaw,
		unsigned int *LUTVRC7,
		unsigned int *LUTFDS,
		unsigned int *LUTN163,
		unsigned int *LUTVibrato) const;
	bool	OpenArrayFile(CFilePtrArray &files, LPCTSTR filepath, std::string_view message);
	void	CloseFileArray(CFilePtrArray &files);

	// Object list functions
	CChunk	*CreateChunk(chunk_type_t Type, CStringA label);
	CChunk	*GetObjectByRef(CStringA label) const;
	int		CountData() const;

	// Debugging
	template <typename... T>
	void	Print(std::string_view text, T... args) const;		// // //
	void	ClearLog() const;

public:
	static const int PATTERN_CHUNK_INDEX;

	static const int PAGE_SIZE;
	static const int PAGE_START;
	static const int PAGE_BANKED;
	static const int PAGE_SAMPLES;

	static const int PATTERN_SWITCH_BANK;

	static const int DPCM_PAGE_WINDOW;
	static const int DPCM_SWITCH_ADDRESS;

	static const bool LAST_BANK_FIXED;

	// Flags
	static const int FLAG_BANKSWITCHED;
	static const int FLAG_VIBRATO;
	static const int FLAG_LINEARPITCH;		// // //
	static const bool UseAllChips;		// !! !!

protected:
	static CCompiler *pCompiler;			// Points to an active CCompiler object

public:
	static CCompiler *GetCompiler();		// Get the active CCompiler object, NULL otherwise

	static unsigned int AdjustSampleAddress(unsigned int Address);

private:
	CFamiTrackerDoc *m_pDocument;

	// Object lists
	std::vector<CChunk*> m_vChunks;
	std::vector<CChunk*> m_vSequenceChunks;
	std::vector<CChunk*> m_vInstrumentChunks;
	std::vector<CChunk*> m_vGrooveChunks;		// // //
	std::vector<CChunk*> m_vSongChunks;
	std::vector<CChunk*> m_vFrameChunks;
	std::vector<CChunk*> m_vPatternChunks;
	//std::vector<CChunk*> m_vWaveChunks;

	// Special objects
	CChunk			*m_pSamplePointersChunk;
	CChunk			*m_pHeaderChunk;

	// Samples
	std::vector<const CDSample*> m_vSamples;

	// Flags
	bool			m_bBankSwitched;

	// Driver
	const driver_t	*m_pDriverData;
	unsigned int	m_iVibratoTableLocation;

	// Sequences and instruments
	unsigned int	m_iInstruments;
	unsigned int	m_iAssignedInstruments[MAX_INSTRUMENTS];
	bool			m_bSequencesUsed2A03[MAX_SEQUENCES][SEQ_COUNT];
	bool			m_bSequencesUsedVRC6[MAX_SEQUENCES][SEQ_COUNT];
	bool			m_bSequencesUsedN163[MAX_SEQUENCES][SEQ_COUNT];
	bool			m_bSequencesUsedS5B[MAX_SEQUENCES][SEQ_COUNT];		// // //

	int				m_iWaveBanks[MAX_INSTRUMENTS];	// N163 waves

	// Sample variables
	unsigned char	m_iSamplesLookUp[MAX_INSTRUMENTS][OCTAVE_RANGE][NOTE_RANGE];
	bool			m_bSamplesAccessed[MAX_INSTRUMENTS][OCTAVE_RANGE][NOTE_RANGE];
	unsigned char	m_iSampleBank[MAX_DSAMPLES];
	unsigned int	m_iSampleStart;
	unsigned int	m_iSamplesUsed;

	// General
	unsigned int	m_iMusicDataSize;		// All music data
	unsigned int	m_iDriverSize;			// Size of selected music driver
	unsigned int	m_iNSFDRVSize;			// Size of NSFDRV header, 0 if it is not written
	unsigned int	m_iSamplesSize;

	unsigned int	m_iLoadAddress;			// NSF load address
	unsigned int	m_iInitAddress;			// NSF init address
	unsigned int	m_iDriverAddress;		// Music driver location

	unsigned int	m_iTrackFrameSize[MAX_TRACKS];	// Cached song frame sizes

	unsigned int	m_iHeaderFlagOffset;	// Offset to flag location in main header
	unsigned int	m_iSongBankReference;	// Offset to bank value in song header

	unsigned int	m_iDuplicatePatterns;	// Number of duplicated patterns removed

	std::vector<int> m_vChanOrder;			// Channel order list
	std::vector<char> m_vChanEnable;		// ft_channel_enable

	// NSF banks
	unsigned int	m_iFirstSampleBank;		// Bank number with the first DPCM sample
	unsigned int	m_iLastBank;			// Last bank in the NSF file

	unsigned int	m_iSamplePointerBank;
	unsigned int	m_iSamplePointerOffset;

	// FDS
	unsigned int	m_iWaveTables;

	// // // Full chip export
	unsigned char	m_iActualChip;
	int				m_iActualNamcoChannels;
	bool			m_bMultiChip;

	// Optimization
	CMap<UINT, UINT, CChunk*, CChunk*> m_PatternMap;
	CMap<CStringA, LPCSTR, CStringA, LPCSTR> m_DuplicateMap;

	// Debugging
	CCompilerLog	*m_pLogger;

	// Diagnostics
	unsigned int	m_iHashCollisions;
};
