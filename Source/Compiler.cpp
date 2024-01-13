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
#include "stdafx.h"
#include "version.h"		// // //
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"		// // //
#include "InstrumentFDS.h"		// // //
#include "InstrumentN163.h"		// // //
#include "PatternCompiler.h"
#include "DSample.h"		// // //
#include "Compiler.h"
#include "Chunk.h"
#include "ChunkRenderText.h"
#include "ChunkRenderBinary.h"
#include "Driver.h"
#include "SoundGen.h"
#include "APU/APU.h"

//
// This is the new NSF data compiler, music is compiled to an object list instead of a binary chunk
//
// The list can be translated to both a binary chunk and an assembly file
// 

/*
 * TODO:
 *  - Remove duplicated FDS waves
 *  - Remove the bank value in CHUNK_SONG??
 *  - Derive classes for each output format instead of separate functions
 *  - Create a config file for NSF driver optimizations
 *  - Pattern hash collisions prevents detecting similar patterns, fix that
 *  - Add bankswitching schemes for other memory mappers
 *
 */

/*
 * Notes:
 *
 *  - DPCM samples and instruments is currently stored as a linear list,
 *    which currently limits the number of possible DPCM configurations
 *    to 127.
 *  - Instrument data is non bankswitched, it might be possible to create
 *    instrument data of a size that makes export impossible.
 *
 */

/*
 * Bankswitched file layout:
 *
 * - $8000 - $AFFF: Music driver and song data (instruments, frames & patterns, unpaged)
 * - $B000 - $BFFF: Swichted part of song data (frames + patterns, 1 page only)
 * - $C000 - $EFFF: Samples (3 pages)
 * - $F000 - $FFFF: Fixed to last bank for compatibility with TNS HFC carts
 *
 * Non-bankswitched, compressed layout:
 *
 * - Music data, driver, DPCM samples
 * 
 * Non-bankswitched + bankswitched, default layout:
 *
 * - Driver, music data, DPCM samples
 *
 */

// Note: Each CCompiler object may only be used once (fix this)

// Remove duplicated patterns (default on)
#define REMOVE_DUPLICATE_PATTERNS

// Don't remove patterns across different tracks (default off)
//#define LOCAL_DUPLICATE_PATTERN_REMOVAL

// Enable bankswitching on all songs (default off)
//#define FORCE_BANKSWITCH

// // //
#define DATA_HEADER_SIZE 8

const int CCompiler::PATTERN_CHUNK_INDEX		= 0;		// Fixed at 0 for the moment

const int CCompiler::PAGE_SIZE					= 0x1000;
const int CCompiler::PAGE_START					= 0x8000;
const int CCompiler::PAGE_BANKED				= 0xB000;	// 0xB000 -> 0xBFFF
const int CCompiler::PAGE_SAMPLES				= 0xC000;

const int CCompiler::PATTERN_SWITCH_BANK		= 3;		// 0xB000 -> 0xBFFF

const int CCompiler::DPCM_PAGE_WINDOW			= 3;		// Number of switchable pages in the DPCM area
const int CCompiler::DPCM_SWITCH_ADDRESS		= 0xF000;	// Switch to new banks when reaching this address

const bool CCompiler::LAST_BANK_FIXED			= true;		// Fix for TNS carts

// Flag byte flags
const int CCompiler::FLAG_BANKSWITCHED	= 1 << 0;
const int CCompiler::FLAG_VIBRATO		= 1 << 1;
const int CCompiler::FLAG_LINEARPITCH	= 1 << 2;		// // //

CCompiler *CCompiler::pCompiler = NULL;

CCompiler *CCompiler::GetCompiler()
{
	return pCompiler;
}

unsigned int CCompiler::AdjustSampleAddress(unsigned int Address)
{
	// Align samples to 64-byte pages
	return (0x40 - (Address & 0x3F)) & 0x3F;
}

// CCompiler

CCompiler::CCompiler(CFamiTrackerDoc *pDoc, CCompilerLog *pLogger) : 
	m_pDocument(pDoc), 
	m_pLogger(pLogger),
	m_iWaveTables(0),
	m_pSamplePointersChunk(NULL),
	m_pHeaderChunk(NULL),
	m_pDriverData(NULL),
	m_iLastBank(0),
	m_iHashCollisions(0),
	m_iFirstSampleBank(0)
{
	ASSERT(CCompiler::pCompiler == NULL);
	CCompiler::pCompiler = this;

	m_iActualChip = m_pDocument->GetExpansionChip();		// // //
	m_iActualNamcoChannels = m_pDocument->GetNamcoChannels();
}

CCompiler::~CCompiler()
{
	CCompiler::pCompiler = NULL;

	Cleanup();

	SAFE_RELEASE(m_pLogger);
}

template <typename... T>
void CCompiler::Print(LPCTSTR text, T... args) const		// // //
{
	static TCHAR buf[256];

	if (m_pLogger == NULL || !text)
		return;

	_sntprintf_s(buf, sizeof(buf), _TRUNCATE, text, args...);

	size_t len = _tcslen(buf);

	if (buf[len - 1] == '\n' && len < (sizeof(buf) - 1)) {
		buf[len - 1] = '\r';
		buf[len] = '\n';
		buf[len + 1] = 0;
	}

	m_pLogger->WriteLog(buf);
}

void CCompiler::ClearLog() const
{
	if (m_pLogger != NULL)
		m_pLogger->Clear();
}

bool CCompiler::OpenFile(LPCTSTR lpszFileName, CFile &file) const
{
	CFileException ex;

	if (!file.Open(lpszFileName, CFile::modeWrite | CFile::modeCreate, &ex)) {
		// Display formatted file exception message
		TCHAR szCause[255];
		CString strFormatted;
		ex.GetErrorMessage(szCause, 255);
		AfxFormatString1(strFormatted, IDS_OPEN_FILE_ERROR, szCause);
		theApp.DisplayMessage(strFormatted, MB_OK | MB_ICONERROR);
		return false;
	}

	return true;
}

void CCompiler::ExportNSF(LPCTSTR lpszFileName, int MachineType)
{
	ClearLog();

	bool nsfewarning = false;

	for (int i = 0; i < 8; i++) {
		nsfewarning |= m_pDocument->GetLevelOffset(i) != 0;
	}

	nsfewarning |= m_pDocument->GetExternalOPLLChipCheck();

	if (nsfewarning) {
		Print(_T("Warning: NSFe optional metadata will not be exported in this format!\n"));
		theApp.DisplayMessage(_T("NSFe optional metadata will not be exported in this format!"), 0, 0);
	}

	// Build the music data
	if (!CompileData()) {
		// Failed
		Cleanup();
		return;
	}

	if (m_bBankSwitched) {
		// Expand and allocate label addresses
		AddBankswitching();
		if (!ResolveLabelsBankswitched()) {
			Cleanup();
			return;
		}
		// Write bank data
		UpdateFrameBanks();
		UpdateSongBanks();
		// Make driver aware of bankswitching
		EnableBankswitching();
	}
	else {
		ResolveLabels();
		ClearSongBanks();
	}

	// Rewrite DPCM sample pointers
	UpdateSamplePointers(m_iSampleStart);

	// Compressed mode means that driver and music is located just below the sample space, no space is lost even when samples are used
	bool bCompressedMode;
	unsigned short MusicDataAddress;

	// Find out load address
	if ((PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize) < 0x8000 || m_bBankSwitched || m_iActualChip != m_pDocument->GetExpansionChip()) // // //
		bCompressedMode = false;
	else
		bCompressedMode = true;
	
	if (bCompressedMode) {
		// Locate driver at $C000 - (driver size)
		m_iLoadAddress = PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize;
		m_iDriverAddress = PAGE_SAMPLES - m_iDriverSize;
		MusicDataAddress = m_iLoadAddress;
	}
	else {
		// Locate driver at $8000
		m_iLoadAddress = PAGE_START;
		m_iDriverAddress = PAGE_START;
		MusicDataAddress = m_iLoadAddress + m_iDriverSize;
	}

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + DATA_HEADER_SIZE;		// // //

	// Load driver
	std::unique_ptr<char[]> pDriverPtr(LoadDriver(m_pDriverData, m_iDriverAddress));		// // //
	char *pDriver = pDriverPtr.get();

	// Patch driver binary
	PatchVibratoTable(pDriver);

	// Copy the Namco table, if used
	// // // nothing here, ft_channel_type is taken care in LoadDriver

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	// Open output file
	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		Print(_T("Error: Could not open output file\n"));
		Cleanup();
		return;
	}

	// Create NSF header
	stNSFHeader Header;
	CreateHeader(&Header, MachineType, 0x00, false);

	// Write header
	OutputFile.Write(&Header, sizeof(stNSFHeader));

	// Write NSF data
	std::unique_ptr<CChunkRenderNSF> Render(new CChunkRenderNSF(&OutputFile, m_iLoadAddress));

	if (m_bBankSwitched) {
		Render->StoreDriver(pDriver, m_iDriverSize);
		Render->StoreChunksBankswitched(m_vChunks);
		Render->StoreSamplesBankswitched(m_vSamples);
	}
	else {
		if (bCompressedMode) {
			Render->StoreChunks(m_vChunks);
			Render->StoreDriver(pDriver, m_iDriverSize);
			Render->StoreSamples(m_vSamples);
		}
		else {
			Render->StoreDriver(pDriver, m_iDriverSize);
			Render->StoreChunks(m_vChunks);
			Render->StoreSamples(m_vSamples);
		}
	}

	// Writing done, print some stats
	Print(_T(" * NSF load address: $%04X\n"), m_iLoadAddress);
	Print(_T("Writing output file...\n"));
	Print(_T(" * Driver size: %i bytes\n"), m_iDriverSize);

	if (m_bBankSwitched) {
		int Percent = (100 * m_iMusicDataSize) / (0x80000 - m_iDriverSize - m_iSamplesSize);
		int Banks = Render->GetBankCount();
		Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);
		Print(_T(" * NSF type: Bankswitched (%i banks)\n"), Banks - 1);
	}
	else {
		int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize - m_iSamplesSize);
		Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);
		Print(_T(" * NSF type: Linear (driver @ $%04X)\n"), m_iDriverAddress);
	}

	Print(_T("Done, total file size: %i bytes\n"), OutputFile.GetLength());

	// Done
	OutputFile.Close();

	Cleanup();
}

