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

#include <map>
#include <vector>
#include "stdafx.h"
#include "DSample.h"		// // //
#include "FamiTrackerDoc.h"		// // // output title
#include "Compiler.h"
#include "Chunk.h"
#include "ChunkRenderText.h"

// Assembly labels
// // // moved from CCompiler
const char CChunkRenderText::LABEL_SONG_LIST[]			= "ft_song_list";
const char CChunkRenderText::LABEL_INSTRUMENT_LIST[]	= "ft_instrument_list";
const char CChunkRenderText::LABEL_SAMPLES_LIST[]		= "ft_sample_list";
const char CChunkRenderText::LABEL_SAMPLES[]			= "ft_samples";
const char CChunkRenderText::LABEL_GROOVE_LIST[]		= "ft_groove_list";			// // //
const char CChunkRenderText::LABEL_GROOVE[]				= "ft_groove_%i";			// // // one argument
const char CChunkRenderText::LABEL_WAVETABLE[]			= "ft_wave_table";
const char CChunkRenderText::LABEL_SAMPLE[]				= "ft_sample_%i";			// one argument
const char CChunkRenderText::LABEL_WAVES[]				= "ft_waves_%i";			// one argument
const char CChunkRenderText::LABEL_SEQ_2A03[]			= "ft_seq_2a03_%i";			// one argument
const char CChunkRenderText::LABEL_SEQ_VRC6[]			= "ft_seq_vrc6_%i";			// one argument
const char CChunkRenderText::LABEL_SEQ_FDS[]			= "ft_seq_fds_%i";			// one argument
const char CChunkRenderText::LABEL_SEQ_N163[]			= "ft_seq_n163_%i";			// one argument
const char CChunkRenderText::LABEL_SEQ_S5B[]			= "ft_seq_s5b_%i";			// // // one argument
const char CChunkRenderText::LABEL_INSTRUMENT[]			= "ft_inst_%i";				// one argument
const char CChunkRenderText::LABEL_SONG[]				= "ft_song_%i";				// one argument
const char CChunkRenderText::LABEL_SONG_FRAMES[]		= "ft_s%i_frames";			// one argument
const char CChunkRenderText::LABEL_SONG_FRAME[]			= "ft_s%if%i";				// two arguments
const char CChunkRenderText::LABEL_PATTERN[]			= "ft_s%ip%ic%i";			// three arguments

/**
 * Text chunk render, these methods will always output single byte strings
 *
 */

static const int DEFAULT_LINE_BREAK = 20;

// String render functions
const stChunkRenderFunc CChunkRenderText::RENDER_FUNCTIONS[] = {
	{CHUNK_HEADER,			&CChunkRenderText::StoreHeaderChunk},
	{CHUNK_SEQUENCE,		&CChunkRenderText::StoreSequenceChunk},
	{CHUNK_INSTRUMENT_LIST,	&CChunkRenderText::StoreInstrumentListChunk},
	{CHUNK_INSTRUMENT,		&CChunkRenderText::StoreInstrumentChunk},
	{CHUNK_SAMPLE_LIST,		&CChunkRenderText::StoreSampleListChunk},
	{CHUNK_SAMPLE_POINTERS,	&CChunkRenderText::StoreSamplePointersChunk},
	{CHUNK_GROOVE_LIST,		&CChunkRenderText::StoreGrooveListChunk},		// // //
	{CHUNK_GROOVE,			&CChunkRenderText::StoreGrooveChunk},		// // //
	{CHUNK_SONG_LIST,		&CChunkRenderText::StoreSongListChunk},
	{CHUNK_SONG,			&CChunkRenderText::StoreSongChunk},
	{CHUNK_FRAME_LIST,		&CChunkRenderText::StoreFrameListChunk},
	{CHUNK_FRAME,			&CChunkRenderText::StoreFrameChunk},
	{CHUNK_PATTERN,			&CChunkRenderText::StorePatternChunk},
	{CHUNK_WAVETABLE,		&CChunkRenderText::StoreWavetableChunk},
	{CHUNK_WAVES,			&CChunkRenderText::StoreWavesChunk}
};

CChunkRenderText::CChunkRenderText(CFile *pFile) : m_pFile(pFile),
	m_pFileNSFStub(nullptr),
	m_pFileNSFHeader(nullptr),
	m_pFileNSFConfig(nullptr),
	m_pFilePeriods(nullptr),
	m_pFileVibrato(nullptr),
	m_pFileMultiChipEnable(nullptr),
	m_pFileMultiChipUpdate(nullptr),
	m_bBankSwitched(false),
	m_iDataWritten(0)
{
}

