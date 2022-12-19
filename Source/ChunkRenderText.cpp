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

CChunkRenderText::CChunkRenderText(CFile *pFile) : m_pFile(pFile)
{
}

void CChunkRenderText::StoreChunks(const std::vector<CChunk*> &Chunks)
{
	// Generate strings
	for (const auto pChunk : Chunks)
		for (int j = 0; j < sizeof(RENDER_FUNCTIONS) / sizeof(stChunkRenderFunc); ++j)
			if (pChunk->GetType() == RENDER_FUNCTIONS[j].type)
				CALL_MEMBER_FN(this, RENDER_FUNCTIONS[j].function)(pChunk, m_pFile);

	// Write strings to file
	WriteFileString(CStringA("; " APP_NAME " exported music data: "), m_pFile);
	WriteFileString(CFamiTrackerDoc::GetDoc()->GetTitle(), m_pFile);
	WriteFileString(CStringA("\n;\n\n"), m_pFile);

	// Module header
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

void CChunkRenderText::StoreSamples(const std::vector<const CDSample*> &Samples)
{
	// Store DPCM samples in file, assembly format
	CStringA str;
	str.Format("\n; DPCM samples (located at DPCM segment)\n");
	WriteFileString(str, m_pFile);

	if (Samples.size() > 0) {
		str.Format("\n\t.segment \"DPCM\"\n");
		WriteFileString(str, m_pFile);
	}

	unsigned int Address = CCompiler::PAGE_SAMPLES;
	for (size_t i = 0; i < Samples.size(); ++i) if (const CDSample *pDSample = Samples[i]) {		// // //
		const unsigned int SampleSize = pDSample->GetSize();
		const char *pData = pDSample->GetData();
		
		CStringA label;
		label.Format(LABEL_SAMPLE, i);		// // //
		str.Format("%s: ; %s\n", LPCSTR(label), pDSample->GetName());
		StoreByteString(pData, SampleSize, str, DEFAULT_LINE_BREAK);
		Address += SampleSize;

		// Adjust if necessary
		if ((Address & 0x3F) > 0) {
			int PadSize = 0x40 - (Address & 0x3F);
			Address	+= PadSize;
			str.Append("\n\t.align 64\n");
		}

		str.Append("\n");
		WriteFileString(str, m_pFile);
	}
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
	if (i < pChunk->GetLength())
		str.AppendFormat("\t.word %i ; N163 channels\n", pChunk->GetData(i++));	// N163 channels

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

	if (len > 0) {
		str.Append("\t.byte ");

		int len = pChunk->GetLength();
		for (int i = 0; i < len; ++i) {
			str.AppendFormat("%i%s", pChunk->GetData(i), (i < len - 1) && (i % 3 != 2) ? ", " : "");
			if (i % 3 == 2 && i < (len - 1))
				str.Append("\n\t.byte ");
		}
	}

	str.Append("\n");

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
		str.AppendFormat("\t.word %s\n", pChunk->GetDataRefName(i++));
		str.AppendFormat("\t.byte %i\t; frame count\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; pattern length\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; speed\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; tempo\n", pChunk->GetData(i++));
		str.AppendFormat("\t.byte %i\t; groove position\n", pChunk->GetData(i++));		// // //
		str.AppendFormat("\t.byte %i\t; initial bank\n", pChunk->GetData(i++));
	}

	str.Append("\n");

	m_songStrings.Add(str);
}

void CChunkRenderText::StoreFrameListChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;

	// Pointers to frames
	str.Format("; Bank %i\n", pChunk->GetBank());
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

	// Frame list
	str.Format("%s:\n\t.word ", pChunk->GetLabel());

	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataReference(i))
			str.AppendFormat("%s%s", (j++ > 0) ? _T(", ") : _T(""), pChunk->GetDataRefName(i));
	}

	// Bank values
	for (int i = 0, j = 0; i < len; ++i) {
		if (pChunk->IsDataBank(i)) {
			if (j == 0) {
				str.AppendFormat("\n\t.byte ", pChunk->GetLabel());
			}
			str.AppendFormat("%s$%02X", (j++ > 0) ? _T(", ") : _T(""), pChunk->GetData(i));
		}
	}

	str.Append("\n");

	m_songDataStrings.Add(str);
}

void CChunkRenderText::StorePatternChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	std::size_t len = pChunk->GetLength();

	// Patterns
	str.Format("; Bank %i\n", pChunk->GetBank());
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

	str.Append("\n");

	m_wavetableStrings.Add(str);
}

void CChunkRenderText::StoreWavesChunk(CChunk *pChunk, CFile *pFile)
{
	CStringA str;
	int len = pChunk->GetLength();

//				int waves = pChunk->GetData(0);
	int wave_len = 16;//(len - 1) / waves;

	// Namco waves
	str.Format("%s:\n", pChunk->GetLabel());
//				str.AppendFormat("\t.byte %i\n", waves);
	
	str.Append("\t.byte ");

	for (int i = 0; i < len; ++i) {
		str.AppendFormat("$%02X", pChunk->GetData(i));
		if ((i % wave_len == (wave_len - 1)) && (i < len - 1))
			str.Append("\n\t.byte ");
		else {
			if (i < len - 1)
				str.Append(", ");
		}
	}

	str.Append("\n");

	m_wavesStrings.Add(str);
}

void CChunkRenderText::WriteFileString(const CStringA &str, CFile *pFile) const
{
	pFile->Write(const_cast<CStringA&>(str).GetBuffer(), str.GetLength());
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
	
	str.Append("\t.byte ");

	for (int i = 0; i < len; ++i) {
		str.AppendFormat("$%02X", pChunk->GetData(i));

		if ((i % LineBreak == (LineBreak - 1)) && (i < len - 1))
			str.Append("\n\t.byte ");
		else if (i < len - 1)
			str.Append(", ");
	}

	str.Append("\n");
}