void CCompiler::ExportNSFE(LPCTSTR lpszFileName, int MachineType)		// // //
{
	ClearLog();

	// Build the music data
	if (!CompileData()) {
		// Failed
		Cleanup();
		return;
	}

	if (m_bBankSwitched) {
		// Expand and allocate label addresses
		AddBankswitching();
		if (!ResolveLabelsBankswitched()) {
			Cleanup();
			return;
		}
		// Write bank data
		UpdateFrameBanks();
		UpdateSongBanks();
		// Make driver aware of bankswitching
		EnableBankswitching();
	}
	else {
		ResolveLabels();
		ClearSongBanks();
	}

	// Rewrite DPCM sample pointers
	UpdateSamplePointers(m_iSampleStart);

	// Compressed mode means that driver and music is located just below the sample space, no space is lost even when samples are used
	bool bCompressedMode;
	unsigned short MusicDataAddress;

	// Find out load address
	if ((PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize) < 0x8000 || m_bBankSwitched || m_iActualChip != m_pDocument->GetExpansionChip()) // // //
		bCompressedMode = false;
	else
		bCompressedMode = true;
	
	if (bCompressedMode) {
		// Locate driver at $C000 - (driver size)
		m_iLoadAddress = PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize;
		m_iDriverAddress = PAGE_SAMPLES - m_iDriverSize;
		MusicDataAddress = m_iLoadAddress;
	}
	else {
		// Locate driver at $8000
		m_iLoadAddress = PAGE_START;
		m_iDriverAddress = PAGE_START;
		MusicDataAddress = m_iLoadAddress + m_iDriverSize;
	}

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + DATA_HEADER_SIZE;		// // //

	// Load driver
	std::unique_ptr<char[]> pDriverPtr(LoadDriver(m_pDriverData, m_iDriverAddress));		// // //
	char *pDriver = pDriverPtr.get();

	// Patch driver binary
	PatchVibratoTable(pDriver);

	// Copy the Namco table, if used
	// // // nothing here, ft_channel_type is taken care in LoadDriver

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	// Open output file
	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		Print(_T("Error: Could not open output file\n"));
		Cleanup();
		return;
	}

	// // // Create NSFe header

	// Write NSFE, INFO, and BANK chunks
	stNSFeHeader Header;
	CreateNSFeHeader(&Header, MachineType);
	OutputFile.Write(&Header, sizeof(Header));

	// TODO write NSF2 chunk?

	std::size_t
		iVRC7Size = 0,
		iAuthSize = 0,
		iTimeSize = 0,
		iTlblSize = 0,
		iDataSize = 0,
		iTextSize = 0,
		iMixeSize = 0;

	for (unsigned int i = 0; i < m_pDocument->GetTrackCount(); i++) {
		iTimeSize += 4;
		iTlblSize += strlen(m_pDocument->GetTrackTitle(i)) + 1;
	}

	// write VRC7 chunk
	// YM2413 and YMF281B are considered external OPLL
	uint8_t extOPLL = m_pDocument->GetExternalOPLLChipCheck() || (theApp.GetSettings()->Emulation.iVRC7Patch > 6); iVRC7Size++;
	uint8_t patchset[19 * 8];
	if (extOPLL)
		for (int i = 0; i < 19; i++)
			for (int j = 0; j < 8; j++) {
				patchset[(8 * i) + j] = m_pDocument->GetOPLLPatchByte((8 * i) + j);
				iVRC7Size++;
			}

	const unsigned char VRC7Ident[] = { 'V', 'R', 'C', '7' };
	OutputFile.Write(reinterpret_cast<char*>(&iVRC7Size), sizeof(int));
	OutputFile.Write(&VRC7Ident, sizeof(VRC7Ident));
	OutputFile.Write(&extOPLL, sizeof(uint8_t));
	if (extOPLL)
		OutputFile.Write(&patchset, (sizeof(uint8_t) * 19 * 8));

	// write time chunk
	const unsigned char TimeIdent[] = {'t', 'i', 'm', 'e'};
	OutputFile.Write(reinterpret_cast<char*>(&iTimeSize), sizeof(int));
	OutputFile.Write(&TimeIdent, sizeof(TimeIdent));
	for (unsigned int i = 0; i < m_pDocument->GetTrackCount(); i++) {
		int t = static_cast<int>(m_pDocument->GetStandardLength(i, 1) * 1000.0 + 0.5);
		OutputFile.Write(reinterpret_cast<char*>(&t), sizeof(int));
	}

	//  write tlbl chunk
	const unsigned char TlblIdent[] = {'t', 'l', 'b', 'l'};
	OutputFile.Write(reinterpret_cast<char*>(&iTlblSize), sizeof(int));
	OutputFile.Write(&TlblIdent, sizeof(TlblIdent));
	for (unsigned int i = 0; i < m_pDocument->GetTrackCount(); i++) {
		OutputFile.Write(m_pDocument->GetTrackTitle(i), static_cast<UINT>(strlen(m_pDocument->GetTrackTitle(i)) + 1));
	}

	// write auth chunk
	CString ripper = _T(APP_NAME_VERSION);
	iAuthSize = strlen(m_pDocument->GetSongName()) + strlen(m_pDocument->GetSongArtist())
		+ strlen(m_pDocument->GetSongCopyright()) + ripper.GetLength() + 4;

	const unsigned char AuthIdent[] = { 'a', 'u', 't', 'h' };
	OutputFile.Write(reinterpret_cast<char*>(&iAuthSize), sizeof(int));
	OutputFile.Write(&AuthIdent, sizeof(AuthIdent));
	OutputFile.Write(m_pDocument->GetSongName(), static_cast<UINT>(strlen(m_pDocument->GetSongName()) + 1));
	OutputFile.Write(m_pDocument->GetSongArtist(), static_cast<UINT>(strlen(m_pDocument->GetSongArtist()) + 1));
	OutputFile.Write(m_pDocument->GetSongCopyright(), static_cast<UINT>(strlen(m_pDocument->GetSongCopyright()) + 1));
	OutputFile.Write((char*)((PCSTR)ripper), ripper.GetLength() + 1);
	ripper.ReleaseBuffer();

	// write text chunk, if available
	if (strlen(m_pDocument->GetComment())) {
		const unsigned char TextIdent[] = { 't', 'e', 'x', 't' };
		iTextSize = strlen(m_pDocument->GetComment()) + 1;
		OutputFile.Write(reinterpret_cast<char*>(&iTextSize), sizeof(int));
		OutputFile.Write(&TextIdent, sizeof(TextIdent));
		OutputFile.Write(m_pDocument->GetComment(), static_cast<UINT>(strlen(m_pDocument->GetComment()) + 1));
	}

	// write mixe chunk
	const unsigned char MixeIdent[] = { 'm', 'i', 'x', 'e' };

	int16_t mixe_device[8]{};

	CSoundGen* pSoundGen = theApp.GetSoundGenerator();

	for (int i = 0; i < 8; i++) {
		if (m_pDocument->GetLevelOffset(i)) {
			mixe_device[i] = (m_pDocument->GetLevelOffset(i) * 10) + pSoundGen->SurveyMixLevels[i];
			iMixeSize += 3;
		}
	}
	OutputFile.Write(reinterpret_cast<char*>(&iMixeSize), sizeof(int));
	OutputFile.Write(&MixeIdent, sizeof(MixeIdent));
	for (uint8_t i = 0; i < 8; i++) {
		if (m_pDocument->GetLevelOffset(i)) {
			unsigned char devicebyte = 0;
			OutputFile.Write(&i, sizeof(uint8_t));
			OutputFile.Write(reinterpret_cast<char*>(&mixe_device[i]), sizeof(int16_t));
		}
	}

	// Write NSF data
	std::unique_ptr<CChunkRenderNSF> Render(new CChunkRenderNSF(&OutputFile, m_iLoadAddress));

	// write DATA chunk
	ULONGLONG iDataSizePos = OutputFile.GetPosition();
	const unsigned char DataIdent[] = {'D', 'A', 'T', 'A'};
	OutputFile.Write(reinterpret_cast<char*>(&iDataSize), sizeof(int));
	OutputFile.Write(&DataIdent, sizeof(DataIdent));

	if (m_bBankSwitched) {
		Render->StoreDriver(pDriver, m_iDriverSize);
		Render->StoreChunksBankswitched(m_vChunks);
		Render->StoreSamplesBankswitched(m_vSamples);
	}
	else {
		if (bCompressedMode) {
			Render->StoreChunks(m_vChunks);
			Render->StoreDriver(pDriver, m_iDriverSize);
			Render->StoreSamples(m_vSamples);
		}
		else {
			Render->StoreDriver(pDriver, m_iDriverSize);
			Render->StoreChunks(m_vChunks);
			Render->StoreSamples(m_vSamples);
		}
	}

	// Writing done, print some stats
	Print(_T(" * NSF load address: $%04X\n"), m_iLoadAddress);
	Print(_T("Writing output file...\n"));
	Print(_T(" * Driver size: %i bytes\n"), m_iDriverSize);

	if (m_bBankSwitched) {
		int Percent = (100 * m_iMusicDataSize) / (0x80000 - m_iDriverSize - m_iSamplesSize);
		int Banks = Render->GetBankCount();
		Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);
		Print(_T(" * NSF type: Bankswitched (%i banks)\n"), Banks - 1);
	}
	else {
		int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize - m_iSamplesSize);
		Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);
		Print(_T(" * NSF type: Linear (driver @ $%04X)\n"), m_iDriverAddress);
	}

	// write NEND chunk
	const unsigned char NEndIdent[] = {'\0', '\0', '\0', '\0', 'N', 'E', 'N', 'D'};		// // //
	OutputFile.Write(&NEndIdent, sizeof(NEndIdent));

	// write actual size of DATA chunk
	OutputFile.Seek(iDataSizePos, CFile::begin);
	iDataSize = Render->GetDATAChunkSize();
	OutputFile.Write(reinterpret_cast<char*>(&iDataSize), sizeof(int));

	Print(_T("Done, total file size: %i bytes\n"), OutputFile.GetLength());

	// Done
	OutputFile.Close();

	Cleanup();
}

void CCompiler::ExportNSF2(LPCTSTR lpszFileName, int MachineType)
{
	ClearLog();

	// Build the music data
	if (!CompileData()) {
		// Failed
		Cleanup();
		return;
	}

	if (m_bBankSwitched) {
		// Expand and allocate label addresses
		AddBankswitching();
		if (!ResolveLabelsBankswitched()) {
			Cleanup();
			return;
		}
		// Write bank data
		UpdateFrameBanks();
		UpdateSongBanks();
		// Make driver aware of bankswitching
		EnableBankswitching();
	}
	else {
		ResolveLabels();
		ClearSongBanks();
	}

	// Rewrite DPCM sample pointers
	UpdateSamplePointers(m_iSampleStart);

	// Compressed mode means that driver and music is located just below the sample space, no space is lost even when samples are used
	bool bCompressedMode;
	unsigned short MusicDataAddress;

	// Find out load address
	if ((PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize) < 0x8000 || m_bBankSwitched || m_iActualChip != m_pDocument->GetExpansionChip()) // // //
		bCompressedMode = false;
	else
		bCompressedMode = true;

	if (bCompressedMode) {
		// Locate driver at $C000 - (driver size)
		m_iLoadAddress = PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize;
		m_iDriverAddress = PAGE_SAMPLES - m_iDriverSize;
		MusicDataAddress = m_iLoadAddress;
	}
	else {
		// Locate driver at $8000
		m_iLoadAddress = PAGE_START;
		m_iDriverAddress = PAGE_START;
		MusicDataAddress = m_iLoadAddress + m_iDriverSize;
	}

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + DATA_HEADER_SIZE;		// // //

	// Load driver
	std::unique_ptr<char[]> pDriverPtr(LoadDriver(m_pDriverData, m_iDriverAddress));		// // //
	char* pDriver = pDriverPtr.get();

	// Patch driver binary
	PatchVibratoTable(pDriver);

	// Copy the Namco table, if used
	// // // nothing here, ft_channel_type is taken care in LoadDriver

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	// Open output file
	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		Print(_T("Error: Could not open output file\n"));
		Cleanup();
		return;
	}

	// Write NSF data
	std::unique_ptr<CChunkRenderNSF> Render(new CChunkRenderNSF(&OutputFile, m_iLoadAddress));

	// Create NSF header
	stNSFHeader Header;
	CreateHeader(&Header, MachineType, 0x80, true);

	// Write header
	OutputFile.Write(&Header, sizeof(stNSFHeader));

	ULONGLONG iDataSizePos = OutputFile.GetPosition() - 3;

	if (m_bBankSwitched) {
		Render->StoreDriver(pDriver, m_iDriverSize);
		Render->StoreChunksBankswitched(m_vChunks);
		Render->StoreSamplesBankswitched(m_vSamples);
	}
	else {
		if (bCompressedMode) {
			Render->StoreChunks(m_vChunks);
			Render->StoreDriver(pDriver, m_iDriverSize);
			Render->StoreSamples(m_vSamples);
		}
		else {
			Render->StoreDriver(pDriver, m_iDriverSize);
			Render->StoreChunks(m_vChunks);
			Render->StoreSamples(m_vSamples);
		}
	}

	// // // Create NSFe metadata

	std::size_t
		iVRC7Size = 0,
		iAuthSize = 0,
		iTimeSize = 0,
		iTlblSize = 0,
		iDataSize = 0,
		iTextSize = 0,
		iMixeSize = 0;

	for (unsigned int i = 0; i < m_pDocument->GetTrackCount(); i++) {
		iTimeSize += 4;
		iTlblSize += strlen(m_pDocument->GetTrackTitle(i)) + 1;
	}

	// write VRC7 chunk
	// YM2413 and YMF281B are considered external OPLL
	uint8_t extOPLL = m_pDocument->GetExternalOPLLChipCheck() || (theApp.GetSettings()->Emulation.iVRC7Patch > 6); iVRC7Size++;
	uint8_t patchset[19 * 8];
	if (extOPLL)
		for (int i = 0; i < 19; i++)
			for (int j = 0; j < 8; j++) {
				patchset[(8 * i) + j] = m_pDocument->GetOPLLPatchByte((8 * i) + j);
				iVRC7Size++;
			}

	const unsigned char VRC7Ident[] = { 'V', 'R', 'C', '7' };
	OutputFile.Write(reinterpret_cast<char*>(&iVRC7Size), sizeof(int));
	OutputFile.Write(&VRC7Ident, sizeof(VRC7Ident));
	OutputFile.Write(&extOPLL, sizeof(uint8_t));
	if (extOPLL)
		OutputFile.Write(&patchset, (sizeof(uint8_t)*19*8));

	// write time chunk
	const unsigned char TimeIdent[] = { 't', 'i', 'm', 'e' };
	OutputFile.Write(reinterpret_cast<char*>(&iTimeSize), sizeof(int));
	OutputFile.Write(&TimeIdent, sizeof(TimeIdent));
	for (unsigned int i = 0; i < m_pDocument->GetTrackCount(); i++) {
		int t = static_cast<int>(m_pDocument->GetStandardLength(i, 1) * 1000.0 + 0.5);
		OutputFile.Write(reinterpret_cast<char*>(&t), sizeof(int));
	}

	//  write tlbl chunk
	const unsigned char TlblIdent[] = { 't', 'l', 'b', 'l' };
	OutputFile.Write(reinterpret_cast<char*>(&iTlblSize), sizeof(int));
	OutputFile.Write(&TlblIdent, sizeof(TlblIdent));
	for (unsigned int i = 0; i < m_pDocument->GetTrackCount(); i++) {
		OutputFile.Write(m_pDocument->GetTrackTitle(i), static_cast<UINT>(strlen(m_pDocument->GetTrackTitle(i)) + 1));
	}

	// write auth chunk
	CString ripper = _T(APP_NAME_VERSION);
	iAuthSize = strlen(m_pDocument->GetSongName()) + strlen(m_pDocument->GetSongArtist())
		+ strlen(m_pDocument->GetSongCopyright()) + ripper.GetLength() + 4;

	const unsigned char AuthIdent[] = { 'a', 'u', 't', 'h' };
	OutputFile.Write(reinterpret_cast<char*>(&iAuthSize), sizeof(int));
	OutputFile.Write(&AuthIdent, sizeof(AuthIdent));
	OutputFile.Write(m_pDocument->GetSongName(), static_cast<UINT>(strlen(m_pDocument->GetSongName()) + 1));
	OutputFile.Write(m_pDocument->GetSongArtist(), static_cast<UINT>(strlen(m_pDocument->GetSongArtist()) + 1));
	OutputFile.Write(m_pDocument->GetSongCopyright(), static_cast<UINT>(strlen(m_pDocument->GetSongCopyright()) + 1));
	OutputFile.Write((char*)((PCSTR)ripper), ripper.GetLength() + 1);
	ripper.ReleaseBuffer();

	// write text chunk, if available
	if (strlen(m_pDocument->GetComment())) {
		const unsigned char TextIdent[] = { 't', 'e', 'x', 't' };
		iTextSize = strlen(m_pDocument->GetComment()) + 1;
		OutputFile.Write(reinterpret_cast<char*>(&iTextSize), sizeof(int));
		OutputFile.Write(&TextIdent, sizeof(TextIdent));
		OutputFile.Write(m_pDocument->GetComment(), static_cast<UINT>(strlen(m_pDocument->GetComment()) + 1));
	}

	// write mixe chunk
	const unsigned char MixeIdent[] = { 'm', 'i', 'x', 'e' };;

	int16_t mixe_device[8]{};

	CSoundGen* pSoundGen = theApp.GetSoundGenerator();

	for (int i = 0; i < 8; i++) {
		if (m_pDocument->GetLevelOffset(i)) {
			mixe_device[i] = (m_pDocument->GetLevelOffset(i) * 10) + pSoundGen->SurveyMixLevels[i];
			iMixeSize += 3;
		}
	}
	OutputFile.Write(reinterpret_cast<char*>(&iMixeSize), sizeof(int));
	OutputFile.Write(&MixeIdent, sizeof(MixeIdent));
	for (uint8_t i = 0; i < 8; i++) {
		if (m_pDocument->GetLevelOffset(i)) {
			unsigned char devicebyte = 0;
			OutputFile.Write(&i, sizeof(uint8_t));
			OutputFile.Write(reinterpret_cast<char*>(&mixe_device[i]), sizeof(int16_t));
		}
	}

	// write NEND chunk
	const unsigned char NEndIdent[] = { '\0', '\0', '\0', '\0', 'N', 'E', 'N', 'D' };		// // //
	OutputFile.Write(&NEndIdent, sizeof(NEndIdent));

	// write actual size of DATA
	OutputFile.Seek(iDataSizePos, CFile::begin);
	iDataSize = Render->GetDATAChunkSize();
	unsigned char NSFSize[3] = {
		char(iDataSize & 0xFF),
		char((iDataSize >> 8) & 0xFF),
		char((iDataSize >> 16) & 0xFF)};
	OutputFile.Write(&NSFSize, sizeof(NSFSize));

	// Writing done, print some stats
	Print(_T(" * NSF load address: $%04X\n"), m_iLoadAddress);
	Print(_T("Writing output file...\n"));
	Print(_T(" * Driver size: %i bytes\n"), m_iDriverSize);

	if (m_bBankSwitched) {
		int Percent = (100 * m_iMusicDataSize) / (0x80000 - m_iDriverSize - m_iSamplesSize);
		int Banks = Render->GetBankCount();
		Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);
		Print(_T(" * NSF type: Bankswitched (%i banks)\n"), Banks - 1);
	}
	else {
		int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize - m_iSamplesSize);
		Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);
		Print(_T(" * NSF type: Linear (driver @ $%04X)\n"), m_iDriverAddress);
	}

	Print(_T("Done, total file size: %i bytes\n"), OutputFile.GetLength());

	// Done
	OutputFile.Close();

	Cleanup();
}