void CChunkRenderText::StoreChunks(const std::vector<CChunk*> &Chunks)
{
	// Generate strings
	for (const auto pChunk : Chunks)
		for (int j = 0; j < sizeof(RENDER_FUNCTIONS) / sizeof(stChunkRenderFunc); ++j)
			if (pChunk->GetType() == RENDER_FUNCTIONS[j].type)
				CALL_MEMBER_FN(this, RENDER_FUNCTIONS[j].function)(pChunk, m_pFile);

	// Module header
	if (m_bBankSwitched)
		WriteFileString(CStringA("\t.segment \"MUS_FIXED\"\n"), m_pFile);

	DumpStrings(CStringA("; Module header\n"), CStringA("\n"), m_headerStrings, m_pFile);

	// Instrument list
	DumpStrings(CStringA("; Instrument pointer list\n"), CStringA("\n"), m_instrumentListStrings, m_pFile);
	DumpStrings(CStringA("; Instruments\n"), CStringA(""), m_instrumentStrings, m_pFile);

	// Sequences
	DumpStrings(CStringA("; Sequences\n"), CStringA("\n"), m_sequenceStrings, m_pFile);

	// Waves (FDS & N163)
	if (m_wavetableStrings.IsEmpty() == FALSE) {
		DumpStrings(CStringA("; FDS waves\n"), CStringA("\n"), m_wavetableStrings, m_pFile);
	}

	if (m_wavesStrings.IsEmpty() == FALSE) {
		DumpStrings(CStringA("; N163 waves\n"), CStringA("\n"), m_wavesStrings, m_pFile);
	}

	// Samples
	DumpStrings(CStringA("; DPCM instrument list (pitch, sample index)\n"), CStringA("\n"), m_sampleListStrings, m_pFile);
	DumpStrings(CStringA("; DPCM samples list (location, size, bank)\n"), CStringA("\n"), m_samplePointersStrings, m_pFile);

	// // // Grooves
	DumpStrings(CStringA("; Groove list\n"), CStringA(""), m_grooveListStrings, m_pFile);
	DumpStrings(CStringA("; Grooves (size, terms)\n"), CStringA("\n"), m_grooveStrings, m_pFile);

	// Songs
	DumpStrings(CStringA("; Song pointer list\n"), CStringA("\n"), m_songListStrings, m_pFile);
	DumpStrings(CStringA("; Song info\n"), CStringA("\n"), m_songStrings, m_pFile);

	// Song data
	DumpStrings(CStringA(";\n; Pattern and frame data for all songs below\n;\n\n"), CStringA(""), m_songDataStrings, m_pFile);

	// Actual DPCM samples are stored later
}

void CChunkRenderText::StoreSamples(const std::vector<const CDSample*> &Samples, CChunk *pChunk)
{
	ASSERT(pChunk != nullptr);

	// Store DPCM samples in file, assembly format
	CStringA str;
	str.Format("\n; DPCM samples (located at DPCM segment)\n");

	if (Samples.size() > 0 && !m_bBankSwitched) {
		// align first sample for external programs using assembly export
		// this allows more flexible memory configurations to directly use the export
		str.Append("\n\t.align 64\n");
		str.Append("\n\t.segment \"DPCM\"\n");
	}


	unsigned int Address = CCompiler::PAGE_SAMPLES;
	for (size_t i = 0; i < Samples.size(); ++i) if (const CDSample *pDSample = Samples[i]) {		// // //
		const unsigned int SampleSize = pDSample->GetSize();
		const char *pData = pDSample->GetData();

		CStringA label;
		label.Format(LABEL_SAMPLE, i);		// // //
		// take advantage of the fact that the list is stored in the same order as the samples vector
		unsigned char bank = (unsigned char)pChunk->GetData((int(i)*3) + 2);

		if (m_bBankSwitched)
			StoreDPCMBankSegment(bank, str);

		// adjust padding if necessary
		if ((Address & 0x3F) > 0) {
			str.Append("\n\t.align 64\n");
			int PadSize = 0x40 - (Address & 0x3F);
			Address += PadSize;
		}
		str.AppendFormat("; %s\n%s:\n", pDSample->GetName(), LPCSTR(label));
		StoreByteString(pData, SampleSize, str, DEFAULT_LINE_BREAK);
		Address += SampleSize;

		str.Append("\n");
	}

	WriteFileString(str, m_pFile);
}

void CChunkRenderText::DumpStrings(const CStringA &preStr, const CStringA &postStr, CStringArray &stringArray, CFile *pFile) const
{
	WriteFileString(preStr, pFile);

	for (int i = 0; i < stringArray.GetCount(); ++i) {
		WriteFileString(stringArray[i], pFile);
	}

	WriteFileString(postStr, pFile);
}

void CChunkRenderText::StoreHeaderChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();
	int i = 0;

	// don't write anything if data doesn't exist
	if (len != 0) {
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));		// // // Groove
		str.AppendFormat("\t.byte %i ; flags\n", pChunk->GetData(i++));
		if (pChunk->IsDataReference(i))
			str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));	// FDS waves
		str.AppendFormat("\t.word %i ; NTSC speed\n", pChunk->GetData(i++));
		str.AppendFormat("\t.word %i ; PAL speed\n", pChunk->GetData(i++));
		if (i < len)
			str.AppendFormat("\t.word %i ; N163 channels\n", pChunk->GetData(i++));	// N163 channels
	}

	m_headerStrings.Add(str);
}

void CChunkRenderText::StoreInstrumentListChunk(CChunk *pChunk, CFile *pFile)
{
	CString str;

	// Store instrument pointers
	str.Format(_T("%s:\n"), pChunk->GetLabel());

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		str.AppendFormat(_T("\t.word %s\n"), pChunk->GetDataRefName(i));
	}

	m_instrumentListStrings.Add(str);
}

void CChunkRenderText::StoreInstrumentChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();

	// don't write anything if data doesn't exist
	if (len != 0) {
		str.Format("%s:\n\t.byte %i\n", pChunk->GetLabel(), pChunk->GetData(0));

		for (int i = 1; i < len; ++i) {
			if (pChunk->IsDataReference(i)) {
				str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i));
			}
			else {
				if (pChunk->GetDataSize(i) == 1) {
					str.AppendFormat("\t.byte $%02X\n", pChunk->GetData(i));
				}
				else {
					str.AppendFormat("\t.word $%04X\n", pChunk->GetData(i));
				}
			}
		}
	}

	str.Append("\n");

	m_instrumentStrings.Add(str);
}

void CChunkRenderText::StoreSequenceChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	str.Format("%s:\n", pChunk->GetLabel());
	StoreByteString(pChunk, str, DEFAULT_LINE_BREAK);

	m_sequenceStrings.Add(str);
}

void CChunkRenderText::StoreSampleListChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	// Store sample list
	str.Format("%s:\n", pChunk->GetLabel());

	for (int i = 0; i < pChunk->GetLength(); i += 3) {
		str.AppendFormat("\t.byte %i, %i, %i\n", pChunk->GetData(i + 0), pChunk->GetData(i + 1), pChunk->GetData(i + 2));
	}

	m_sampleListStrings.Add(str);
}

void CChunkRenderText::StoreSamplePointersChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();

	// Store sample pointer
	str.Format("%s:\n", pChunk->GetLabel());

	// don't write anything if data doesn't exist
	if (len != 0) {
		int samplenum = 0;
		for (int i = 0; i < len; i += 3) {
			CStringA label;
			label.Format(LABEL_SAMPLE, samplenum);
			str.Append("\t.byte ");
			// location
			//str.AppendFormat("%i, ", pChunk->GetData(i + 0));
			str.AppendFormat("<((%s - $C000) >> 6), ", label);

			// size
			str.AppendFormat("%i, ", pChunk->GetData(i + 1));

			//str.AppendFormat("%i", pChunk->GetData(i + 2));
			str.AppendFormat("<.bank(%s)\n", label);
			samplenum++;
		}
	}

	m_samplePointersStrings.Add(str);
}

void CChunkRenderText::StoreGrooveListChunk(CChunk *pChunk, CFile *pFile)		// // //
{
	CStringA str;
	
	str.Format("%s:\n", pChunk->GetLabel());
	
	for (int i = 0; i < pChunk->GetLength(); ++i) {
		str.AppendFormat("\t.byte $%02X\n", pChunk->GetData(i));
	}

	m_grooveListStrings.Add(str);
}

void CChunkRenderText::StoreGrooveChunk(CChunk *pChunk, CFile *pFile)		// // //
{
	CStringA str;
	
	// str.Format("%s:\n", pChunk->GetLabel());
	StoreByteString(pChunk, str, DEFAULT_LINE_BREAK);

	m_grooveStrings.Add(str);
}