void CCompiler::ExportNES(LPCTSTR lpszFileName, bool EnablePAL)
{
	// 32kb NROM, no CHR
	const char NES_HEADER[] = {
		0x4E, 0x45, 0x53, 0x1A, 0x02, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	ClearLog();

	bool nsfewarning = false;

	for (int i = 0; i < 8; i++) {
		nsfewarning |= m_pDocument->GetLevelOffset(i) != 0;
	}

	nsfewarning |= m_pDocument->GetExternalOPLLChipCheck();

	if (nsfewarning) {
		Print(_T("Warning: NSFe optional metadata will not be exported in this format!\n"));
		theApp.DisplayMessage(_T("NSFe optional metadata will not be exported in this format!"), 0, 0);
	}

	if (m_pDocument->GetExpansionChip() != SNDCHIP_NONE) {
		Print(_T("Error: Expansion chips not supported.\n"));
		theApp.DisplayMessage(_T("Expansion chips are currently not supported when exporting to .NES!"), 0, 0);
		Cleanup();
		return;
	}

	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		Cleanup();
		return;
	}

	// Build the music data
	if (!CompileData()) {
		Cleanup();
		return;
	}

	if (m_bBankSwitched) {
		// Abort if larger than 32kb
		Print(_T("Error: Song is too large, aborted.\n"));
		theApp.DisplayMessage(_T("Song is too big to fit into 32kB!"), 0, 0);
		Cleanup();
		return;
	}

	ResolveLabels();

	// Rewrite DPCM sample pointers
	UpdateSamplePointers(m_iSampleStart);

	// Locate driver at $8000
	m_iLoadAddress = PAGE_START;
	m_iDriverAddress = PAGE_START;
	unsigned short MusicDataAddress = m_iLoadAddress + m_iDriverSize;

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + DATA_HEADER_SIZE;		// // //

	// Load driver
	std::unique_ptr<char[]> pDriverPtr(LoadDriver(m_pDriverData, m_iDriverAddress));		// // //
	char *pDriver = pDriverPtr.get();

	// Patch driver binary
	PatchVibratoTable(pDriver);

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize - m_iSamplesSize);

	Print(_T("Writing file...\n"));
	Print(_T(" * Driver size: %i bytes\n"), m_iDriverSize);
	Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);

	// Write header
	OutputFile.Write(NES_HEADER, 0x10);

	// Write NES data
	std::unique_ptr<CChunkRenderNES> Render(new CChunkRenderNES(&OutputFile, m_iLoadAddress));
	Render->StoreDriver(pDriver, m_iDriverSize);
	Render->StoreChunks(m_vChunks);
	Render->StoreSamples(m_vSamples);
	Render->StoreCaller(NSF_CALLER_BIN, NSF_CALLER_SIZE);

	Print(_T("Done, total file size: %i bytes\n"), 0x8000 + 0x10);

	// Done
	OutputFile.Close();

	Cleanup();
}

void CCompiler::ExportBIN(LPCTSTR lpszBIN_File, LPCTSTR lpszDPCM_File, int MachineType, bool ExtraData)
{
	ClearLog();

	bool nsfewarning = false;

	for (int i = 0; i < 8; i++) {
		nsfewarning |= m_pDocument->GetLevelOffset(i) != 0;
	}

	nsfewarning |= m_pDocument->GetExternalOPLLChipCheck();

	if (nsfewarning) {
		Print(_T("Warning: NSFe optional metadata will not be exported in this format!\n"));
		theApp.DisplayMessage(_T("NSFe optional metadata will not be exported in this format!"), 0, 0);
	}

	// Build the music data
	if (!CompileData()) {
		// Failed
		Cleanup();
		return;
	}

	if (m_bBankSwitched) {
		// TODO: write seperate binary banks
		Print(_T("Error: Can't write bankswitched songs!\n"));
		Cleanup();
		return;
	}
	else {
		ResolveLabels();
		ClearSongBanks();
	}

	// Rewrite DPCM sample pointers
	UpdateSamplePointers(m_iSampleStart);

	// Compressed mode means that driver and music is located just below the sample space, no space is lost even when samples are used
	bool bCompressedMode;
	unsigned short MusicDataAddress;

	// Find out load address
	if ((PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize) < 0x8000 || m_bBankSwitched || m_iActualChip != m_pDocument->GetExpansionChip()) // // //
		bCompressedMode = false;
	else
		bCompressedMode = true;

	if (bCompressedMode) {
		// Locate driver at $C000 - (driver size)
		m_iLoadAddress = PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize;
		m_iDriverAddress = PAGE_SAMPLES - m_iDriverSize;
		MusicDataAddress = m_iLoadAddress;
	}
	else {
		// Locate driver at $8000
		m_iLoadAddress = PAGE_START;
		m_iDriverAddress = PAGE_START;
		MusicDataAddress = m_iLoadAddress + m_iDriverSize;
	}

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + DATA_HEADER_SIZE;		// // //

	stNSFHeader Header;
	CreateHeader(&Header, MachineType, 0x00, false);

	// Open output files
	CFile OutputFileBIN;
	if (!OpenFile(lpszBIN_File, OutputFileBIN)) {
		OutputFileBIN.Close();
		Print(_T("Error: Could not open output binary file\n"));
		Cleanup();
		return;
	}

	CFile OutputFileDPCM;
	if (_tcslen(lpszDPCM_File) != 0) {
		if (!OpenFile(lpszDPCM_File, OutputFileDPCM)) {
			OutputFileDPCM.Close();
			OutputFileBIN.Close();
			Print(_T("Error: Could not open output binary DPCM file\n"));
			Cleanup();
			return;
		}
	}

	Print(_T("Writing output files...\n"));

	WriteBinary(&OutputFileBIN);

	if (_tcslen(lpszDPCM_File) != 0)
		WriteSamplesBinary(&OutputFileDPCM);

	if (ExtraData) {
		// Get the directory of the BIN
		CString BINDirectory = OutputFileBIN.GetFilePath();
		BINDirectory.Delete(
			(OutputFileBIN.GetFilePath().GetLength()) - (OutputFileBIN.GetFileName().GetLength()),
			OutputFileBIN.GetFileName().GetLength());

		// Write period file
		CFile OutputFilePeriod;
		CString period_directory = BINDirectory;
		period_directory += _T("periods.s");
		if (!OpenFile(period_directory, OutputFilePeriod)) {
			OutputFilePeriod.Close();
			Print(_T("Error: Could not open output periods file\n"));
			Cleanup();
			return;
		}

		// Write vibrato file
		CFile OutputFileVibrato;
		CString vibrato_directory = BINDirectory;
		vibrato_directory += _T("vibrato.s");
		if (!OpenFile(vibrato_directory, OutputFileVibrato)) {
			OutputFilePeriod.Close();
			OutputFileVibrato.Close();
			Print(_T("Error: Could not open output vibrato file\n"));
			Cleanup();
			return;
		}

		// Write NSF header
		CFile OutputFileNSFHeader;
		CString header_directory = BINDirectory;
		header_directory += _T("nsf_header.s");

		if (!OpenFile(header_directory, OutputFileNSFHeader)) {
			OutputFilePeriod.Close();
			OutputFileVibrato.Close();
			OutputFileNSFHeader.Close();
			Print(_T("Error: Could not open NSF header file\n"));
			Cleanup();
			return;
		}

		// Write NSF config file
		CFile OutputNSFConfig;
		CString config_directory = BINDirectory;
		config_directory += _T("nsf.cfg");
		if (!OpenFile(config_directory, OutputNSFConfig)) {
			OutputFilePeriod.Close();
			OutputFileVibrato.Close();
			OutputFileNSFHeader.Close();
			OutputNSFConfig.Close();
			Print(_T("Error: Could not open NSF config file\n"));
			Cleanup();
			return;
		}

		Print(_T("Writing additional data files...\n"));
		WritePeriods(&OutputFilePeriod);
		WriteVibrato(&OutputFileVibrato);
		WriteNSFHeader(&OutputFileNSFHeader, Header);
		WriteNSFConfig(&OutputNSFConfig, m_iSampleStart, Header);
		OutputFilePeriod.Close();
		OutputFileVibrato.Close();
		OutputFileNSFHeader.Close();
		OutputNSFConfig.Close();
	}

	Print(_T("Done\n"));

	// Done
	OutputFileBIN.Close();

	if (_tcslen(lpszDPCM_File) != 0)
		OutputFileDPCM.Close();

	Cleanup();
}

void CCompiler::ExportPRG(LPCTSTR lpszFileName, bool EnablePAL)
{
	// Same as export to .NES but without the header

	ClearLog();

	bool nsfewarning = false;

	for (int i = 0; i < 8; i++) {
		nsfewarning |= m_pDocument->GetLevelOffset(i) != 0;
	}

	nsfewarning |= m_pDocument->GetExternalOPLLChipCheck();

	if (nsfewarning) {
		Print(_T("Warning: NSFe optional metadata will not be exported in this format!\n"));
		theApp.DisplayMessage(_T("NSFe optional metadata will not be exported in this format!"), 0, 0);
	}

	if (m_pDocument->GetExpansionChip() != SNDCHIP_NONE) {
		Print(_T("Expansion chips not supported.\n"));
		theApp.DisplayMessage(_T("Error: Expansion chips is currently not supported when exporting to PRG!"), 0, 0);
		Cleanup();
		return;
	}

	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		Cleanup();
		return;
	}

	// Build the music data
	if (!CompileData()) {
		Cleanup();
		return;
	}

	if (m_bBankSwitched) {
		// Abort if larger than 32kb
		Print(_T("Song is too big, aborted.\n"));
		theApp.DisplayMessage(_T("Error: Song is too big to fit!"), 0, 0);
		Cleanup();
		return;
	}

	ResolveLabels();

	// Rewrite DPCM sample pointers
	UpdateSamplePointers(m_iSampleStart);

	// Locate driver at $8000
	m_iLoadAddress = PAGE_START;
	m_iDriverAddress = PAGE_START;
	unsigned short MusicDataAddress = m_iLoadAddress + m_iDriverSize;

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + DATA_HEADER_SIZE;		// // //

	// Load driver
	std::unique_ptr<char[]> pDriverPtr(LoadDriver(m_pDriverData, m_iDriverAddress));		// // //
	char *pDriver = pDriverPtr.get();

	// Patch driver binary
	PatchVibratoTable(pDriver);

	// Write music data address
	SetDriverSongAddress(pDriver, MusicDataAddress);

	int Percent = (100 * m_iMusicDataSize) / (0x8000 - m_iDriverSize - m_iSamplesSize);

	Print(_T("Writing file...\n"));
	Print(_T(" * Driver size: %i bytes\n"), m_iDriverSize);
	Print(_T(" * Song data size: %i bytes (%i%%)\n"), m_iMusicDataSize, Percent);

	// Write NES data
	std::unique_ptr<CChunkRenderNES> Render(new CChunkRenderNES(&OutputFile, m_iLoadAddress));
	Render->StoreDriver(pDriver, m_iDriverSize);
	Render->StoreChunks(m_vChunks);
	Render->StoreSamples(m_vSamples);
	Render->StoreCaller(NSF_CALLER_BIN, NSF_CALLER_SIZE);

	// Done
	OutputFile.Close();

	Cleanup();
}

void CCompiler::ExportASM(LPCTSTR lpszFileName, int MachineType, bool ExtraData)
{
	ClearLog();

	bool nsfewarning = false;

	for (int i = 0; i < 8; i++) {
		nsfewarning |= m_pDocument->GetLevelOffset(i) != 0;
	}

	nsfewarning |= m_pDocument->GetExternalOPLLChipCheck();

	if (nsfewarning) {
		Print(_T("Warning: NSFe optional metadata will not be exported in this format!\n"));
		theApp.DisplayMessage(_T("NSFe optional metadata will not be exported in this format!"), 0, 0);
	}

	// Build the music data
	if (!CompileData()) {
		// Failed
		Cleanup();
		return;
	}

	if (m_bBankSwitched) {
		// Expand and allocate label addresses
		AddBankswitching();
		if (!ResolveLabelsBankswitched()) {
			Cleanup();
			return;
		}
		// Make driver aware of bankswitching
		EnableBankswitching();
		// Write bank data
		UpdateFrameBanks();
		UpdateSongBanks();
	}
	else {
		ResolveLabels();
		ClearSongBanks();
	}

	// Rewrite DPCM sample pointers
	UpdateSamplePointers(m_iSampleStart);

	// Compressed mode means that driver and music is located just below the sample space, no space is lost even when samples are used
	bool bCompressedMode;
	unsigned short MusicDataAddress;

	// Find out load address
	if ((PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize) < 0x8000 || m_bBankSwitched || m_iActualChip != m_pDocument->GetExpansionChip()) // // //
		bCompressedMode = false;
	else
		bCompressedMode = true;

	if (bCompressedMode) {
		// Locate driver at $C000 - (driver size)
		m_iLoadAddress = PAGE_SAMPLES - m_iDriverSize - m_iMusicDataSize;
		m_iDriverAddress = PAGE_SAMPLES - m_iDriverSize;
		MusicDataAddress = m_iLoadAddress;
	}
	else {
		// Locate driver at $8000
		m_iLoadAddress = PAGE_START;
		m_iDriverAddress = PAGE_START;
		MusicDataAddress = m_iLoadAddress + m_iDriverSize;
	}

	// Init is located first at the driver
	m_iInitAddress = m_iDriverAddress + DATA_HEADER_SIZE;		// // //

	stNSFHeader Header;
	CreateHeader(&Header, MachineType, 0x00, false);

	CFile OutputFile;
	if (!OpenFile(lpszFileName, OutputFile)) {
		OutputFile.Close();
		Print(_T("Error: Could not open output file\n"));
		Cleanup();
		return;
	}

	// Write output file
	Print(_T("Writing output files...\n"));

	WriteAssembly(&OutputFile);

	if (ExtraData) {
		// Get the directory of the output file
		CString FileDirectory = OutputFile.GetFilePath();
		FileDirectory.Delete(
			(OutputFile.GetFilePath().GetLength()) - (OutputFile.GetFileName().GetLength()),
			OutputFile.GetFileName().GetLength());

		// Write period file
		CFile OutputFilePeriod;
		CString period_directory = FileDirectory;
		period_directory += _T("periods.s");

		if (!OpenFile(period_directory, OutputFilePeriod)) {
			OutputFilePeriod.Close();
			Print(_T("Error: Could not open output periods file\n"));
			Cleanup();
			return;
		}

		// Write vibrato file
		CFile OutputFileVibrato;
		CString vibrato_directory = FileDirectory;
		vibrato_directory += _T("vibrato.s");

		if (!OpenFile(vibrato_directory, OutputFileVibrato)) {
			OutputFilePeriod.Close();
			OutputFileVibrato.Close();
			Print(_T("Error: Could not open output vibrato file\n"));
			Cleanup();
			return;
		}

		// Write NSF header
		CFile OutputFileNSFHeader;
		CString header_directory = FileDirectory;
		header_directory += _T("nsf_header.s");

		if (!OpenFile(header_directory, OutputFileNSFHeader)) {
			OutputFilePeriod.Close();
			OutputFileVibrato.Close();
			OutputFileNSFHeader.Close();
			Print(_T("Error: Could not open output NSF header file\n"));
			Cleanup();
			return;
		}

		// Write NSF config file
		CFile OutputNSFConfig;
		CString config_directory = FileDirectory;
		config_directory += _T("nsf.cfg");
		if (!OpenFile(config_directory, OutputNSFConfig)) {
			OutputFilePeriod.Close();
			OutputFileVibrato.Close();
			OutputFileNSFHeader.Close();
			OutputNSFConfig.Close();
			Print(_T("Error: Could not open NSF config file\n"));
			Cleanup();
			return;
		}

		Print(_T("Writing additional data files...\n"));
		WritePeriods(&OutputFilePeriod);
		WriteVibrato(&OutputFileVibrato);
		WriteNSFHeader(&OutputFileNSFHeader, Header);
		WriteNSFConfig(&OutputNSFConfig, m_iSampleStart, Header);
		OutputFilePeriod.Close();
		OutputFileVibrato.Close();
		OutputFileNSFHeader.Close();
		OutputNSFConfig.Close();
	}

	// Done
	OutputFile.Close();

	Print(_T("Done\n"));

	Cleanup();
}

char* CCompiler::LoadDriver(const driver_t *pDriver, unsigned short Origin) const
{
	// Copy embedded driver
	unsigned char* pData = new unsigned char[pDriver->driver_size];
	memcpy(pData, pDriver->driver, pDriver->driver_size);
	
	// // // Custom pitch tables
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	for (size_t i = 0; i < pDriver->freq_table_size; i += 2) {		// // //
		int Table = pDriver->freq_table[i + 1];
		switch (Table) {
		case CDetuneTable::DETUNE_NTSC:
		case CDetuneTable::DETUNE_PAL:
		case CDetuneTable::DETUNE_SAW:
		case CDetuneTable::DETUNE_FDS:
		case CDetuneTable::DETUNE_N163:
			for (int j = 0; j < NOTE_COUNT; ++j) {
				int Reg = pSoundGen->ReadPeriodTable(j, Table);
				pData[pDriver->freq_table[i] + 2 * j    ] = Reg & 0xFF;
				pData[pDriver->freq_table[i] + 2 * j + 1] = Reg >> 8;
			} break;
		case CDetuneTable::DETUNE_VRC7:
			for (int j = 0; j <= NOTE_RANGE; ++j) { // one extra item
				int Reg = pSoundGen->ReadPeriodTable(j % NOTE_RANGE, Table) * 4;
				if (j == NOTE_RANGE) Reg <<= 1;
				pData[pDriver->freq_table[i] + j                 ] = Reg & 0xFF;
				pData[pDriver->freq_table[i] + j + NOTE_RANGE + 1] = Reg >> 8;
			} break;
		default:
			AfxDebugBreak();
		}
	}

	// Relocate driver
	for (size_t i = 0; i < pDriver->word_reloc_size; ++i) {
		// Words
		unsigned short value = pData[pDriver->word_reloc[i]] + (pData[pDriver->word_reloc[i] + 1] << 8);
		value += Origin;
		pData[pDriver->word_reloc[i]] = value & 0xFF;
		pData[pDriver->word_reloc[i] + 1] = value >> 8;
	}

	for (size_t i = 0; i < pDriver->adr_reloc_size; i += 2) {		// // //
		unsigned short value = pData[pDriver->adr_reloc[i]] + (pData[pDriver->adr_reloc[i + 1]] << 8);
		value += Origin;
		pData[pDriver->adr_reloc[i]] = value & 0xFF;
		pData[pDriver->adr_reloc[i + 1]] = value >> 8;
	}

	if (m_iActualChip == SNDCHIP_N163) {
		pData[m_iDriverSize - 2 - 0x100 - 0xC0 * 2 - 8 - 1 - 8 + m_iActualNamcoChannels] = 3;
	}

	if (m_iActualChip & (m_iActualChip - 1)) {		// // // special processing for multichip
		int ptr = FT_UPDATE_EXT_ADR;
		for (int i = 0; i < 6; ++i) {
			ASSERT(pData[ptr] == 0x20); // jsr
			if (!(m_iActualChip & (1 << i))) {
				pData[ptr++] = 0xEA; // nop
				pData[ptr++] = 0xEA;
				pData[ptr++] = 0xEA;
			}
			else
				ptr += 3;
		}

		const int CH_MAP[] = {
			0, 1, 2, 3, 27,
			6, 7, 8,
			4, 5, -1,
			9, 10, 11, 12, 13, 14, 15, 16,
			17,
			21, 22, 23, 24, 25, 26,
			18, 19, 20,
		};

		for (int i = 0; i < CHANNELS; ++i)
			pData[FT_CH_ENABLE_ADR + i] = 0;
		for (const int x : m_vChanOrder)
			pData[FT_CH_ENABLE_ADR + CH_MAP[m_pDocument->GetChannelType(x)]] = 1;
	}

	return (char*)pData;
}

void CCompiler::SetDriverSongAddress(char *pDriver, unsigned short Address) const
{
	// Write start address of music data
	pDriver[m_iDriverSize - 2] = Address & 0xFF;
	pDriver[m_iDriverSize - 1] = Address >> 8;
}

void CCompiler::PatchVibratoTable(char *pDriver) const
{
	// Copy the vibrato table, the stock one only works for new vibrato mode
	const CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	for (int i = 0; i < 256; ++i) {
		*(pDriver + m_iVibratoTableLocation + i) = (char)pSoundGen->ReadVibratoTable(i);
	}
}

#pragma warning (push)
#pragma warning (disable : 4996)
void CCompiler::CreateHeader(stNSFHeader *pHeader, int MachineType, unsigned int NSF2Flags, bool NSF2) const
{
	// Fill the NSF header
	//
	// Speed will be the same for NTSC/PAL
	//

	unsigned int SpeedPAL, SpeedNTSC, Speed;
	Speed = m_pDocument->GetEngineSpeed();

	// If speed is default, write correct NTSC/PAL speed periods
	// else, set the same custom speed for both
	SpeedNTSC = (Speed == 0) ? 1000000 / CAPU::FRAME_RATE_NTSC : 1000000 / Speed;
	SpeedPAL = (Speed == 0) ? 1000000 / CAPU::FRAME_RATE_PAL : 1000000 / Speed;

	memset(pHeader, 0, 0x80);

	pHeader->Ident[0]	= 0x4E;
	pHeader->Ident[1]	= 0x45;
	pHeader->Ident[2]	= 0x53;
	pHeader->Ident[3]	= 0x4D;
	pHeader->Ident[4]	= 0x1A;

	pHeader->Version	= NSF2 ? 0x02 : 0x01;
	pHeader->TotalSongs	= m_pDocument->GetTrackCount();
	pHeader->StartSong	= 1;
	pHeader->LoadAddr	= m_iLoadAddress;
	pHeader->InitAddr	= m_iInitAddress;
	pHeader->PlayAddr	= m_iInitAddress + 3;

	memset(pHeader->SongName, 0x00, 32);
	memset(pHeader->ArtistName, 0x00, 32);
	memset(pHeader->Copyright, 0x00, 32);

	strncpy((char*)pHeader->SongName,   m_pDocument->GetSongName(), 32);
	strncpy((char*)pHeader->ArtistName, m_pDocument->GetSongArtist(), 32);
	strncpy((char*)pHeader->Copyright,  m_pDocument->GetSongCopyright(), 32);

	pHeader->Speed_NTSC = SpeedNTSC; //0x411A; // default ntsc speed

	if (m_bBankSwitched) {
		for (int i = 0; i < 4; ++i) {
			unsigned int SampleBank = m_iFirstSampleBank + i;
			pHeader->BankValues[i] = i;
			pHeader->BankValues[i + 4] = (SampleBank < m_iLastBank) ? SampleBank : m_iLastBank;
		}
		if (LAST_BANK_FIXED) {
			// Bind last page to last bank
			pHeader->BankValues[7] = m_iLastBank;
		}
	}
	else {
		for (int i = 0; i < 8; ++i) {
			pHeader->BankValues[i] = 0;
		}
	}

	pHeader->Speed_PAL = SpeedPAL; //0x4E20; // default pal speed

	// Allow PAL or dual tunes only if no expansion chip is selected
	// Expansion chips weren't available in PAL areas
	if (m_pDocument->GetExpansionChip() == SNDCHIP_NONE) {
		switch (MachineType) {
			case 0:	// NTSC
				pHeader->Flags = 0x00;
				break;
			case 1:	// PAL
				pHeader->Flags = 0x01;
				break;
			case 2:	// Dual
				pHeader->Flags = 0x02;
				break;
		}
	}
	else {
		pHeader->Flags = 0x00;
	}

	// Expansion chip
	pHeader->SoundChip = m_iActualChip;		// // //

	pHeader->NSF2Flags = NSF2Flags;
	pHeader->NSFDataLength[0] = 0;		// !! !! Allow this to be overwritten
	pHeader->NSFDataLength[1] = 0;
	pHeader->NSFDataLength[2] = 0;
}
#pragma warning (pop)

void CCompiler::CreateNSFeHeader(stNSFeHeader *pHeader, int MachineType)		// // //
{
	memset(pHeader, 0, 40);

	unsigned int SpeedPAL, SpeedNTSC, Speed;
	Speed = m_pDocument->GetEngineSpeed();
	SpeedNTSC = (Speed == 0) ? 1000000 / CAPU::FRAME_RATE_NTSC : 1000000 / Speed;
	SpeedPAL = (Speed == 0) ? 1000000 / CAPU::FRAME_RATE_PAL : 1000000 / Speed;

	pHeader->InfoSize = 12;
	pHeader->BankSize = 8;
	pHeader->RateSize = 4;

	pHeader->NSFeIdent[0] = 'N';
	pHeader->NSFeIdent[1] = 'S';
	pHeader->NSFeIdent[2] = 'F';
	pHeader->NSFeIdent[3] = 'E';

	pHeader->InfoIdent[0] = 'I';
	pHeader->InfoIdent[1] = 'N';
	pHeader->InfoIdent[2] = 'F';
	pHeader->InfoIdent[3] = 'O';

	pHeader->BankIdent[0] = 'B';
	pHeader->BankIdent[1] = 'A';
	pHeader->BankIdent[2] = 'N';
	pHeader->BankIdent[3] = 'K';

	pHeader->RateIdent[0] = 'R';
	pHeader->RateIdent[1] = 'A';
	pHeader->RateIdent[2] = 'T';
	pHeader->RateIdent[3] = 'E';

	pHeader->TotalSongs	= m_pDocument->GetTrackCount();
	pHeader->StartSong	= 0;
	pHeader->LoadAddr	= m_iLoadAddress;
	pHeader->InitAddr	= m_iInitAddress;
	pHeader->PlayAddr	= m_iInitAddress + 3;

	if (m_bBankSwitched) {
		for (int i = 0; i < 4; ++i) {
			pHeader->BankValues[i] = i;
			pHeader->BankValues[i + 4] = m_iFirstSampleBank + i;
		}
	}
	else {
		for (int i = 0; i < 8; ++i) {
			pHeader->BankValues[i] = 0;
		}
	}

	if (m_pDocument->GetExpansionChip() == SNDCHIP_NONE) {
		switch (MachineType) {
			case 0:	// NTSC
				pHeader->Flags = 0x00;
				break;
			case 1:	// PAL
				pHeader->Flags = 0x01;
				break;
			case 2:	// Dual
				pHeader->Flags = 0x02;
				break;
		}
	}
	else {
		pHeader->Flags = 0x00;
	}

	pHeader->Speed_NTSC = SpeedNTSC;
	pHeader->Speed_PAL = SpeedPAL;
	pHeader->SoundChip = m_iActualChip;		// // //
}


void CCompiler::UpdateSamplePointers(unsigned int Origin)
{
	// Rewrite sample pointer list with valid addresses
	//
	// TODO: rewrite this to utilize the CChunkDataBank to resolve bank numbers automatically
	//

	ASSERT(m_pSamplePointersChunk != NULL);

	unsigned int Address = Origin;
	unsigned int Bank = m_iFirstSampleBank;

	if (!m_bBankSwitched)
		Bank = 0;			// Disable DPCM bank switching

	m_pSamplePointersChunk->Clear();

	// The list is stored in the same order as the samples vector

	for (auto pDSample : m_vSamples) {
		unsigned int Size = pDSample->GetSize();

		if (m_bBankSwitched) {
			if ((Address + Size) >= DPCM_SWITCH_ADDRESS) {
				Address = PAGE_SAMPLES;
				Bank += DPCM_PAGE_WINDOW;
			}
		}

		// Store
		m_pSamplePointersChunk->StoreByte(Address >> 6);
		m_pSamplePointersChunk->StoreByte(Size >> 4);
		m_pSamplePointersChunk->StoreByte(Bank);

#ifdef _DEBUG
		Print(_T(" * DPCM sample %s: $%04X, bank %i (%i bytes)\n"), pDSample->GetName(), Address, Bank, Size);
#endif
		Address += Size;
		Address += AdjustSampleAddress(Address);
	}
#ifdef _DEBUG
	Print(_T(" * DPCM sample banks: %i\n"), Bank - m_iFirstSampleBank + DPCM_PAGE_WINDOW);
#endif

	// Save last bank number for NSF header
	m_iLastBank = Bank + 1;
}