void CChunkRenderText::StoreSongListChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	str.Format("%s:\n", pChunk->GetLabel());

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i));
	}

	m_songListStrings.Add(str);
}

void CChunkRenderText::StoreSongChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	str.Format("%s:\n", pChunk->GetLabel());

	for (int i = 0; i < pChunk->GetLength();) {
		LPCSTR datarefname = pChunk->GetDataRefName(i++);
		str.AppendFormat("\t.word %s\n", datarefname);
		str.AppendFormat("\t.byte %i\t; frame count\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; pattern length\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; speed\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; tempo\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; groove position\n", pChunk->GetData(i++));		// // //
		str.AppendFormat("\t.byte <.bank(%s)\t; initial bank\n", datarefname); i++;		// !! !!
	}

	str.Append("\n");

	m_songStrings.Add(str);
}

void CChunkRenderText::StoreFrameListChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	unsigned char bank = pChunk->GetBank();

	// Pointers to frames
	if (m_bBankSwitched)
		StoreMusicBankSegment(bank, str);

	str.AppendFormat("%s:\n", pChunk->GetLabel());

	for (int i = 0; i < pChunk->GetLength(); ++i) {
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i));
	}

	m_songDataStrings.Add(str);
}

void CChunkRenderText::StoreFrameChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();

	unsigned char bank = pChunk->GetBank();

	if (m_bBankSwitched)
		StoreMusicBankSegment(bank, str);

	// Frame list
	str.AppendFormat("%s:\n\t.word ", pChunk->GetLabel());

	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataReference(i))
			str.AppendFormat("%s%s", (j++ > 0) ? _T(", ") : _T(""), pChunk->GetDataRefName(i));
	}

	str.AppendFormat("\n\t.byte ", pChunk->GetLabel());
	// Bank values
	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataReference(i)) {
			str.AppendFormat("%s<.bank(%s)", (j++ > 0) ? _T(", ") : _T(""), pChunk->GetDataRefName(i));
		}
	}

	str.Append("\n");

	m_songDataStrings.Add(str);
}

void CChunkRenderText::StorePatternChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	std::size_t len = pChunk->GetLength();

	unsigned char bank = pChunk->GetBank();

	// Patterns
	if (m_bBankSwitched)
		StoreMusicBankSegment(bank, str);
	str.AppendFormat("%s:\n", pChunk->GetLabel());

	const std::vector<char> &vec = pChunk->GetStringData(0);
	len = vec.size();

	StoreByteString(&vec.front(), static_cast<int>(vec.size()), str, DEFAULT_LINE_BREAK);
/*
	for (int i = 0; i < len; ++i) {
		str.AppendFormat("$%02X", (unsigned char)vec[i]);
		if ((i % 20 == 19) && (i < len - 1))
			str.Append("\n\t.byte ");
		else {
			if (i < len - 1)
				str.Append(", ");
		}
	}
*/
	str.Append("\n");

	m_songDataStrings.Add(str);
}

void CChunkRenderText::StoreWavetableChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();

	// FDS waves
	str.Format("%s:\n", pChunk->GetLabel());

	// don't write anything if data doesn't exist
	if (len != 0) {
		str.Append("\t.byte ");

		for (int i = 0; i < len; ++i) {
			str.AppendFormat("$%02X", pChunk->GetData(i));
			if ((i % 64 == 63) && (i < len - 1))
				str.Append("\n\t.byte ");
			else {
				if (i < len - 1)
					str.Append(", ");
			}
		}
	}

	str.Append("\n");

	m_wavetableStrings.Add(str);
}

void CChunkRenderText::StoreWavesChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();

	int waves = pChunk->GetData(0);
	int wave_len = (len - 1) / waves;

	// Namco waves
	str.Format("%s:\n", pChunk->GetLabel());

	// don't write anything if data doesn't exist
	if (len != 0) {
		str.AppendFormat("\t.byte %i\n", waves);

		str.Append("\t.byte ");

		for (int i = 0; i < (len-1); ++i) {
			str.AppendFormat("$%02X", pChunk->GetData(i+1));
			if (i % wave_len == (wave_len - 1) && i < len - 2)
				str.Append("\n\t.byte ");
			else {
				if (i < len - 2)
					str.Append(", ");
			}
		}
	}

	str.Append("\n");

	m_wavesStrings.Add(str);
}