void CCompiler::UpdateFrameBanks()
{
	// Write bank numbers to frame lists (can only be used when bankswitching is used)

	int Channels = m_pDocument->GetAvailableChannels();

	for (CChunk *pChunk : m_vFrameChunks) {
		if (pChunk->GetType() == CHUNK_FRAME) {
			// Add bank data
			for (int j = 0; j < Channels; ++j) {
				unsigned char bank = GetObjectByRef(pChunk->GetDataRefName(j))->GetBank();
				if (bank < PATTERN_SWITCH_BANK)
					bank = PATTERN_SWITCH_BANK;
				pChunk->SetupBankData(j + Channels, bank);
			}
		}
	}
}

void CCompiler::UpdateSongBanks()
{
	// Write bank numbers to song lists (can only be used when bankswitching is used)
	for (CChunk *pChunk : m_vSongChunks) {
		int bank = GetObjectByRef(pChunk->GetDataRefName(0))->GetBank();
		if (bank < PATTERN_SWITCH_BANK)
			bank = PATTERN_SWITCH_BANK;
		pChunk->SetupBankData(m_iSongBankReference, bank);
	}
}

void CCompiler::ClearSongBanks()
{
	// Clear bank data in song chunks
	for (CChunk *pChunk : m_vSongChunks)
		pChunk->SetupBankData(m_iSongBankReference, 0);
}

void CCompiler::EnableBankswitching()
{
	// Set bankswitching flag in the song header
	ASSERT(m_pHeaderChunk != NULL);
	unsigned char flags = (unsigned char)m_pHeaderChunk->GetData(m_iHeaderFlagOffset);
	flags |= FLAG_BANKSWITCHED;
	m_pHeaderChunk->ChangeByte(m_iHeaderFlagOffset, flags);
}

void CCompiler::ResolveLabels()
{
	// Resolve label addresses, no banks since bankswitching is disabled
	CMap<CStringA, LPCSTR, int, int> labelMap;

	// Pass 1, collect labels
	CollectLabels(labelMap);

	// Pass 2
	AssignLabels(labelMap);
}

bool CCompiler::ResolveLabelsBankswitched()
{
	// Resolve label addresses and banks
	CMap<CStringA, LPCSTR, int, int> labelMap;

	// Pass 1, collect labels
	if (!CollectLabelsBankswitched(labelMap))
		return false;

	// Pass 2
	AssignLabels(labelMap);

	return true;
}

void CCompiler::CollectLabels(CMap<CStringA, LPCSTR, int, int> &labelMap) const
{
	// Collect labels and assign offsets
	int Offset = 0;
	for (const CChunk *pChunk : m_vChunks) {
		labelMap[pChunk->GetLabel()] = Offset;
		Offset += pChunk->CountDataSize();
	}
}

bool CCompiler::CollectLabelsBankswitched(CMap<CStringA, LPCSTR, int, int> &labelMap)
{
	int Offset = 0;
	int Bank = PATTERN_SWITCH_BANK;

	// Instruments and stuff
	for (const CChunk *pChunk : m_vChunks) {
		int Size = pChunk->CountDataSize();

		switch (pChunk->GetType()) {
			case CHUNK_FRAME_LIST:
			case CHUNK_FRAME:
			case CHUNK_PATTERN:
				break;
			default:
				labelMap[pChunk->GetLabel()] = Offset;
				Offset += Size;
		}
	}

	if (Offset + m_iDriverSize > 0x3000) {
		// Instrument data did not fit within the limit, display an error and abort?
		Print(_T("Error: Instrument data overflow, can't export file!\n"));
		return false;
	}

	unsigned int Track = 0;

	// The switchable area is $B000-$C000
	for (CChunk *pChunk : m_vChunks) {
		int Size = pChunk->CountDataSize();

		switch (pChunk->GetType()) {
			case CHUNK_FRAME_LIST:
				// Make sure the entire frame list will fit, if not then allocate a new bank
				if (Offset + m_iDriverSize + m_iTrackFrameSize[Track++] > 0x4000) {
					Offset = 0x3000 - m_iDriverSize;
					++Bank;
				}
			case CHUNK_FRAME:
				labelMap[pChunk->GetLabel()] = Offset;
				pChunk->SetBank(Bank < 4 ? ((Offset + m_iDriverSize) >> 12) : Bank);
				Offset += Size;
				break;
			case CHUNK_PATTERN:
				// Make sure entire pattern will fit
				if (Offset + m_iDriverSize + Size > 0x4000) {
					Offset = 0x3000 - m_iDriverSize;
					++Bank;
				}
				labelMap[pChunk->GetLabel()] = Offset;
				pChunk->SetBank(Bank < 4 ? ((Offset + m_iDriverSize) >> 12) : Bank);
				Offset += Size;
			default:
				break;
		}
	}

	if (m_bBankSwitched)
		m_iFirstSampleBank = ((Bank < 4) ? ((Offset + m_iDriverSize) >> 12) : Bank) + 1;

	m_iLastBank = m_iFirstSampleBank;

	return true;
}

void CCompiler::AssignLabels(CMap<CStringA, LPCSTR, int, int> &labelMap)
{
	// Pass 2: assign addresses to labels
	for (CChunk *pChunk : m_vChunks)
		pChunk->AssignLabels(labelMap);
}

bool CCompiler::CompileData()
{
	// Compile music data to an object tree
	//

	// // // Full chip export
	m_iActualChip = m_pDocument->GetExpansionChip();
	m_iActualNamcoChannels = m_pDocument->GetNamcoChannels();

	// Select driver and channel order
	switch (m_pDocument->GetExpansionChip()) {
		case SNDCHIP_NONE:
			m_pDriverData = &DRIVER_PACK_2A03;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_2A03;
			Print(_T(" * No expansion chip\n"));
			break;
		case SNDCHIP_VRC6:
			m_pDriverData = &DRIVER_PACK_VRC6;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_VRC6;
			Print(_T(" * VRC6 expansion enabled\n"));
			break;
		case SNDCHIP_MMC5:
			m_pDriverData = &DRIVER_PACK_MMC5;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_MMC5;
			Print(_T(" * MMC5 expansion enabled\n"));
			break;
		case SNDCHIP_VRC7:
			m_pDriverData = &DRIVER_PACK_VRC7;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_VRC7;
			Print(_T(" * VRC7 expansion enabled\n"));
			break;
		case SNDCHIP_FDS:
			m_pDriverData = &DRIVER_PACK_FDS;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_FDS;
			Print(_T(" * FDS expansion enabled\n"));
			break;
		case SNDCHIP_N163:
			m_pDriverData = &DRIVER_PACK_N163;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_N163;
			Print(_T(" * N163 expansion enabled\n"));
			break;
		case SNDCHIP_S5B:
			m_pDriverData = &DRIVER_PACK_S5B;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_S5B;
			Print(_T(" * S5B expansion enabled\n"));
			break;
		default:		// // // crude, not meant for release
			m_pDriverData = &DRIVER_PACK_ALL;
			m_iVibratoTableLocation = VIBRATO_TABLE_LOCATION_ALL;
			Print(_T(" * Multiple expansion chips enabled\n"));
//			if (m_pDocument->ExpansionEnabled(SNDCHIP_N163))
//				m_pDocument->SetNamcoChannels(8, true);
//			m_pDocument->SelectExpansionChip(0x3F, true);
			break;
	}

	// // // Setup channel order list, DPCM is located last
	const int Channels = m_pDocument->GetAvailableChannels();
	const int Chip = m_pDocument->GetExpansionChip(); // 0CC: use m_iActualChip once cc65 is embedded
	int Channel = 0;
	for (int i = 0; i < 4; i++) {
		int Channel = m_pDocument->GetChannelIndex(CHANID_SQUARE1 + i);
		m_vChanOrder.push_back(Channel);
	}
	if (Chip & SNDCHIP_MMC5) for (int i = 0; i < 2; i++) {
		int Channel = m_pDocument->GetChannelIndex(CHANID_MMC5_SQUARE1 + i);
		m_vChanOrder.push_back(Channel);
	}
	if (Chip & SNDCHIP_VRC6) for (int i = 0; i < 3; i++) {
		int Channel = m_pDocument->GetChannelIndex(CHANID_VRC6_PULSE1 + i);
		m_vChanOrder.push_back(Channel);
	}
	if (Chip & SNDCHIP_N163) {
		int lim = m_iActualNamcoChannels;
//		if (Chip & ~SNDCHIP_N163) lim = 8;
		for (int i = 0; i < lim; i++) { // 0CC: use m_iActualNamcoChannels once cc65 is embedded
			int Channel = m_pDocument->GetChannelIndex(CHANID_N163_CH1 + i);
			m_vChanOrder.push_back(Channel);
		}
	}
	if (Chip & SNDCHIP_FDS) {
		int Channel = m_pDocument->GetChannelIndex(CHANID_FDS);
		m_vChanOrder.push_back(Channel);
	}
	if (Chip & SNDCHIP_S5B) for (int i = 0; i < 3; i++) {
		int Channel = m_pDocument->GetChannelIndex(CHANID_S5B_CH1 + i);
		m_vChanOrder.push_back(Channel);
	}
	if (Chip & SNDCHIP_VRC7) for (int i = 0; i < 6; i++) {
		int Channel = m_pDocument->GetChannelIndex(CHANID_VRC7_CH1 + i);
		m_vChanOrder.push_back(Channel);
	}
	m_vChanOrder.push_back(CHANID_DPCM);

	// Driver size
	m_iDriverSize = m_pDriverData->driver_size;

	// Scan and optimize song
	ScanSong();

	Print(_T("Building music data...\n"));

	// Build music data
	CreateMainHeader();
	CreateSequenceList();
	CreateInstrumentList();
	CreateSampleList();
	StoreSamples();
	StoreGrooves();		// // //
	StoreSongs();

	// Determine if bankswitching is needed
	m_bBankSwitched = false;
	m_iMusicDataSize = CountData();

	// Get samples start address
	m_iSampleStart = m_iDriverSize + m_iMusicDataSize;

	if (m_iSampleStart < 0x4000)
		m_iSampleStart = PAGE_SAMPLES;
	else
		m_iSampleStart += AdjustSampleAddress(m_iSampleStart) + PAGE_START;

	if (m_iSampleStart + m_iSamplesSize > 0xFFFF)
		m_bBankSwitched = true;

	if (m_iSamplesSize > 0x4000)
		m_bBankSwitched = true;

	if ((m_iMusicDataSize + m_iSamplesSize + m_iDriverSize) > 0x8000)
		m_bBankSwitched = true;

	if (m_bBankSwitched)
		m_iSampleStart = PAGE_SAMPLES;

	// Compiling done
	Print(_T(" * Samples located at: $%04X\n"), m_iSampleStart);

#ifdef FORCE_BANKSWITCH
	m_bBankSwitched = true;
#endif /* FORCE_BANKSWITCH */

	return true;
}

void CCompiler::Cleanup()
{
	// Delete objects

	for (CChunk *pChunk : m_vChunks)
		delete pChunk;

	m_vChunks.clear();
	m_vSequenceChunks.clear();
	m_vInstrumentChunks.clear();
	m_vGrooveChunks.clear();		// // //
	m_vSongChunks.clear();
	m_vFrameChunks.clear();
	m_vPatternChunks.clear();

	m_pSamplePointersChunk = NULL;	// This pointer is also stored in m_vChunks
	m_pHeaderChunk = NULL;

	// // // Full chip export
	if (m_pDocument->GetNamcoChannels() != m_iActualNamcoChannels ||
		m_pDocument->GetExpansionChip() != m_iActualChip)
	{
		m_pDocument->SetNamcoChannels(m_iActualNamcoChannels, true);
		m_pDocument->SelectExpansionChip(m_iActualChip, true);
	}
}

void CCompiler::AddBankswitching()
{
	// Add bankswitching data

	for (CChunk *pChunk : m_vChunks) {
		// Frame chunks
		if (pChunk->GetType() == CHUNK_FRAME) {
			int Length = pChunk->GetLength();
			// Bank data is located at end
			for (int j = 0; j < Length; ++j) {
				pChunk->StoreBankReference(pChunk->GetDataRefName(j), 0);
			}
		}
	}

	// Frame lists sizes has changed
	const int TrackCount = m_pDocument->GetTrackCount();
	for (int i = 0; i < TrackCount; ++i) {
		m_iTrackFrameSize[i] += m_pDocument->GetChannelCount() * m_pDocument->GetFrameCount(i);
	}

	// Data size has changed
	m_iMusicDataSize = CountData();
}