void CChunkRenderText::StoreMusicBankSegment(unsigned char bank, CStringA &str)
{
	if (bank < CCompiler::PATTERN_SWITCH_BANK)
		return;
	CStringA segmenttxt, memorytxt;
	bool duplicate = false;
	str.Format("\t.segment \"MUS_%02X\"\n", bank);
	memorytxt.Format("  PRG_MUSIC_%02X:     start = $B000, size = $1000, type = ro, file = %%O, fill = yes, fillval = $00, bank = $%02X;\n", bank, bank);
	segmenttxt.Format("  MUS_%02X:    load = PRG_MUSIC_%02X,     type = ro;\n", bank, bank);

	for (const CStringA string : m_configMemoryAreaStrings)
		if (string == memorytxt) duplicate |= true;

	for (const CStringA string : m_configSegmentStrings)
		if (string == segmenttxt) duplicate |= true;

	if (!duplicate) {
		m_configMemoryAreaStrings.push_back(memorytxt);
		m_configSegmentStrings.push_back(segmenttxt);
	}
}

void CChunkRenderText::StoreDPCMBankSegment(unsigned char bank, CStringA &str)
{
	CStringA segmenttxt, memorytxt;
	bool duplicate = false;
	str.AppendFormat("\n\t.segment \"DMC_%02X\"\n", bank);
	memorytxt.Format("  PRG_DPCM_%02X:      start = $C000, size = $3000, type = ro, file = %%O, fill = yes, fillval = $00, bank = $%02X;\n", bank, bank);
	segmenttxt.Format("  DMC_%02X:    load = PRG_DPCM_%02X,      type = ro;\n", bank, bank);

	for (const auto &string : m_configMemoryAreaStrings)
		if (string == memorytxt) duplicate |= true;
	for (const auto &string : m_configSegmentStrings)
		if (string == segmenttxt) duplicate |= true;

	if (!duplicate) {
		m_configMemoryAreaStrings.push_back(memorytxt);
		m_configSegmentStrings.push_back(segmenttxt);
	}
}

void CChunkRenderText::WriteFileString(const CStringA &str, CFile *pFile) const
{
	if (!pFile) return;
	pFile->Write(const_cast<CStringA&>(str).GetBuffer(), str.GetLength());
}

// These functions write to a separate file. CFile path to those separate files must be the same as export.

void CChunkRenderText::StoreNSFStub(unsigned char Expansion, vibrato_t VibratoStyle, bool LinearPitch, int ActualNamcoChannels, bool UseAllChips, bool IsAssembly) const
{
	CString str;

	str.Append(";\n; NSF stub file, used to define compile constants\n;\n\n");
	str.Append("PACKAGE = 1\n");
	str.Append("HAS_NSF_HEADER = 1\n");
	str.Append("USE_AUX_DATA = 1\n");
	bool MultiChip = ((Expansion & (Expansion - 1)) != 0) && UseAllChips;
	if (MultiChip) {
		// Simulate NSF export multichip kernel
		str.Append("USE_ALL = 1\n");
		str.Append("NAMCO_CHANNELS = 8\n");
		str.Append("USE_OLDVIBRATO = 1\n");
		str.Append("USE_BANKSWITCH = 1\n");
		str.Append("USE_LINEARPITCH = 1\n");
	}
	else {
		if (Expansion & SNDCHIP_VRC6)
			str.Append("USE_VRC6 = 1\n");
		if (Expansion & SNDCHIP_VRC7)
			str.Append("USE_VRC7 = 1\n");
		if (Expansion & SNDCHIP_FDS)
			str.Append("USE_FDS = 1\n");
		if (Expansion & SNDCHIP_MMC5)
			str.Append("USE_MMC5 = 1\n");
		if (Expansion & SNDCHIP_N163)
			str.Append("USE_N163 = 1\n");
		if (Expansion & SNDCHIP_S5B)
			str.Append("USE_S5B = 1\n");
		str.AppendFormat("NAMCO_CHANNELS = %d\n", ActualNamcoChannels);
		if (m_bBankSwitched)
			str.Append("USE_BANKSWITCH = 1\n");
		if (VibratoStyle == VIBRATO_OLD)
			str.Append("USE_OLDVIBRATO = 1\n");
		if (LinearPitch)
			str.Append("USE_LINEARPITCH = 1\n");
	}

	str.Append("\n; path to NSF driver source\n");
	str.Append(".include \"driver/driver.s\"\n");

	std::string asmfile = m_pFile->GetFileName();
	str.Append("; path to NSF export source\n");
	str.AppendFormat(".include \"%s\"\n", asmfile.c_str());

	WriteFileString(str, m_pFileNSFStub);
}

void CChunkRenderText::StoreNSFHeader(stNSFHeader Header) const
{
	CString str;

	str.Append(";\n; NSF Header\n;\n");
	str.Append(".import __FTR_FILEOFFS__\n");
	str.Append("NSF2_SIZE = __FTR_FILEOFFS__\n\n");

	str.Append("\n.segment \"HEADER1\"\n");
	str.AppendFormat(".byte $%02X, $%02X, $%02X, $%02X, $%02X\t; ID\n",
		Header.Ident[0], Header.Ident[1], Header.Ident[2], Header.Ident[3], Header.Ident[4]);
	str.Append(".byte $01\t\t\t\t\t\t; Version\n");
	str.AppendFormat(".byte $%02X\t\t\t\t\t\t\t; Number of songs\n", Header.TotalSongs);
	str.AppendFormat(".byte $%02X\t\t\t\t\t\t\t; Start song\n", Header.StartSong);
	str.Append(".word LOAD\t\t\t\t\t\t; LOAD address\n");
	str.Append(".word INIT\t\t\t\t\t\t; INIT address\n");
	str.Append(".word PLAY\t\t\t\t\t\t; PLAY address\n\n");

	str.Append(".segment \"HEADER2\"\n");
	str.AppendFormat(".byte \"%s\"\t; Name, 32 bytes\n\n", Header.SongName);

	str.Append(".segment \"HEADER3\"\n");
	str.AppendFormat(".byte \"%s\"\t; Artist, 32 bytes\n\n", Header.ArtistName);

	str.Append(".segment \"HEADER4\"\n");
	str.AppendFormat(".byte \"%s\"\t; Copyright, 32 bytes\n\n", Header.Copyright);

	str.Append(".segment \"HEADER5\"\n");
	str.AppendFormat(".word $%04X\t\t\t\t\t\t; NTSC speed\n", Header.Speed_NTSC);
	str.AppendFormat(".byte $%02X, $%02X, $%02X, $%02X, $%02X, $%02X, $%02X, $%02X\t; Bank values\n",
		Header.BankValues[0], Header.BankValues[1], Header.BankValues[2], Header.BankValues[3],
		Header.BankValues[4], Header.BankValues[5], Header.BankValues[6], Header.BankValues[7]);
	str.AppendFormat(".word $%04X\t\t\t\t\t\t; PAL speed\n", Header.Speed_PAL);
	str.AppendFormat(".byte $%02X\t\t\t\t\t\t; Region flags\n", Header.Flags);
	str.Append(".byte EXPANSION_FLAG\t\t\t; Expansion audio flags\n");
	str.AppendFormat(".byte $%02X\t\t\t\t\t\t; NSF2 flags\n", Header.NSF2Flags);
	str.Append(".faraddr NSF2_SIZE\t\t\t\t; NSF data length\n");

	WriteFileString(str, m_pFileNSFHeader);
}

void CChunkRenderText::StoreNSFConfig(unsigned int DPCMSegment, stNSFHeader Header) const
{
	CString str;
	CString segmentType = (Header.SoundChip & SNDCHIP_FDS ? "rw" : "ro");

	str.Append("MEMORY {\n");
	str.Append("  ZP:               start = $00,   size = $100,  type = rw, file = \"\";\n");
	str.Append("  RAM:              start = $200,  size = $600,  type = rw, file = \"\";\n");
	str.Append("  HDR:              start = $00,   size = $80,   type = ro, file = \"%O_header\";\n");

	if (m_bBankSwitched) {
		str.Append("  PRG_DRIVER_MUSIC: start = $8000, size = $3000, type = " + segmentType + ", file = %O, fill = yes, fillval = $00, bank = $00;\n");
		for (const auto &string : m_configMemoryAreaStrings)
			str.Append(string);
	}
	else
		str.Append("  PRG:              start = $8000, size = $8000, type = " + segmentType + ", file = %O, bank = $00;\n");
	str.Append("  FTR:              start = $0000, size = $4000, type = ro, file = %O, define = yes;\n");
	str.Append("}\n\n");
	str.Append("SEGMENTS {\n");
	str.Append("  ZEROPAGE:  load = ZP,               type = zp;\n");
	str.Append("  BSS:       load = RAM,              type = bss, define = yes;\n");
	str.Append("  HEADER1:   load = HDR,              type = ro;\n");
	str.Append("  HEADER2:   load = HDR,              type = ro, start = $0E, fillval = $0;\n");
	str.Append("  HEADER3:   load = HDR,              type = ro, start = $2E, fillval = $0;\n");
	str.Append("  HEADER4:   load = HDR,              type = ro, start = $4E, fillval = $0;\n");
	str.Append("  HEADER5:   load = HDR,              type = ro, start = $6E;\n");

	if (m_bBankSwitched) {
		str.Append("  CODE:      load = PRG_DRIVER_MUSIC, type = " + segmentType + ";\n");
		str.Append("  MUS_FIXED: load = PRG_DRIVER_MUSIC, type = " + segmentType + ";\n");
		for (const auto &string : m_configSegmentStrings)
			str.Append(string);
	}
	else {
		str.Append("  CODE:      load = PRG, type = " + segmentType + ";\n");
		str.Append("  DPCM:      load = PRG, type = " + segmentType);
		// this is tricky. the driver size is unpredictable here.
		// so instead, we only start at address DPCMSegment
		// *only if* the nonbankswitching DPCM segment address remains at CCompiler::PAGE_SAMPLES
		if (DPCMSegment == CCompiler::PAGE_SAMPLES)
			str.AppendFormat(",  start = $%04X;\n", DPCMSegment);
		else
			str.Append(";\n");
	}

	str.Append("}\nFILES {\n  # hack to get .dbg symbols to align with code\n  %O: format = bin;\n  \"%O_header\": format = bin;\n}");

	WriteFileString(str, m_pFileNSFConfig);
}