void CCompiler::ScanSong()
{
	// Scan and optimize song
	//

	// Re-assign instruments
	m_iInstruments = 0;

	memset(m_iAssignedInstruments, 0, sizeof(int) * MAX_INSTRUMENTS);
	// TODO: remove these
	memset(m_bSequencesUsed2A03, false, sizeof(bool) * MAX_SEQUENCES * SEQ_COUNT);
	memset(m_bSequencesUsedVRC6, false, sizeof(bool) * MAX_SEQUENCES * SEQ_COUNT);
	memset(m_bSequencesUsedN163, false, sizeof(bool) * MAX_SEQUENCES * SEQ_COUNT);
	memset(m_bSequencesUsedS5B, false, sizeof(bool) * MAX_SEQUENCES * SEQ_COUNT);		// // //
	
	static const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //
	bool *used[] = {*m_bSequencesUsed2A03, *m_bSequencesUsedVRC6, *m_bSequencesUsedN163, *m_bSequencesUsedS5B};

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pDocument->IsInstrumentUsed(i) && IsInstrumentInPattern(i)) {
			
			// List of used instruments
			m_iAssignedInstruments[m_iInstruments++] = i;
			
			// Create a list of used sequences
			inst_type_t it = m_pDocument->GetInstrumentType(i);		// // //
			for (size_t z = 0; z < sizeof(used) / sizeof(bool*); z++) if (it == inst[z]) {
				auto pInstrument = std::static_pointer_cast<CSeqInstrument>(m_pDocument->GetInstrument(i));
				for (int j = 0; j < SEQ_COUNT; ++j) if (pInstrument->GetSeqEnable(j))
					*(used[z] + pInstrument->GetSeqIndex(j) * SEQ_COUNT + j) = true;
				break;
			}
		}
	}

	// See which samples are used
	m_iSamplesUsed = 0;

	memset(m_bSamplesAccessed, 0, MAX_INSTRUMENTS * OCTAVE_RANGE * NOTE_RANGE * sizeof(bool));

	// Get DPCM channel index
	const int DpcmChannel = m_pDocument->GetChannelIndex(CHANID_DPCM);
	const int TrackCount = m_pDocument->GetTrackCount();
	unsigned int Instrument = 0;

	for (int i = 0; i < TrackCount; ++i) {
		const int patternlen = m_pDocument->GetPatternLength(i);
		const int frames = m_pDocument->GetFrameCount(i);
		for (int j = 0; j < frames; ++j) {
			int p = m_pDocument->GetPatternAtFrame(i, j, DpcmChannel);
			for (int k = 0; k < patternlen; ++k) {
				stChanNote Note;
				m_pDocument->GetDataAtPattern(i, p, DpcmChannel, k, &Note);
				if (Note.Instrument < MAX_INSTRUMENTS)
					Instrument = Note.Instrument;
				if (Note.Note > 0) {
					m_bSamplesAccessed[Instrument][Note.Octave][Note.Note - 1] = true;
				}
			}
		}
	}
}

bool CCompiler::IsInstrumentInPattern(int index) const
{
	// Returns true if the instrument is used in a pattern

	const int TrackCount = m_pDocument->GetTrackCount();
	const int Channels = m_pDocument->GetAvailableChannels();

	// Scan patterns in entire module
	for (int i = 0; i < TrackCount; ++i) {
		int PatternLength = m_pDocument->GetPatternLength(i);
		for (int j = 0; j < Channels; ++j) {
			for (int k = 0; k < MAX_PATTERN; ++k) {
				for (int l = 0; l < PatternLength; ++l) {
					stChanNote Note;
					m_pDocument->GetDataAtPattern(i, k, j, l, &Note);
					if (Note.Instrument == index)
						return true;
				}
			}
		}
	}	

	return false;
}

void CCompiler::CreateMainHeader()
{
	// Writes the music header
	int TicksPerSec = m_pDocument->GetEngineSpeed();

	int Chip = m_pDocument->GetExpansionChip();		// // //
	bool bMultichip = (Chip & (Chip - 1)) != 0;

	unsigned short DividerNTSC, DividerPAL;

	CChunk *pChunk = CreateChunk(CHUNK_HEADER, "");

	if (TicksPerSec == 0) {
		// Default
		DividerNTSC = CAPU::FRAME_RATE_NTSC * 60;
		DividerPAL	= CAPU::FRAME_RATE_PAL * 60;
	}
	else {
		// Custom
		DividerNTSC = TicksPerSec * 60;
		DividerPAL = TicksPerSec * 60;
	}

	unsigned char Flags = 0; // bankswitch flag is set later
	if (m_pDocument->GetVibratoStyle() == VIBRATO_OLD) Flags |= FLAG_VIBRATO;
	if (m_pDocument->GetLinearPitch()) Flags |= FLAG_LINEARPITCH;		// // //

	// Write header

	pChunk->StoreReference(CChunkRenderText::LABEL_SONG_LIST);
	pChunk->StoreReference(CChunkRenderText::LABEL_INSTRUMENT_LIST);
	pChunk->StoreReference(CChunkRenderText::LABEL_SAMPLES_LIST);
	pChunk->StoreReference(CChunkRenderText::LABEL_SAMPLES);
	pChunk->StoreReference(CChunkRenderText::LABEL_GROOVE_LIST);		// // //
	
	m_iHeaderFlagOffset = pChunk->GetLength();		// Save the flags offset
	pChunk->StoreByte(Flags);

	// FDS table, only if FDS is enabled
	if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS) || bMultichip)
		pChunk->StoreReference(CChunkRenderText::LABEL_WAVETABLE);

	pChunk->StoreWord(DividerNTSC);
	pChunk->StoreWord(DividerPAL);

	// N163 channel count
	if (m_pDocument->ExpansionEnabled(SNDCHIP_N163) || bMultichip) {
		/*if (m_pDocument->GetExpansionChip() != SNDCHIP_N163)		// // //
			pChunk->StoreByte(8);
		else*/ pChunk->StoreByte(std::max(m_iActualNamcoChannels, 1));
	}

	m_pHeaderChunk = pChunk;
}

// Sequences

void CCompiler::CreateSequenceList()
{
	// Create sequence lists
	//

	unsigned int Size = 0, StoredCount = 0;
	static const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};
	const bool *used[] = {*m_bSequencesUsed2A03, *m_bSequencesUsedVRC6, *m_bSequencesUsedN163, *m_bSequencesUsedS5B};
	static const char *format[] = {
		CChunkRenderText::LABEL_SEQ_2A03, CChunkRenderText::LABEL_SEQ_VRC6,
		CChunkRenderText::LABEL_SEQ_N163, CChunkRenderText::LABEL_SEQ_S5B
	};

	// TODO: use the CSeqInstrument::GetSequence
	// TODO: merge identical sequences from all chips
	for (size_t c = 0; c < sizeof(inst) / sizeof(inst_type_t); c++) {
		for (int i = 0; i < MAX_SEQUENCES; ++i)  for (int j = 0; j < SEQ_COUNT; ++j) {
			CSequence* pSeq = m_pDocument->GetSequence(inst[c], i, j);
			int Index = i * SEQ_COUNT + j;
			if (*(used[c] + Index) && pSeq->GetItemCount() > 0) {
				CStringA label;
				label.Format(format[c], Index);
				Size += StoreSequence(pSeq, label);
				++StoredCount;
			}
		}
	}

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (auto pInstrument = std::dynamic_pointer_cast<CInstrumentFDS>(m_pDocument->GetInstrument(i))) {
			for (int j = 0; j < CInstrumentFDS::SEQUENCE_COUNT; ++j) {
				const CSequence* pSeq = pInstrument->GetSequence(j);		// // //
				if (pSeq->GetItemCount() > 0) {
					int Index = i * SEQ_COUNT + j;
					CStringA label;
					label.Format(CChunkRenderText::LABEL_SEQ_FDS, Index);
					Size += StoreSequence(pSeq, label);
					++StoredCount;
				}
			}
		}
	}

	Print(_T(" * Sequences used: %i (%i bytes)\n"), StoredCount, Size);
}

int CCompiler::StoreSequence(const CSequence *pSeq, CStringA &label)
{
	CChunk *pChunk = CreateChunk(CHUNK_SEQUENCE, label);
	m_vSequenceChunks.push_back(pChunk);

	// Store the sequence
	int iItemCount	  = pSeq->GetItemCount();
	int iLoopPoint	  = pSeq->GetLoopPoint();
	int iReleasePoint = pSeq->GetReleasePoint();
	int iSetting	  = pSeq->GetSetting();

	if (iReleasePoint != -1)
		iReleasePoint += 1;
	else
		iReleasePoint = 0;

	if (iLoopPoint > iItemCount)
		iLoopPoint = -1;

	pChunk->StoreByte((unsigned char)iItemCount);
	pChunk->StoreByte((unsigned char)iLoopPoint);
	pChunk->StoreByte((unsigned char)iReleasePoint);
	pChunk->StoreByte((unsigned char)iSetting);

	for (int i = 0; i < iItemCount; ++i) {
		pChunk->StoreByte(pSeq->GetItem(i));
	}

	// Return size of this chunk
	return iItemCount + 4;
}

// Instruments

void CCompiler::CreateInstrumentList()
{
	/*
	 * Create the instrument list
	 *
	 * The format of instruments depends on the type
	 *
	 */

	unsigned int iTotalSize = 0;	
	CChunk *pWavetableChunk = NULL;	// FDS
	CChunk *pWavesChunk = NULL;		// N163
	int iWaveSize = 0;				// N163 waves size

	CChunk *pInstListChunk = CreateChunk(CHUNK_INSTRUMENT_LIST, CChunkRenderText::LABEL_INSTRUMENT_LIST);
	
	if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS)) {
		pWavetableChunk = CreateChunk(CHUNK_WAVETABLE, CChunkRenderText::LABEL_WAVETABLE);
	}

	memset(m_iWaveBanks, -1, MAX_INSTRUMENTS * sizeof(int));

	// Collect N163 waves
	for (unsigned int i = 0; i < m_iInstruments; ++i) {
		int iIndex = m_iAssignedInstruments[i];
		if (m_pDocument->GetInstrumentType(iIndex) == INST_N163 && m_iWaveBanks[i] == -1) {
			auto pInstrument = std::static_pointer_cast<CInstrumentN163>(m_pDocument->GetInstrument(iIndex));
			for (unsigned int j = i + 1; j < m_iInstruments; ++j) {
				int inst = m_iAssignedInstruments[j];
				if (m_pDocument->GetInstrumentType(inst) == INST_N163 && m_iWaveBanks[j] == -1) {
					auto pNewInst = std::static_pointer_cast<CInstrumentN163>(m_pDocument->GetInstrument(inst));
					if (pInstrument->IsWaveEqual(pNewInst.get())) {
						m_iWaveBanks[j] = iIndex;
					}
				}
			}
			if (m_iWaveBanks[i] == -1) {
				m_iWaveBanks[i] = iIndex;
				// Store wave
				CStringA label;
				label.Format(CChunkRenderText::LABEL_WAVES, iIndex);
				pWavesChunk = CreateChunk(CHUNK_WAVES, label);
				// Store waves
				iWaveSize += pInstrument->StoreWave(pWavesChunk);
			}
		}
	}

	// Store instruments
	for (unsigned int i = 0; i < m_iInstruments; ++i) {
		// Add reference to instrument list
		CStringA label;
		label.Format(CChunkRenderText::LABEL_INSTRUMENT, i);
		pInstListChunk->StoreReference(label);
		iTotalSize += 2;

		// Actual instrument
		CChunk *pChunk = CreateChunk(CHUNK_INSTRUMENT, label);
		m_vInstrumentChunks.push_back(pChunk);

		int iIndex = m_iAssignedInstruments[i];
		auto pInstrument = m_pDocument->GetInstrument(iIndex);
/*
		if (pInstrument->GetType() == INST_N163) {
			CString label;
			label.Format(LABEL_WAVES, iIndex);
			pWavesChunk = CreateChunk(CHUNK_WAVES, label);
			// Store waves
			iWaveSize += ((CInstrumentN163*)pInstrument)->StoreWave(pWavesChunk);
		}
*/

		if (pInstrument->GetType() == INST_N163) {
			// Translate wave index
			iIndex = m_iWaveBanks[i];
		}

		// Returns number of bytes 
		iTotalSize += pInstrument->Compile(pChunk, iIndex);		// // //

		// // // Check if FDS
		if (pInstrument->GetType() == INST_FDS && pWavetableChunk != NULL) {
			// Store wave
			AddWavetable(std::static_pointer_cast<CInstrumentFDS>(pInstrument).get(), pWavetableChunk);
			pChunk->StoreByte(m_iWaveTables - 1);
		}
	}

	Print(_T(" * Instruments used: %i (%i bytes)\n"), m_iInstruments, iTotalSize);

	if (iWaveSize > 0)
		Print(_T(" * N163 waves size: %i bytes\n"), iWaveSize);
}

// Samples

void CCompiler::CreateSampleList()
{
	/*
	 * DPCM instrument list
	 *
	 * Each item is stored as a pair of the sample pitch and pointer to the sample table
	 *
	 */

	const int SAMPLE_ITEM_WIDTH = 3;	// 3 bytes / sample item

	// Clear the sample list
	memset(m_iSampleBank, 0xFF, MAX_DSAMPLES);
	
	CChunk *pChunk = CreateChunk(CHUNK_SAMPLE_LIST, CChunkRenderText::LABEL_SAMPLES_LIST);

	// Store sample instruments
	unsigned int Item = 0;
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pDocument->IsInstrumentUsed(i) && m_pDocument->GetInstrumentType(i) == INST_2A03) {
			auto pInstrument = std::static_pointer_cast<CInstrument2A03>(m_pDocument->GetInstrument(i));

			for (int j = 0; j < OCTAVE_RANGE; ++j) {
				for (int k = 0; k < NOTE_RANGE; ++k) {
					// Get sample
					unsigned char iSample = pInstrument->GetSampleIndex(j, k);
					if ((iSample > 0) && m_bSamplesAccessed[i][j][k] && m_pDocument->IsSampleUsed(iSample - 1)) {

						unsigned char SamplePitch = pInstrument->GetSamplePitch(j, k);
						unsigned char SampleIndex = GetSampleIndex(iSample - 1);
						unsigned int  SampleDelta = pInstrument->GetSampleDeltaValue(j, k);
						SamplePitch |= (SamplePitch & 0x80) >> 1;

						// Save a reference to this item
						m_iSamplesLookUp[i][j][k] = ++Item;

						pChunk->StoreByte(SamplePitch);
						pChunk->StoreByte(SampleDelta);
						pChunk->StoreByte(SampleIndex * SAMPLE_ITEM_WIDTH);
					}
					else
						// No instrument here
						m_iSamplesLookUp[i][j][k] = 0;
				}
			}
		}
	}
}

void CCompiler::StoreSamples()
{
	/*
	 * DPCM sample list
	 *
	 * Each sample is stored as a pair of the sample address and sample size
	 *
	 */

	unsigned int iAddedSamples = 0;
	unsigned int iSampleAddress = 0x0000;

	// Get sample start address
	m_iSamplesSize = 0;

	CChunk *pChunk = CreateChunk(CHUNK_SAMPLE_POINTERS, CChunkRenderText::LABEL_SAMPLES);
	m_pSamplePointersChunk = pChunk;

	// Store DPCM samples in a separate array
	for (unsigned int i = 0; i < m_iSamplesUsed; ++i) {

		unsigned int iIndex = m_iSampleBank[i];
		ASSERT(iIndex != 0xFF);
		const CDSample *pDSample = m_pDocument->GetSample(iIndex);
		unsigned int iSize = pDSample->GetSize();

		if (iSize > 0) {
			// Fill sample list
			unsigned char iSampleAddr = iSampleAddress >> 6;
			unsigned char iSampleSize = iSize >> 4;
			unsigned char iSampleBank = 0;

			// Update SAMPLE_ITEM_WIDTH here
			pChunk->StoreByte(iSampleAddr);
			pChunk->StoreByte(iSampleSize);
			pChunk->StoreByte(iSampleBank);

			// Add this sample to storage
			m_vSamples.push_back(pDSample);

			// Pad end of samples
			unsigned int iAdjust = AdjustSampleAddress(iSampleAddress + iSize);

			iAddedSamples++;
			iSampleAddress += iSize + iAdjust;
			m_iSamplesSize += iSize + iAdjust;
		}
	}

	Print(_T(" * DPCM samples used: %i (%i bytes)\n"), m_iSamplesUsed, m_iSamplesSize);
}