void CChunkRenderText::StorePeriods(unsigned int *pLUTNTSC, unsigned int* pLUTPAL, unsigned int* pLUTSaw, unsigned int *pLUTVRC7, unsigned int *pLUTFDS, unsigned int *pLUTN163) const
{
	CString str;

	const auto StoreWordString = [&](unsigned int* Table) {
		str.Append("\t.word\t");
		for (int i = 0; i < NOTE_COUNT; ++i) {
			unsigned int Val = Table[i] & 0xFFFF;
			if (i < NOTE_COUNT - 1) {
				if (i % NOTE_RANGE < NOTE_RANGE - 1)
					str.AppendFormat("$%04X, ", Val);
				else
					str.AppendFormat("$%04X\n\t.word\t", Val);
			}
			else
				str.AppendFormat("$%04X\n", Val);
		}
	};

	// One possible optimization is to store the PAL table as the difference from the NTSC table

	str.Append("; 2A03 NTSC\n");
	str.Append(".if .defined(NTSC_PERIOD_TABLE)\n");
	str.Append("ft_periods_ntsc: ;; Patched\n");
	StoreWordString(pLUTNTSC);

	str.Append(".endif\n\n");
	str.Append("; 2A03 PAL\n");
	str.Append(".if .defined(PAL_PERIOD_TABLE)\n");
	str.Append("ft_periods_pal: ;; Patched\n");
	StoreWordString(pLUTPAL);

	str.Append(".endif\n\n");
	str.Append("; VRC6 Sawtooth\n");
	str.Append(".if .defined(USE_VRC6)\n");
	str.Append("ft_periods_sawtooth: ;; Patched\n");
	StoreWordString(pLUTSaw);

	str.Append(".endif\n\n");
	str.Append("; FDS\n");
	str.Append(".if .defined(USE_FDS)\n");
	str.Append("ft_periods_fds: ;; Patched\n");
	StoreWordString(pLUTFDS);

	str.Append(".endif\n\n");
	str.Append("; N163\n");
	str.Append(".if .defined(USE_N163)\n");
	str.Append("ft_periods_n163: ;; Patched\n");
	StoreWordString(pLUTN163);

	str.Append(".endif\n\n");
	str.Append("; VRC7\n");
	str.Append(".if .defined(USE_VRC7)\n");
	str.Append("; Fnum table, multiplied by 4 for higher resolution\n");
	str.Append(".define ft_vrc7_table ");

	for (int i = 0; i <= NOTE_RANGE; ++i) {		// // // include last item for linear pitch code optimization
		if (i == NOTE_RANGE)
			str.AppendFormat("$%04X\n", pLUTVRC7[0] << 3);
		else
			str.AppendFormat("$%04X, ", pLUTVRC7[i] << 2);
	}

	str.Append("\n");
	str.Append("ft_note_table_vrc7_l: ;; Patched\n");
	str.Append("\t.lobytes ft_vrc7_table\n");
	str.Append("ft_note_table_vrc7_h:\n");
	str.Append("\t.hibytes ft_vrc7_table\n");
	str.Append(".endif\n");

	WriteFileString(str, m_pFilePeriods);
}