int CCompiler::GetSampleIndex(int SampleNumber)
{
	// Returns a sample pos from the sample bank
	for (int i = 0; i < MAX_DSAMPLES; i++) {
		if (m_iSampleBank[i] == SampleNumber)
			return i;							// Sample is already stored
		else if(m_iSampleBank[i] == 0xFF) {
			m_iSampleBank[i] = SampleNumber;	// Allocate new position
			m_iSamplesUsed++;
			return i;
		}
	}

	// TODO: Fail if getting here!!!
	return SampleNumber;
}

// // // Groove list

void CCompiler::StoreGrooves()
{
	/*
	 * Store grooves
	 */

	unsigned int Size = 1, Count = 0;
	
	CChunk *pGrooveListChunk = CreateChunk(CHUNK_GROOVE_LIST, CChunkRenderText::LABEL_GROOVE_LIST);
	pGrooveListChunk->StoreByte(0); // padding; possibly used to disable groove

	for (int i = 0; i < MAX_GROOVE; i++) {
		unsigned int Pos = Size;
		CGroove *Groove = m_pDocument->GetGroove(i);
		if (Groove == NULL) continue;
		
		CStringA label;
		label.Format(CChunkRenderText::LABEL_GROOVE, i);
		// pGrooveListChunk->StoreReference(label);

		CChunk *pChunk = CreateChunk(CHUNK_GROOVE, label);
		m_vGrooveChunks.push_back(pChunk);
		for (int j = 0; j < Groove->GetSize(); j++) {
			pChunk->StoreByte(Groove->GetEntry(j));
			Size++;
		}
		pChunk->StoreByte(0);
		pChunk->StoreByte(Pos);
		Size += 2;
		Count++;
	}

	Print(" * Grooves used: %i (%i bytes)\n", Count, Size);
}

// Songs

void CCompiler::StoreSongs()
{
	/*
	 * Store patterns and frames for each song
	 * 
	 */

	const int TrackCount = m_pDocument->GetTrackCount();

	CChunk *pSongListChunk = CreateChunk(CHUNK_SONG_LIST, CChunkRenderText::LABEL_SONG_LIST);

	m_iDuplicatePatterns = 0;

	// Store song info
	for (int i = 0; i < TrackCount; ++i) {
		// Add reference to song list
		CStringA label;
		label.Format(CChunkRenderText::LABEL_SONG, i);
		pSongListChunk->StoreReference(label);

		// Create song
		CChunk *pChunk = CreateChunk(CHUNK_SONG, label);
		m_vSongChunks.push_back(pChunk);

		// Store reference to song
		label.Format(CChunkRenderText::LABEL_SONG_FRAMES, i);
		pChunk->StoreReference(label);
		pChunk->StoreByte(m_pDocument->GetFrameCount(i));
		pChunk->StoreByte(m_pDocument->GetPatternLength(i));

		if (m_pDocument->GetSongGroove(i))		// // //
			if (m_pDocument->GetGroove(m_pDocument->GetSongSpeed(i)) != NULL)
				pChunk->StoreByte(0);
			else pChunk->StoreByte(DEFAULT_SPEED);
		else pChunk->StoreByte(m_pDocument->GetSongSpeed(i));

		pChunk->StoreByte(m_pDocument->GetSongTempo(i));

		if (m_pDocument->GetSongGroove(i) && m_pDocument->GetGroove(m_pDocument->GetSongSpeed(i)) != NULL) {		// // //
			int Pos = 1;
			for (unsigned int j = 0; j < m_pDocument->GetSongSpeed(i); j++)
				if (m_pDocument->GetGroove(j) != NULL) Pos += m_pDocument->GetGroove(j)->GetSize() + 2;
			pChunk->StoreByte(Pos);
		}
		else pChunk->StoreByte(0);

		pChunk->StoreBankReference(label, 0);
	}

	m_iSongBankReference = m_vSongChunks[0]->GetLength() - 1;	// Save bank value position (all songs are equal)

	// Store actual songs
	for (int i = 0; i < TrackCount; ++i) {
		Print(_T(" * Song %i: "), i);
		// Store frames
		CreateFrameList(i);
		// Store pattern data
		StorePatterns(i);
	}

	if (m_iDuplicatePatterns > 0)
		Print(_T(" * %i duplicated pattern(s) removed\n"), m_iDuplicatePatterns);
	
#ifdef _DEBUG
	Print(_T("Hash collisions: %i (of %i items)\r\n"), m_iHashCollisions, m_PatternMap.GetCount());
#endif
}

// Frames

void CCompiler::CreateFrameList(unsigned int Track)
{
	/*
	 * Creates a frame list
	 *
	 * The pointer list is just pointing to each item in the frame list
	 * and the frame list holds the offset addresses for the patterns for all channels
	 *
	 * ---------------------
	 *  Frame entry pointers
	 *  $XXXX (2 bytes, offset to a frame entry)
	 *  ...
	 * ---------------------
	 *
	 * ---------------------
	 *  Frame entries
	 *  $XXXX * 4 (2 * 2 bytes, each pair is an offset to the pattern)
	 * ---------------------
	 *
	 */
	
	const int FrameCount   = m_pDocument->GetFrameCount(Track);
	const int ChannelCount = m_pDocument->GetAvailableChannels();

	// Create frame list
	CStringA label;
	label.Format(CChunkRenderText::LABEL_SONG_FRAMES, Track);
	CChunk *pFrameListChunk = CreateChunk(CHUNK_FRAME_LIST, label);

	unsigned int TotalSize = 0;

	// Store addresses to patterns
	for (int i = 0; i < FrameCount; ++i) {
		// Add reference to frame list
		label.Format(CChunkRenderText::LABEL_SONG_FRAME, Track, i);
		pFrameListChunk->StoreReference(label);
		TotalSize += 2;

		// Store frame item
		CChunk *pChunk = CreateChunk(CHUNK_FRAME, label);
		m_vFrameChunks.push_back(pChunk);

		// Pattern pointers
		for (int j = 0; j < ChannelCount; ++j) {
			int Chan = m_vChanOrder[j];
			int Pattern = m_pDocument->GetPatternAtFrame(Track, i, Chan);
			label.Format(CChunkRenderText::LABEL_PATTERN, Track, Pattern, Chan);
			pChunk->StoreReference(label);
			TotalSize += 2;
		}
	}

	m_iTrackFrameSize[Track] = TotalSize;

	Print(_T("%i frames (%i bytes), "), FrameCount, TotalSize);
}

// Patterns

void CCompiler::StorePatterns(unsigned int Track)
{
	/* 
	 * Store patterns and save references to them for the frame list
	 * 
	 */

	const int iChannels = m_pDocument->GetAvailableChannels();

	CPatternCompiler PatternCompiler(m_pDocument, m_iAssignedInstruments, (DPCM_List_t*)&m_iSamplesLookUp, m_pLogger);

	int PatternCount = 0;
	int PatternSize = 0;

	// Iterate through all patterns
	for (int i = 0; i < MAX_PATTERN; ++i) {
		for (int j = 0; j < iChannels; ++j) {
			// And store only used ones
			if (IsPatternAddressed(Track, i, j)) {

				// Compile pattern data
				PatternCompiler.CompileData(Track, i, j);

				CStringA label;
				label.Format(CChunkRenderText::LABEL_PATTERN, Track, i, j);

				bool StoreNew = true;

#ifdef REMOVE_DUPLICATE_PATTERNS
				unsigned int Hash = PatternCompiler.GetHash();
				
				// Check for duplicate patterns
				CChunk *pDuplicate = m_PatternMap[Hash];

				if (pDuplicate != NULL) {
					// Hash only indicates that patterns may be equal, check exact data
					if (PatternCompiler.CompareData(pDuplicate->GetStringData(PATTERN_CHUNK_INDEX))) {
						// Duplicate was found, store a reference to existing pattern
						m_DuplicateMap[label] = pDuplicate->GetLabel();
						++m_iDuplicatePatterns;
						StoreNew = false;
					}
				}
#endif /* REMOVE_DUPLICATE_PATTERNS */

				if (StoreNew) {
					// Store new pattern
					CChunk *pChunk = CreateChunk(CHUNK_PATTERN, label);
					m_vPatternChunks.push_back(pChunk);

#ifdef REMOVE_DUPLICATE_PATTERNS
					if (m_PatternMap[Hash] != NULL)
						m_iHashCollisions++;
					m_PatternMap[Hash] = pChunk;
#endif /* REMOVE_DUPLICATE_PATTERNS */
					
					// Store pattern data as string
					pChunk->StoreString(PatternCompiler.GetData());

					PatternSize += PatternCompiler.GetDataSize();
					++PatternCount;
				}
			}
		}
	}

#ifdef REMOVE_DUPLICATE_PATTERNS
	// Update references to duplicates
	for (const auto pChunk : m_vFrameChunks) {
		for (int j = 0, n = pChunk->GetLength(); j < n; ++j) {
			CStringA str = m_DuplicateMap[pChunk->GetDataRefName(j)];
			if (str.GetLength() != 0) {
				// Update reference
				pChunk->UpdateDataRefName(j, str);
			}
		}
	}
#endif /* REMOVE_DUPLICATE_PATTERNS */

#ifdef LOCAL_DUPLICATE_PATTERN_REMOVAL
	// Forget patterns when one whole track is stored
	m_PatternMap.RemoveAll();
	m_DuplicateMap.RemoveAll();
#endif /* LOCAL_DUPLICATE_PATTERN_REMOVAL */

	Print(_T("%i patterns (%i bytes)\r\n"), PatternCount, PatternSize);
}

bool CCompiler::IsPatternAddressed(unsigned int Track, int Pattern, int Channel) const
{
	// Scan the frame list to see if a pattern is accessed for that frame
	const int FrameCount = m_pDocument->GetFrameCount(Track);
	
	for (int i = 0; i < FrameCount; ++i) {
		if (m_pDocument->GetPatternAtFrame(Track, i, Channel) == Pattern)
			return true;
	}

	return false;
}

void CCompiler::AddWavetable(CInstrumentFDS *pInstrument, CChunk *pChunk)
{
	// TODO Find equal existing waves
	/*
	for (int i = 0; i < m_iWaveTables; ++i) {
		if (!memcmp(Wave, m_iWaveTable[i], 64))
			return i;
	}
	*/

	// Allocate new wave
	for (int i = 0; i < 64; ++i)
		pChunk->StoreByte(pInstrument->GetSample(i));

	m_iWaveTables++;
}

void CCompiler::WriteAssembly(CFile *pFile)
{
	// Dump all chunks and samples as assembly text
	CChunkRenderText Render(pFile);
	Render.StoreChunks(m_vChunks);
	Print(_T(" * Music data size: %i bytes\n"), m_iMusicDataSize);
	Render.StoreSamples(m_vSamples);
	Print(_T(" * DPCM samples size: %i bytes\n"), m_iSamplesSize);
}

void CCompiler::WritePeriods(CFile* pFile)
{
	unsigned int length = 0;
	const auto WriteString = [&pFile, &length](CString str) {
		pFile->Write(const_cast<CStringA&>(str).GetBuffer(), str.GetLength());
		length += str.GetLength();
	};

	const auto DumpFunc = [WriteString](unsigned int* Table) {
		for (int i = 0; i < NOTE_COUNT; ++i) {
			unsigned int Val = Table[i] & 0xFFFF;
			CString str;
			if (i < NOTE_COUNT - 1) {
				if (i % NOTE_RANGE < NOTE_RANGE - 1)
					str.Format("$%04X, ", Val);
				else
					str.Format("$%04X\n\t.word\t", Val);
			}
			else
				str.Format("$%04X\n", Val);
			WriteString(str);
		}
	};

	unsigned int NoteLookupTableNTSC[NOTE_COUNT];
	unsigned int NoteLookupTablePAL[NOTE_COUNT];
	unsigned int NoteLookupTableSaw[NOTE_COUNT];
	unsigned int NoteLookupTableFDS[NOTE_COUNT];
	unsigned int NoteLookupTableN163[NOTE_COUNT];
	unsigned int NoteLookupTableVRC7[NOTE_COUNT];

	CSoundGen* pSoundGen = theApp.GetSoundGenerator();
	for (unsigned int i = 0; i < CDetuneTable::DETUNE_N163; ++i) {		// // //
		switch (i) {
		case CDetuneTable::DETUNE_NTSC:
			for (int j = 0; j < NOTE_COUNT; ++j) NoteLookupTableNTSC[j] = pSoundGen->ReadPeriodTable(j, i); break;
		case CDetuneTable::DETUNE_PAL:
			for (int j = 0; j < NOTE_COUNT; ++j) NoteLookupTablePAL[j] = pSoundGen->ReadPeriodTable(j, i); break;
		case CDetuneTable::DETUNE_SAW:
			for (int j = 0; j < NOTE_COUNT; ++j) NoteLookupTableSaw[j] = pSoundGen->ReadPeriodTable(j, i); break;
		case CDetuneTable::DETUNE_VRC7:
			for (int j = 0; j < NOTE_COUNT; ++j) NoteLookupTableVRC7[j] = pSoundGen->ReadPeriodTable(j, i); break;
		case CDetuneTable::DETUNE_FDS:
			for (int j = 0; j < NOTE_COUNT; ++j) NoteLookupTableFDS[j] = pSoundGen->ReadPeriodTable(j, i); break;
		case CDetuneTable::DETUNE_N163:
			for (int j = 0; j < NOTE_COUNT; ++j) NoteLookupTableN163[j] = pSoundGen->ReadPeriodTable(j, i); break;
		default:
			AfxDebugBreak();
		}
	}

	// One possible optimization is to store the PAL table as the difference from the NTSC table

	WriteString("; 2A03 NTSC\n");
	WriteString(".if .defined(NTSC_PERIOD_TABLE)\n");
	WriteString("ft_periods_ntsc: ;; Patch\n\t.word\t");
	DumpFunc(NoteLookupTableNTSC);

	WriteString(".endif\n\n");
	WriteString("; 2A03 PAL\n");
	WriteString(".if .defined(PAL_PERIOD_TABLE)\n");
	WriteString("ft_periods_pal: ;; Patch\n\t.word\t");
	DumpFunc(NoteLookupTablePAL);

	WriteString(".endif\n\n");
	WriteString("; VRC6 Sawtooth\n");
	WriteString(".if .defined(USE_VRC6)\n");
	WriteString("ft_periods_sawtooth: ;; Patch\n\t.word\t");
	DumpFunc(NoteLookupTableSaw);

	WriteString(".endif\n\n");
	WriteString("; FDS\n");
	WriteString(".if .defined(USE_FDS)\n");
	WriteString("ft_periods_fds: ;; Patch\n\t.word\t");
	DumpFunc(NoteLookupTableFDS);

	WriteString(".endif\n\n");
	WriteString("; N163\n");
	WriteString(".if .defined(USE_N163)\n");
	WriteString("ft_periods_n163: ;; Patch\n\t.word\t");
	DumpFunc(NoteLookupTableN163);

	WriteString(".endif\n\n");
	WriteString("; VRC7\n");
	WriteString(".if .defined(USE_VRC7)\n");
	WriteString("; Fnum table, multiplied by 4 for higher resolution\n");
	WriteString(".define ft_vrc7_table ");

	for (int i = 0; i <= NOTE_RANGE; ++i) {		// // // include last item for linear pitch code optimization
		CString str;
		if (i == NOTE_RANGE)
			str.Format("$%04X\n\n", NoteLookupTableVRC7[0] << 3);
		else
			str.Format("$%04X, ", NoteLookupTableVRC7[i] << 2);
		WriteString(str);
	}

	WriteString("ft_note_table_vrc7_l: ;; Patch\n");
	WriteString("\t.lobytes ft_vrc7_table\n");
	WriteString("ft_note_table_vrc7_h:\n");
	WriteString("\t.hibytes ft_vrc7_table\n");
	WriteString(".endif\n");

	Print(_T(" * Period table size: %i bytes\n"), length);
}