void CChunkRenderText::StoreVibrato(unsigned int *pLUTVibrato) const
{
	CString str;

	str.Append("; Vibrato table (256 bytes)\n"
		"ft_vibrato_table: ;; Patched\n");
	for (int i = 0; i < 16; i++) {	// depth
		str.Append("\t.byte ");
		for (int j = 0; j < 16; j++) {	// phase
			str.AppendFormat("$%02X%s", pLUTVibrato[i * 16 + j], j < 15 ? ", " : "");
		}
		str.Append("\n");
	}

	WriteFileString(str, m_pFileVibrato);
}

void CChunkRenderText::StoreUpdateExt(unsigned char Expansion) const
{
	// // // special processing for multichip
	if ((Expansion & (Expansion - 1)) == false) return;
	
	CString str;

	str.Append("\t; VRC6\n");
	if (Expansion & SNDCHIP_VRC6)
		str.Append("\tjsr ft_update_vrc6");
	else
		str.Append("\tnop\n\tnop\n\tnop\n");
	str.Append("\t; VRC7\n");
	if (Expansion & SNDCHIP_VRC7)
		str.Append("\tjsr ft_update_vrc7");
	else
		str.Append("\tnop\n\tnop\n\tnop\n");
	str.Append("\t; FDS\n");
	if (Expansion & SNDCHIP_FDS)
		str.Append("\tjsr ft_update_fds");
	else
		str.Append("\tnop\n\tnop\n\tnop\n");
	str.Append("\t; MMC5\n");
	if (Expansion & SNDCHIP_MMC5)
		str.Append("\tjsr ft_update_mmc5");
	else
		str.Append("\tnop\n\tnop\n\tnop\n");
	str.Append("\t; N163\n");
	if (Expansion & SNDCHIP_N163)
		str.Append("\tjsr ft_update_n163");
	else
		str.Append("\tnop\n\tnop\n\tnop\n");
	str.Append("\t; S5B\n");
	if (Expansion & SNDCHIP_S5B)
		str.Append("\tjsr ft_update_s5b");
	else
		str.Append("\tnop\n\tnop\n\tnop\n");
	WriteFileString(str, m_pFileMultiChipUpdate);
}

void CChunkRenderText::StoreEnableExt(std::vector<char> &ChannelOrder) const
{
	// // // special processing for multichip
	CStringA str;

	str.Append("ft_channel_enable: ;; Patched\n");
	StoreByteString(ChannelOrder.data(), (int)ChannelOrder.size(), str, DEFAULT_LINE_BREAK);
	WriteFileString(str, m_pFileMultiChipEnable);
}

void CChunkRenderText::SetExtraDataFiles(CFile *pFileNSFStub, CFile* pFileNSFHeader, CFile* pFileNSFConfig, CFile* pFilePeriods, CFile* pFileVibrato, CFile *pFileMultiChipEnable, CFile *pFileMultiChipUpdate)
{
	m_pFileNSFStub = pFileNSFStub;
	m_pFileNSFHeader = pFileNSFHeader;
	m_pFileNSFConfig = pFileNSFConfig;
	m_pFilePeriods = pFilePeriods;
	m_pFileVibrato = pFileVibrato;
	m_pFileMultiChipEnable = pFileMultiChipEnable;
	m_pFileMultiChipUpdate = pFileMultiChipUpdate;
}

void CChunkRenderText::SetBankSwitching(bool bBankSwitched)
{
	m_bBankSwitched = bBankSwitched;
}

void CChunkRenderText::StoreByteString(const char *pData, int Len, CStringA &str, int LineBreak) const
{	
	str.Append("\t.byte ");

	for (int i = 0; i < Len; ++i) {
		str.AppendFormat("$%02X", (unsigned char)pData[i]);

		if ((i % LineBreak == (LineBreak - 1)) && (i < Len - 1))
			str.Append("\n\t.byte ");
		else if (i < Len - 1)
			str.Append(", ");
	}

	str.Append("\n");
}

void CChunkRenderText::StoreByteString(const CChunk *pChunk, CStringA &str, int LineBreak) const
{
	int len = pChunk->GetLength();

	// don't write anything if data doesn't exist
	if (len != 0) {
		str.Append("\t.byte ");

		for (int i = 0; i < len; ++i) {
			str.AppendFormat("$%02X", pChunk->GetData(i));

			if ((i % LineBreak == (LineBreak - 1)) && (i < len - 1))
				str.Append("\n\t.byte ");
			else if (i < len - 1)
				str.Append(", ");
		}
	}

	str.Append("\n");
}