void CCompiler::WriteVibrato(CFile* pFile)
{
	unsigned int length = 0;
	const auto WriteString = [&pFile, &length](CString str) {
		pFile->Write(const_cast<CStringA&>(str).GetBuffer(), str.GetLength());
		length += str.GetLength();
	};

	unsigned int VibratoTable[VIBRATO_LENGTH];

	// Copy the vibrato table, the stock one only works for new vibrato mode
	const CSoundGen* pSoundGen = theApp.GetSoundGenerator();

	for (int i = 0; i < 256; ++i) {
		VibratoTable[i] = pSoundGen->ReadVibratoTable(i);
	}

	WriteString("; Vibrato table (256 bytes)\n"
		"ft_vibrato_table: ;; Patch\n");
	for (int i = 0; i < 16; i++) {	// depth
		WriteString("\t.byte ");
		for (int j = 0; j < 16; j++) {	// phase
			CString b;
			b.Format("$%02X%s", VibratoTable[i * 16 + j], j < 15 ? ", " : "");
			WriteString(b);
		}
		WriteString("\n");
	}

	Print(_T(" * Vibrato table size: %i bytes\n"), length);
}

void CCompiler::WriteNSFHeader(CFile* pFile, stNSFHeader Header)
{
	CString str;
	unsigned int length = 0;
	const auto WriteString = [&pFile, &length](CString str) {
		pFile->Write(const_cast<CStringA&>(str).GetBuffer(), str.GetLength());
		length += str.GetLength();
	};

	const auto WriteByte = [&WriteString](unsigned int Byte) {
		CString str;
		str.Format("$%02X", Byte);
		WriteString(str);
	};

	const auto WriteWord = [&WriteString](unsigned int Word) {
		CString str;
		str.Format("$%04X", Word);
		WriteString(str);
	};

	const auto WriteInt = [&WriteString](unsigned int Int) {
		CString str;
		str.Format("%i", Int);
		WriteString(str);
	};

	const auto WriteNSFIdent = [&WriteString](unsigned char* Ident) {
		for (int i = 0; i < 5; ++i) {
			CString str;
			str.Format("$%02X%s", Ident[i], i < 4 ? ", " : "");
			WriteString(str);
		}
	};

	const auto WriteBankValues = [&WriteString](unsigned char* BankValues) {
		for (int i = 0; i < 8; ++i) {
			CString str;
			str.Format("%i%s", BankValues[i], i < 7 ? ", " : "");
			WriteString(str);
		}
	};

	const auto WriteNSFLength = [&WriteString](unsigned char* Length) {
		for (int i = 0; i < 3; ++i) {
			CString str;
			str.Format("$%02X%s", Length[i], i < 2 ? ", " : "");
			WriteString(str);
		}
	};

	WriteString(";\n; NSF Header\n;\n");
	if (Header.SoundChip & SNDCHIP_VRC6) WriteString("USE_VRC6 = 1\n");
	if (Header.SoundChip & SNDCHIP_VRC7) WriteString("USE_VRC7 = 1\n");
	if (Header.SoundChip & SNDCHIP_FDS) WriteString("USE_FDS = 1\n");
	if (Header.SoundChip & SNDCHIP_MMC5) WriteString("USE_MMC5 = 1\n");
	if (Header.SoundChip & SNDCHIP_N163) WriteString("USE_N163 = 1\n");
	if (Header.SoundChip & SNDCHIP_S5B) WriteString("USE_S5B = 1\n");
	if (m_bBankSwitched) WriteString("USE_BANKSWITCH = 1\n");
	if (m_pDocument->GetVibratoStyle() == VIBRATO_OLD) WriteString("USE_OLDVIBRATO = 1\n");
	if (m_pDocument->GetLinearPitch()) WriteString("USE_LINEARPITCH = 1\n");
	WriteString("\n.segment \"HEADER1\"\n");
	WriteString(".byte "); WriteNSFIdent(Header.Ident);
	WriteString("\t; ID\n");
	WriteString(".byte $01\t\t\t\t\t\t; Version\n");
	WriteString(".byte "); WriteInt(Header.TotalSongs);
	WriteString("\t\t\t\t\t\t\t; Number of songs\n");
	WriteString(".byte "); WriteInt(Header.StartSong);
	WriteString("\t\t\t\t\t\t\t; Start song\n");
	WriteString(".word LOAD\t\t\t\t\t\t; LOAD address\n");
	WriteString(".word INIT\t\t\t\t\t\t; INIT address\n");
	WriteString(".word PLAY\t\t\t\t\t\t; PLAY address\n\n");

	WriteString(".segment \"HEADER2\"\n");
	WriteString(".asciiz \""); WriteString(static_cast<CString>(Header.SongName));
	WriteString("\"\t; Name, 32 bytes\n\n");

	WriteString(".segment \"HEADER3\"\n");
	WriteString(".asciiz \""); WriteString(static_cast<CString>(Header.ArtistName));
	WriteString("\"\t; Artist, 32 bytes\n\n");

	WriteString(".segment \"HEADER4\"\n");
	WriteString(".asciiz \""); WriteString(static_cast<CString>(Header.Copyright));
	WriteString("\"\t; Copyright, 32 bytes\n\n");

	WriteString(".segment \"HEADER5\"\n");
	WriteString(".word "); WriteWord(Header.Speed_NTSC);
	WriteString("\t\t\t\t\t\t; NTSC speed\n");
	WriteString(".byte "); WriteBankValues(Header.BankValues);
	WriteString("\t; Bank values\n");
	WriteString(".word "); WriteWord(Header.Speed_PAL);
	WriteString("\t\t\t\t\t\t; PAL speed\n");
	WriteString(".byte "); WriteInt(Header.Flags);
	WriteString("\t\t\t\t\t\t\t; Flags, NTSC\n");
	WriteString(".byte EXPANSION_FLAG\t\t\t; Expansion audio flags\n");
	WriteString(".byte "); WriteByte(Header.NSF2Flags);
	WriteString("\t\t\t\t\t\t; NSF2 flags\n");
	WriteString(".byte "); WriteNSFLength(Header.NSFDataLength);
	WriteString("\t\t\t\t; NSF data length\n");

	Print(_T(" * NSF header size: %i bytes\n"), length);
}

void CCompiler::WriteNSFConfig(CFile* pFile, unsigned int DPCMSegment, stNSFHeader Header)
{
	CString str;
	unsigned int length = 0;
	const auto WriteString = [&pFile, &length](CString str) {
		pFile->Write(const_cast<CStringA&>(str).GetBuffer(), str.GetLength());
		length += str.GetLength();
	};

	const auto WriteWord = [&WriteString](unsigned int Word) {
		CString str;
		str.Format("$%04X", Word);
		WriteString(str);
	};

	WriteString("MEMORY {\n");
	WriteString("  ZP:  start = $00, size = $100, type = rw, file = \"\";\n");
	WriteString("  RAM: start = $200,  size = $600,   type = rw, file = \"\";\n");
	WriteString("  HDR: start = $00,   size = $80,    type = ro, file = %O;\n");
	WriteString("  PRG: start = $8000, size = $40000, type = ");
	WriteString(Header.SoundChip & SNDCHIP_FDS ? "rw" : "ro");
	WriteString(", file = %O;\n");
	WriteString("}\n\n");
	WriteString("SEGMENTS {\n");
	WriteString("  ZEROPAGE: load = ZP,  type = zp;\n");
	WriteString("  BSS:      load = RAM, type = bss, define = yes;\n");
	WriteString("  HEADER1:  load = HDR, type = ro;\n");
	WriteString("  HEADER2:  load = HDR, type = ro,  start = $0E, fillval = $0;\n");
	WriteString("  HEADER3:  load = HDR, type = ro,  start = $2E, fillval = $0;\n");
	WriteString("  HEADER4:  load = HDR, type = ro,  start = $4E, fillval = $0;\n");
	WriteString("  HEADER5:  load = HDR, type = ro,  start = $6E;\n");
	WriteString("  CODE:     load = PRG, type = ");
	WriteString(Header.SoundChip & SNDCHIP_FDS ? "rw" : "ro");
	WriteString(",  start = $8000;\n");
	WriteString("  DPCM:     load = PRG, type = ");
	WriteString(Header.SoundChip & SNDCHIP_FDS ? "rw" : "ro");
	WriteString(",  start = "); WriteWord(DPCMSegment);
	WriteString(";\n");
	WriteString("}\n");

	Print(_T(" * NSF config size: %i bytes\n"), length);
}

void CCompiler::WriteBinary(CFile *pFile)
{
	// Dump all chunks as binary
	CChunkRenderBinary Render(pFile);
	Render.StoreChunks(m_vChunks);
	Print(_T(" * Music data size: %i bytes\n"), m_iMusicDataSize);
}

void CCompiler::WriteSamplesBinary(CFile *pFile)
{
	// Dump all samples as binary
	CChunkRenderBinary Render(pFile);
	Render.StoreSamples(m_vSamples);
	Print(_T(" * DPCM samples size: %i bytes\n"), m_iSamplesSize);
}

// Object list functions

CChunk *CCompiler::CreateChunk(chunk_type_t Type, CStringA label)
{
	CChunk *pChunk = new CChunk(Type, label);
	m_vChunks.push_back(pChunk);
	return pChunk;
}

int CCompiler::CountData() const
{
	// Only count data
	int Offset = 0;

	for (const auto pChunk : m_vChunks)
		Offset += pChunk->CountDataSize();

	return Offset;
}

CChunk *CCompiler::GetObjectByRef(CStringA label) const
{
	for (const auto pChunk : m_vChunks)
		if (!label.Compare(pChunk->GetLabel()))
			return pChunk;
	return nullptr;
}

#if 0

void CCompiler::WriteChannelMap()
{
	CChunk *pChunk = CreateChunk(CHUNK_CHANNEL_MAP, "");
	
	pChunk->StoreByte(CHANID_SQUARE1 + 1);
	pChunk->StoreByte(CHANID_SQUARE2 + 1);
	pChunk->StoreByte(CHANID_TRIANGLE + 1);
	pChunk->StoreByte(CHANID_NOISE + 1);

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC6)) {
		pChunk->StoreByte(CHANID_VRC6_PULSE1 + 1);
		pChunk->StoreByte(CHANID_VRC6_PULSE2 + 1);
		pChunk->StoreByte(CHANID_VRC6_SAWTOOTH + 1);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC7)) {
		pChunk->StoreByte(CHANID_VRC7_CH1 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH2 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH3 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH4 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH5 + 1);
		pChunk->StoreByte(CHANID_VRC7_CH6 + 1);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS)) {
		pChunk->StoreByte(CHANID_FDS + 1);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_MMC5)) {
		pChunk->StoreByte(CHANID_MMC5_SQUARE1 + 1);
		pChunk->StoreByte(CHANID_MMC5_SQUARE2 + 1);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_N163)) {
		for (unsigned int i = 0; i < m_pDocument->GetNamcoChannels(); ++i) {
			pChunk->StoreByte(CHANID_N163_CH1 + i + 1);
		}
	}

	pChunk->StoreByte(CHANID_DPCM + 1);
}

void CCompiler::WriteChannelTypes()
{
	const int TYPE_2A03 = 0;
	const int TYPE_VRC6 = 2;
	const int TYPE_VRC7 = 4;
	const int TYPE_FDS	= 6;
	const int TYPE_MMC5 = 8;
	const int TYPE_N163 = 10;
	const int TYPE_S5B	= 12;

	CChunk *pChunk = CreateChunk(CHUNK_CHANNEL_TYPES, "");
	
	for (int i = 0; i < 4; ++i)
		pChunk->StoreByte(TYPE_2A03);

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC6)) {
		for (int i = 0; i < 3; ++i)
			pChunk->StoreByte(TYPE_VRC6);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_VRC7)) {
		for (int i = 0; i < 3; ++i)
			pChunk->StoreByte(TYPE_VRC7);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_FDS)) {
		pChunk->StoreByte(TYPE_FDS);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_MMC5)) {
		for (int i = 0; i < 2; ++i)
			pChunk->StoreByte(TYPE_MMC5);
	}

	if (m_pDocument->ExpansionEnabled(SNDCHIP_N163)) {
		for (unsigned int i = 0; i < m_pDocument->GetNamcoChannels(); ++i)
			pChunk->StoreByte(TYPE_N163);
	}

	pChunk->StoreByte(TYPE_2A03);
}

#endif
