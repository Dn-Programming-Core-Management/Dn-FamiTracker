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
#include "FamiTrackerTypes.h"		// // //
#include "PatternData.h"		// // //
#include "TextExporter.h"
#include "FamiTrackerDoc.h"

#define DEBUG_OUT(...) { CString s__; s__.Format(__VA_ARGS__); OutputDebugString(s__); }

// command tokens
enum
{
	CT_COMMENTLINE,    // anything may follow
	// song info
	CT_TITLE,          // string
	CT_AUTHOR,         // string
	CT_COPYRIGHT,      // string
	CT_COMMENT,        // string (concatenates line)
	// global settings
	CT_MACHINE,        // uint (0=NTSC, 1=PAL)
	CT_FRAMERATE,      // uint (0=default)
	CT_EXPANSION,      // uint (0=none, 1=VRC6, 2=VRC7, 4=FDS, 8=MMC5, 16=N163, 32=S5B)
	CT_VIBRATO,        // uint (0=old, 1=new)
	CT_SPLIT,          // uint (32=default)
	// namco global settings
	CT_N163CHANNELS,   // uint
	// macros
	CT_MACRO,          // uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	CT_MACROVRC6,      // uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	CT_MACRON163,      // uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	CT_MACROS5B,       // uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	// dpcm samples
	CT_DPCMDEF,        // uint (index) uint (size) string (name)
	CT_DPCM,           // : hex_list
	// // // detune settings
	CT_DETUNE,         // uint (chip) uint (oct) uint (note) int (offset)
	// // // grooves
	CT_GROOVE,         // uint (index) uint (size) : int_list
	CT_USEGROOVE,      // : int_list
	// instruments
	CT_INST2A03,       // uint (index) int int int int int string (name)
	CT_INSTVRC6,       // uint (index) int int int int int string (name)
	CT_INSTVRC7,       // uint (index) int (patch) hex hex hex hex hex hex hex hex string (name)
	CT_INSTFDS,        // uint (index) int (mod enable) int (m speed) int (m depth) int (m delay) string (name)
	CT_INSTN163,       // uint (index) int int int int int uint (w size) uint (w pos) uint (w count) string (name)
	CT_INSTS5B,        // uint (index) int int int int int  string (name)
	// instrument data
	CT_KEYDPCM,        // uint (inst) uint (oct) uint (note) uint (sample) uint (pitch) uint (loop) uint (loop_point)
	CT_FDSWAVE,        // uint (inst) : uint_list x 64
	CT_FDSMOD,         // uint (inst) : uint_list x 32
	CT_FDSMACRO,       // uint (inst) uint (type) int (loop) int (release) int (setting) : int_list
	CT_N163WAVE,       // uint (inst) uint (wave) : uint_list
	// track info
	CT_TRACK,          // uint (pat length) uint (speed) uint (tempo) string (name)
	CT_COLUMNS,        // : uint_list (effect columns)
	CT_ORDER,          // hex (frame) : hex_list
	// pattern data
	CT_PATTERN,        // hex (pattern)
	CT_ROW,            // row data
	// end of command list
	CT_COUNT
};

static const TCHAR* CT[CT_COUNT] =
{
	// comment
	_T("#"),
	// song info
	_T("TITLE"),
	_T("AUTHOR"),
	_T("COPYRIGHT"),
	_T("COMMENT"),
	// global settings
	_T("MACHINE"),
	_T("FRAMERATE"),
	_T("EXPANSION"),
	_T("VIBRATO"),
	_T("SPLIT"),
	// namco global settings
	_T("N163CHANNELS"),
	// macros
	_T("MACRO"),
	_T("MACROVRC6"),
	_T("MACRON163"),
	_T("MACROS5B"),
	// dpcm
	_T("DPCMDEF"),
	_T("DPCM"),
	// // // detune settings
	_T("DETUNE"),
	// // // grooves
	_T("GROOVE"),
	_T("USEGROOVE"),
	// instruments
	_T("INST2A03"),
	_T("INSTVRC6"),
	_T("INSTVRC7"),
	_T("INSTFDS"),
	_T("INSTN163"),
	_T("INSTS5B"),
	// instrument data
	_T("KEYDPCM"),
	_T("FDSWAVE"),
	_T("FDSMOD"),
	_T("FDSMACRO"),
	_T("N163WAVE"),
	// track info
	_T("TRACK"),
	_T("COLUMNS"),
	_T("ORDER"),
	// pattern data
	_T("PATTERN"),
	_T("ROW"),
};

// =============================================================================

class Tokenizer
{
public:
	Tokenizer(const CString* text_)
		: text(text_), pos(0), line(1), linestart(0)
	{}

	~Tokenizer()
	{}

	void Reset()
	{
		pos = 0;
		line = 1;
		linestart = 0;
	}

	void ConsumeSpace()
	{
		if (!text) return;
		while (pos < text->GetLength())
		{
			TCHAR c = text->GetAt(pos);
			if (c != TCHAR(' ') &&
				c != TCHAR('\t'))
			{
				return;
			}
			++pos;
		}
	}

	void FinishLine()
	{
		if (!text) return;
		while (pos < text->GetLength() && text->GetAt(pos) != TCHAR('\n'))
		{
			++pos;
		}
		if (pos < text->GetLength()) ++pos; // skip newline
		++line;
		linestart = pos;
	}

	int GetColumn() const
	{
		return 1 + pos - linestart;
	}

	bool Finished() const
	{
		if (!text) return true;
		return pos >= text->GetLength();
	}

	CString ReadToken()
	{
		if (!text) return _T("");

		ConsumeSpace();
		CString t = _T("");

		bool inQuote = false;
		bool lastQuote = false; // for finding double-quotes
		do
		{
			if (pos >= text->GetLength()) break;
			TCHAR c = text->GetAt(pos);
			if ((c == TCHAR(' ') && !inQuote) ||
				c == TCHAR('\t') ||
				c == TCHAR('\r') ||
				c == TCHAR('\n'))
			{
				break;
			}

			// quotes suppress space ending the token
			if (c == TCHAR('\"'))
			{
				if (!inQuote && t.GetLength() == 0) // first quote begins a quoted string
				{
					inQuote = true;
				}
				else
				{
					if (lastQuote) // convert "" to "
					{
						t += c;
						lastQuote = false;
					}
					else
					{
						lastQuote = true;
					}
				}
			}
			else
			{
				lastQuote = false;
				t += c;
			}

			++pos;
		}
		while (true);

		//DEBUG_OUT("ReadToken(%d,%d): '%s'\n", line, GetColumn(), t);
		return t;
	}

	bool ReadInt(int& i, int range_min, int range_max, CString* err)
	{
		CString t = ReadToken();
		int c = GetColumn();
		if (t.GetLength() < 1)
		{
			if (err) err->Format(_T("Line %d column %d: expected integer, no token found."), line, c);
			return false;
		}

		int result = ::sscanf(t, "%d", &i);
		if(result == EOF || result == 0)
		{
			if (err) err->Format(_T("Line %d column %d: expected integer, '%s' found."), line, c, t);
			return false;
		}

		if (i < range_min || i > range_max)
		{
			if (err) err->Format(_T("Line %d column %d: expected integer in range [%d,%d], %d found."), line, c, range_min, range_max, i);
			return false;
		}

		return true;
	}

	bool ReadHex(int& i, int range_min, int range_max, CString* err)
	{
		CString t = ReadToken();
		int c = GetColumn();
		if (t.GetLength() < 1)
		{
			if (err) err->Format(_T("Line %d column %d: expected hexadecimal, no token found."), line, c);
			return false;
		}

		int result = ::sscanf(t, "%x", &i);
		if(result == EOF || result == 0)
		{
			if (err) err->Format(_T("Line %d column %d: expected hexadecimal, '%s' found."), line, c, t);
			return false;
		}

		if (i < range_min || i > range_max)
		{
			if (err) err->Format(_T("Line %d column %d: expected hexidecmal in range [%X,%X], %X found."), line, c, range_min, range_max, i);
			return false;
		}
		return true;
	}

	// note: finishes line if found
	bool ReadEOL(CString* err)
	{
		int c = GetColumn();
		ConsumeSpace();
		CString s = ReadToken();
		if (s.GetLength() > 0)
		{
			if (err) err->Format(_T("Line %d column %d: expected end of line, '%s' found."), line, c, s);
			return false;
		}

		if (Finished()) return true;

		TCHAR eol = text->GetAt(pos);
		if (eol != TCHAR('\r') && eol != TCHAR('\n'))
		{
			if (err) err->Format(_T("Line %d column %d: expected end of line, '%c' found."), line, c, eol);
			return false;
		}

		FinishLine();
		return true;
	}

	// note: finishes line if found
	bool IsEOL()
	{
		ConsumeSpace();
		if (Finished()) return true;

		TCHAR eol = text->GetAt(pos);
		if (eol == TCHAR('\r') || eol == TCHAR('\n'))
		{
			FinishLine();
			return true;
		}

		return false;
	}

	const CString* text;
	int pos;
	int line;
	int linestart;
};

// =============================================================================

bool CTextExport::ImportHex(CString& sToken, int& i, int line, int column, CString& sResult)
{
	i = 0;
	for (int d=0; d < sToken.GetLength(); ++d)
	{
		const TCHAR* HEX_TEXT[16] = {
			_T("0"), _T("1"), _T("2"), _T("3"), _T("4"), _T("5"), _T("6"), _T("7"),
			_T("8"), _T("9"), _T("A"), _T("B"), _T("C"), _T("D"), _T("E"), _T("F") };

		i <<= 4;
		CString t = sToken.Mid(d,1);
		int h = 0;
		for (h=0; h < 16; ++h)
			if (0 == t.CompareNoCase(HEX_TEXT[h])) break;
		if (h >= 16)
		{
			sResult.Format(_T("Line %d column %d: hexadecimal number expected, '%s' found."), line, column, sToken);
			return false;
		}
		i += h;
	}
	return true;
}

CString CTextExport::ExportString(const CString& s)
{
	// puts " at beginning and end of string, replace " with ""
	CString r = _T("\"");
	for (int i=0; i < s.GetLength(); ++i)
	{
		TCHAR c = s.GetAt(i);
		if (c == TCHAR('\"'))
			r += c;
		r += c;
	}	
	r += _T("\"");
	return r;
}

// =============================================================================

bool CTextExport::ImportCellText(		// // //
	CFamiTrackerDoc* pDoc,
	Tokenizer &t,
	unsigned int track,
	unsigned int pattern,
	unsigned int channel,
	unsigned int row,
	CString& sResult)
{
	stChanNote Cell = BLANK_NOTE;		// // //

	CString sNote = t.ReadToken();
	if      (sNote == _T("...")) { Cell.Note = 0; }
	else if (sNote == _T("---")) { Cell.Note = HALT; }
	else if (sNote == _T("===")) { Cell.Note = RELEASE; }
	else
	{
		if (sNote.GetLength() != 3)
		{
			sResult.Format(_T("Line %d column %d: note column should be 3 characters wide, '%s' found."), t.line, t.GetColumn(), sNote);
			return false;
		}

		if (channel == 3) // noise
		{
			int h;
			if (!ImportHex(sNote.Left(1), h, t.line, t.GetColumn(), sResult))
				return false;
			Cell.Note = (h % NOTE_RANGE) + 1;
			Cell.Octave = h / NOTE_RANGE;

			// importer is very tolerant about the second and third characters
			// in a noise note, they can be anything
		}
		else if (sNote.GetAt(0) == TCHAR('^') && sNote.GetAt(1) == TCHAR('-')) {		// // //
			int o = sNote.GetAt(2) - TCHAR('0');
			if (o < 1 || o > ECHO_BUFFER_LENGTH) {
				sResult.Format(_T("Line %d column %d: out-of-bound echo buffer accessed."), t.line, t.GetColumn());
				return false;
			}
			Cell.Note = ECHO;
			Cell.Octave = o;
		}
		else {
			int n = 0;
			switch (sNote.GetAt(0))
			{
				case TCHAR('c'): case TCHAR('C'): n = 0; break;
				case TCHAR('d'): case TCHAR('D'): n = 2; break;
				case TCHAR('e'): case TCHAR('E'): n = 4; break;
				case TCHAR('f'): case TCHAR('F'): n = 5; break;
				case TCHAR('g'): case TCHAR('G'): n = 7; break;
				case TCHAR('a'): case TCHAR('A'): n = 9; break;
				case TCHAR('b'): case TCHAR('B'): n = 11; break;
				default:
					sResult.Format(_T("Line %d column %d: unrecognized note '%s'."), t.line, t.GetColumn(), sNote);
					return false;
			}
			switch (sNote.GetAt(1))
			{
				case TCHAR('-'): case TCHAR('.'): break;
				case TCHAR('#'): case TCHAR('+'): n += 1; break;
				case TCHAR('b'): case TCHAR('f'): n -= 1; break;
				default:
					sResult.Format(_T("Line %d column %d: unrecognized note '%s'."), t.line, t.GetColumn(), sNote);
					return false;
			}
			while (n < 0) n += NOTE_RANGE;
			while (n >= NOTE_RANGE) n -= NOTE_RANGE;
			Cell.Note = n + 1;

			int o = sNote.GetAt(2) - TCHAR('0');
			if (o < 0 || o >= OCTAVE_RANGE)
			{
				sResult.Format(_T("Line %d column %d: unrecognized octave '%s'."), t.line, t.GetColumn(), sNote);
				return false;
			}
			Cell.Octave = o;
		}
	}

	CString sInst = t.ReadToken();
	if (sInst == _T("..")) { Cell.Instrument = MAX_INSTRUMENTS; }
	else
	{
		if (sInst.GetLength() != 2)
		{
			sResult.Format(_T("Line %d column %d: instrument column should be 2 characters wide, '%s' found."), t.line, t.GetColumn(), sInst);
			return false;
		}
		int h;
		if (!ImportHex(sInst, h, t.line, t.GetColumn(), sResult))
			return false;
		if (h >= MAX_INSTRUMENTS)
		{
			sResult.Format(_T("Line %d column %d: instrument '%s' is out of bounds."), t.line, t.GetColumn(), sInst);
			return false;
		}
		Cell.Instrument = h;
	}

	CString sVol  = t.ReadToken();
	const TCHAR* VOL_TEXT[17] = {
		_T("0"), _T("1"), _T("2"), _T("3"), _T("4"), _T("5"), _T("6"), _T("7"),
		_T("8"), _T("9"), _T("A"), _T("B"), _T("C"), _T("D"), _T("E"), _T("F"),
		_T(".") };
	int v = 0;
	for (; v <= 17; ++v)
		if (0 == sVol.CompareNoCase(VOL_TEXT[v])) break;
	if (v > 17)
	{
		sResult.Format(_T("Line %d column %d: unrecognized volume token '%s'."), t.line, t.GetColumn(), sVol);
		return false;
	}
	Cell.Vol = v;

	for (unsigned int e=0; e <= pDoc->GetEffColumns(track, channel); ++e)
	{
		CString sEff = t.ReadToken();
		if (sEff != _T("..."))
		{
			if (sEff.GetLength() != 3)
			{
				sResult.Format(_T("Line %d column %d: effect column should be 3 characters wide, '%s' found."), t.line, t.GetColumn(), sEff);
				return false;
			}

			int p=0;
			TCHAR pC = sEff.GetAt(0);
			if (pC >= TCHAR('a') && pC <= TCHAR('z')) pC += TCHAR('A') - TCHAR('a');
			for (;p < EF_COUNT; ++p)
				if (EFF_CHAR[p] == pC) break;
			// // //
			switch (pDoc->GetChipType(channel)) {
			case SNDCHIP_FDS:
				switch (p - 1) {
				case EF_SWEEPUP:	p = EF_FDS_MOD_DEPTH; break;
				case EF_SWEEPDOWN:	p = EF_FDS_MOD_SPEED_HI; break;
				case EF_VOLUME:		p = EF_FDS_VOLUME; break;
				} break;
			case SNDCHIP_S5B:
				switch (p - 1) {
				case EF_SWEEPUP:			p = EF_SUNSOFT_ENV_LO; break;
				case EF_SWEEPDOWN:			p = EF_SUNSOFT_ENV_HI; break;
				case EF_FDS_MOD_SPEED_LO:	p = EF_SUNSOFT_ENV_TYPE; break;
				} break;
			case SNDCHIP_N163:
				switch (p - 1) {
				case EF_DAC: case EF_SAMPLE_OFFSET: p = EF_N163_WAVE_BUFFER; break;
				} break;
			}
			if (p >= EF_COUNT)
			{
				sResult.Format(_T("Line %d column %d: unrecognized effect '%s'."), t.line, t.GetColumn(), sEff);
				return false;
			}
			Cell.EffNumber[e] = p+1;

			int h;
			if (!ImportHex(sEff.Right(2), h, t.line, t.GetColumn(), sResult))
				return false;
			Cell.EffParam[e] = h;
		}
	}

	pDoc->SetDataAtPattern(track,pattern,channel,row,&Cell);
	return true;
}

const CString& CTextExport::ExportCellText(const stChanNote& stCell, unsigned int nEffects, bool bNoise)		// // //
{
	static CString s;
	CString tmp;
	
	static const char* TEXT_NOTE[ECHO+1] = {		// // //
		_T("..."),
		_T("C-?"), _T("C#?"), _T("D-?"), _T("D#?"), _T("E-?"), _T("F-?"),
		_T("F#?"), _T("G-?"), _T("G#?"), _T("A-?"), _T("A#?"), _T("B-?"),
		_T("==="), _T("---"), _T("^-?") };
		
	s = (stCell.Note <= ECHO) ? TEXT_NOTE[stCell.Note] : "...";		// // //
	if (stCell.Note >= NOTE_C && stCell.Note <= NOTE_B || stCell.Note == ECHO)		// // //
	{
		if (bNoise)
		{
			char nNoiseFreq = (stCell.Note - 1 + stCell.Octave * NOTE_RANGE) & 0x0F;
			s.Format(_T("%01X-#"), nNoiseFreq);
		}
		else
		{
			s = s.Left(2);
			tmp.Format(_T("%01d"), stCell.Octave);
			s += tmp;
		}
	}

	tmp.Format(_T(" %02X"), stCell.Instrument);
	s += (stCell.Instrument == MAX_INSTRUMENTS) ? _T(" ..") : tmp;

	tmp.Format(_T(" %01X"), stCell.Vol);
	s += (stCell.Vol == 0x10) ? _T(" .") : tmp;

	for (unsigned int e=0; e < nEffects; ++e)
	{
		if (stCell.EffNumber[e] == 0)
		{
			s += _T(" ...");
		}
		else
		{
			tmp.Format(_T(" %c%02X"), EFF_CHAR[stCell.EffNumber[e]-1], stCell.EffParam[e]);
			s += tmp;
		}
	}

	return s;
}

// =============================================================================

CTextExport::CTextExport()
{
}

CTextExport::~CTextExport()
{
}

// =============================================================================

#define CHECK(x) { if (!(x)) { return sResult; } }

#define CHECK_SYMBOL(x) \
	{ \
		CString symbol_ = t.ReadToken(); \
		if (symbol_ != _T(x)) \
		{ \
			sResult.Format(_T("Line %d column %d: expected '%s', '%s' found."), t.line, t.GetColumn(), _T(x), symbol_); \
			return sResult; \
		} \
	}

#define CHECK_COLON() CHECK_SYMBOL(":")

const char* CTextExport::Charify(CString& s)		// // //
{
	// NOTE if Famitracker is switched to unicode, need to do a conversion here
	return s.GetString();
}

const CString& CTextExport::ImportFile(LPCTSTR FileName, CFamiTrackerDoc *pDoc)
{
	static CString sResult;
	sResult = _T("");

	// read file into "text" CString
	CString text = _T("");
	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeRead | CFile::typeText, &oFileException))
	{
		TCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);
		
		sResult.Format(_T("Unable to open file:\n%s"), szError);
		return sResult;
	}
	CString line;
	while (f.ReadString(line))
		text += line + TCHAR('\n');
	f.Close();

	// begin a new document
	if (!pDoc->OnNewDocument())
	{
		sResult = _T("Unable to create new Famitracker document.");
		return sResult;
	}

	// parse the file
	Tokenizer t(&text);
	int i; // generic integer for reading
	unsigned int dpcm_index = 0;
	unsigned int dpcm_pos = 0;
	unsigned int track = 0;
	unsigned int pattern = 0;
	bool UseGroove[MAX_TRACKS];		// // //
	while (!t.Finished())
	{
		// read first token on line
		if (t.IsEOL()) continue; // blank line
		CString command = t.ReadToken();

		int c = 0;
		for (; c < CT_COUNT; ++c)
			if (0 == command.CompareNoCase(CT[c])) break;

		//DEBUG_OUT("Command read: %s\n", command);
		switch (c)
		{
			case CT_COMMENTLINE:
				t.FinishLine();
				break;
			case CT_TITLE:
				pDoc->SetSongName(Charify(t.ReadToken()));
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_AUTHOR:
				pDoc->SetSongArtist(Charify(t.ReadToken()));
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_COPYRIGHT:
				pDoc->SetSongCopyright(Charify(t.ReadToken()));
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_COMMENT:
				{
					CString sComment = pDoc->GetComment();
					if (sComment.GetLength() > 0)
						sComment = sComment + _T("\r\n");
					sComment += t.ReadToken();
					pDoc->SetComment(sComment, pDoc->ShowCommentOnOpen());
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_MACHINE:
				CHECK(t.ReadInt(i,0,PAL,&sResult));
				pDoc->SetMachine(i);
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_FRAMERATE:
				CHECK(t.ReadInt(i,0,800,&sResult));
				pDoc->SetEngineSpeed(i);
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_EXPANSION:
				CHECK(t.ReadInt(i,0,255,&sResult));
				pDoc->SelectExpansionChip(i);
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_VIBRATO:
				CHECK(t.ReadInt(i,0,VIBRATO_NEW,&sResult));
				pDoc->SetVibratoStyle((vibrato_t)i);
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_SPLIT:
				CHECK(t.ReadInt(i,0,255,&sResult));
				pDoc->SetSpeedSplitPoint(i);
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_N163CHANNELS:
				CHECK(t.ReadInt(i,1,8,&sResult));
				pDoc->SetNamcoChannels(i);
				pDoc->SelectExpansionChip(pDoc->GetExpansionChip());
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_MACRO:
			case CT_MACROVRC6:
			case CT_MACRON163:
			case CT_MACROS5B:
				{
					const int CHIP_MACRO[4] = { SNDCHIP_NONE, SNDCHIP_VRC6, SNDCHIP_N163, SNDCHIP_S5B };
					int chip = c - CT_MACRO;

					int mt;
					CHECK(t.ReadInt(mt,0,SEQ_COUNT-1,&sResult));
					CHECK(t.ReadInt(i,0,MAX_SEQUENCES-1,&sResult));
					CSequence* pSeq = pDoc->GetSequence(CHIP_MACRO[chip], i, mt);

					CHECK(t.ReadInt(i,-1,MAX_SEQUENCE_ITEMS,&sResult));
					pSeq->SetLoopPoint(i);
					CHECK(t.ReadInt(i,-1,MAX_SEQUENCE_ITEMS,&sResult));
					pSeq->SetReleasePoint(i);
					CHECK(t.ReadInt(i,0,255,&sResult));
					pSeq->SetSetting(static_cast<seq_setting_t>(i));		// // //

					CHECK_COLON();

					int count = 0;
					while (!t.IsEOL())
					{
						CHECK(t.ReadInt(i,-128,127,&sResult));
						if (count >= MAX_SEQUENCE_ITEMS)
						{
							sResult.Format(_T("Line %d column %d: macro overflow, max size: %d."), t.line, t.GetColumn(), MAX_SEQUENCE_ITEMS);
							return sResult;
						}
						pSeq->SetItem(count, i);
						++count;
					}
					pSeq->SetItemCount(count);
				}
				break;
			case CT_DPCMDEF:
				{
					CHECK(t.ReadInt(i,0,MAX_DSAMPLES-1,&sResult));
					dpcm_index = i;
					dpcm_pos = 0;

					CHECK(t.ReadInt(i,0,CDSample::MAX_SIZE,&sResult));
					CDSample* pSample = pDoc->GetSample(dpcm_index);
					pSample->Allocate(i, NULL);
					::memset(pSample->GetData(), 0, i);
					pSample->SetName(Charify(t.ReadToken()));

					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_DPCM:
				{
					CDSample* pSample = pDoc->GetSample(dpcm_index);
					CHECK_COLON();
					while (!t.IsEOL())
					{
						CHECK(t.ReadHex(i,0x00,0xFF,&sResult));
						if (dpcm_pos >= pSample->GetSize())
						{
							sResult.Format(_T("Line %d column %d: DPCM sample %d overflow, increase size used in %s."), t.line, t.GetColumn(), dpcm_index, CT[CT_DPCMDEF]);
							return sResult;
						}
						*(pSample->GetData() + dpcm_pos) = (char)(i);
						++dpcm_pos;
					}
				}
				break;
			case CT_DETUNE:		// // //
				{
					int oct, note, offset;
					CHECK(t.ReadInt(i,0,5,&sResult));
					CHECK(t.ReadInt(oct,0,OCTAVE_RANGE-1,&sResult));
					CHECK(t.ReadInt(note,0,NOTE_RANGE-1,&sResult));
					CHECK(t.ReadInt(offset,-32768,32767,&sResult));
					pDoc->SetDetuneOffset(i, oct * NOTE_RANGE + note, offset);
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_GROOVE:		// // //
				{
					int size, entry;
					CHECK(t.ReadInt(i,0,MAX_GROOVE-1,&sResult));
					CHECK(t.ReadInt(size,1,MAX_GROOVE_SIZE,&sResult));
					CGroove *Groove = new CGroove();
					Groove->SetSize(size);
					CHECK_COLON();
					for (int j = 0; j < size; j++) {
						CHECK(t.ReadInt(entry,1,255,&sResult));
						Groove->SetEntry(j, entry);
					}
					pDoc->SetGroove(i, Groove);
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_USEGROOVE:		// // //
				{
					CHECK_COLON();
					int oldTrack = track;
					while (!t.IsEOL()) {
						CHECK(t.ReadInt(i,1,MAX_TRACKS,&sResult));
						i--;
						UseGroove[i] = true;
						if (static_cast<unsigned int>(i) < pDoc->GetTrackCount()) {
							pDoc->SetSongGroove(i, true);
						}
					}
				}
				break;
			case CT_INST2A03:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					CInstrument2A03* pInst = (CInstrument2A03*)pDoc->CreateInstrument(INST_2A03);
					pDoc->AddInstrument(pInst, i);
					for (int s=0; s < SEQ_COUNT; ++s)
					{
						CHECK(t.ReadInt(i,-1,MAX_SEQUENCES-1,&sResult));
						pInst->SetSeqEnable(s, (i == -1) ? 0 : 1);
						pInst->SetSeqIndex(s, (i == -1) ? 0 : i);
					}
					pInst->SetName(Charify(t.ReadToken()));
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_INSTVRC6:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					CInstrumentVRC6* pInst = (CInstrumentVRC6*)pDoc->CreateInstrument(INST_VRC6);
					pDoc->AddInstrument(pInst, i);
					for (int s=0; s < SEQ_COUNT; ++s)
					{
						CHECK(t.ReadInt(i,-1,MAX_SEQUENCES-1,&sResult));
						pInst->SetSeqEnable(s, (i == -1) ? 0 : 1);
						pInst->SetSeqIndex(s, (i == -1) ? 0 : i);
					}
					pInst->SetName(Charify(t.ReadToken()));
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_INSTVRC7:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					CInstrumentVRC7* pInst = (CInstrumentVRC7*)pDoc->CreateInstrument(INST_VRC7);
					pDoc->AddInstrument(pInst, i);
					CHECK(t.ReadInt(i,0,15,&sResult));
					pInst->SetPatch(i);
					for (int r=0; r < 8; ++r)
					{
						CHECK(t.ReadHex(i,0x00,0xFF,&sResult));
						pInst->SetCustomReg(r, i);
					}
					pInst->SetName(Charify(t.ReadToken()));
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_INSTFDS:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					CInstrumentFDS* pInst = (CInstrumentFDS*)pDoc->CreateInstrument(INST_FDS);
					pDoc->AddInstrument(pInst, i);
					CHECK(t.ReadInt(i,0,1,&sResult));
					pInst->SetModulationEnable(i==1);
					CHECK(t.ReadInt(i,0,4095,&sResult));
					pInst->SetModulationSpeed(i);
					CHECK(t.ReadInt(i,0,63,&sResult));
					pInst->SetModulationDepth(i);
					CHECK(t.ReadInt(i,0,255,&sResult));
					pInst->SetModulationDelay(i);
					pInst->SetName(Charify(t.ReadToken()));
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_INSTN163:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					CInstrumentN163* pInst = (CInstrumentN163*)pDoc->CreateInstrument(INST_N163);
					pDoc->AddInstrument(pInst, i);
					for (int s=0; s < SEQ_COUNT; ++s)
					{
						CHECK(t.ReadInt(i,-1,MAX_SEQUENCES-1,&sResult));
						pInst->SetSeqEnable(s, (i == -1) ? 0 : 1);
						pInst->SetSeqIndex(s, (i == -1) ? 0 : i);
					}
					CHECK(t.ReadInt(i,0,256-16*pDoc->GetNamcoChannels(),&sResult));		// // //
					pInst->SetWaveSize(i);
					CHECK(t.ReadInt(i,0,256-16*pDoc->GetNamcoChannels()-1,&sResult));		// // //
					pInst->SetWavePos(i);
					CHECK(t.ReadInt(i,0,CInstrumentN163::MAX_WAVE_COUNT,&sResult));
					pInst->SetWaveCount(i);
					pInst->SetName(Charify(t.ReadToken()));
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_INSTS5B:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					CInstrumentS5B* pInst = (CInstrumentS5B*)pDoc->CreateInstrument(INST_S5B);
					pDoc->AddInstrument(pInst, i);
					for (int s=0; s < SEQ_COUNT; ++s)
					{
						CHECK(t.ReadInt(i,-1,MAX_SEQUENCES-1,&sResult));
						pInst->SetSeqEnable(s, (i == -1) ? 0 : 1);
						pInst->SetSeqIndex(s, (i == -1) ? 0 : i);
					}
					pInst->SetName(Charify(t.ReadToken()));
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_KEYDPCM:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					if (pDoc->GetInstrumentType(i) != INST_2A03)
					{
						sResult.Format(_T("Line %d column %d: instrument %d is not defined as a 2A03 instrument."), t.line, t.GetColumn(), i);
						return sResult;
					}
					CInstrument2A03* pInst = (CInstrument2A03*)pDoc->GetInstrument(i);

					int io, in;
					CHECK(t.ReadInt(io,0,OCTAVE_RANGE,&sResult));
					CHECK(t.ReadInt(in,0,12,&sResult));

					CHECK(t.ReadInt(i,0,MAX_DSAMPLES-1,&sResult));
					pInst->SetSample(io, in, i+1);
					CHECK(t.ReadInt(i,0,15,&sResult));
					pInst->SetSamplePitch(io, in, i);
					CHECK(t.ReadInt(i,0,1,&sResult));
					pInst->SetSampleLoop(io, in, i==1);
					CHECK(t.ReadInt(i,0,255,&sResult));
					pInst->SetSampleLoopOffset(io, in, i);
					CHECK(t.ReadInt(i,-1,127,&sResult));
					pInst->SetSampleDeltaValue(io, in, i);

					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_FDSWAVE:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					if (pDoc->GetInstrumentType(i) != INST_FDS)
					{
						sResult.Format(_T("Line %d column %d: instrument %d is not defined as an FDS instrument."), t.line, t.GetColumn(), i);
						return sResult;
					}
					CInstrumentFDS* pInst = (CInstrumentFDS*)pDoc->GetInstrument(i);
					CHECK_COLON();
					for (int s=0; s < CInstrumentFDS::WAVE_SIZE; ++s)
					{
						CHECK(t.ReadInt(i,0,63,&sResult));
						pInst->SetSample(s, i);
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_FDSMOD:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					if (pDoc->GetInstrumentType(i) != INST_FDS)
					{
						sResult.Format(_T("Line %d column %d: instrument %d is not defined as an FDS instrument."), t.line, t.GetColumn(), i);
						return sResult;
					}
					CInstrumentFDS* pInst = (CInstrumentFDS*)pDoc->GetInstrument(i);
					CHECK_COLON();
					for (int s=0; s < CInstrumentFDS::MOD_SIZE; ++s)
					{
						CHECK(t.ReadInt(i,0,7,&sResult));
						pInst->SetModulation(s, i);
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_FDSMACRO:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					if (pDoc->GetInstrumentType(i) != INST_FDS)
					{
						sResult.Format(_T("Line %d column %d: instrument %d is not defined as an FDS instrument."), t.line, t.GetColumn(), i);
						return sResult;
					}
					CInstrumentFDS* pInst = (CInstrumentFDS*)pDoc->GetInstrument(i);

					CHECK(t.ReadInt(i,0,2,&sResult));
					CSequence * pSeq = NULL;
					switch(i)
					{
						case 0:
							pSeq = pInst->GetVolumeSeq();
							break;
						case 1:
							pSeq = pInst->GetArpSeq();
							break;
						case 2:
							pSeq = pInst->GetPitchSeq();
							break;
						default:
							sResult.Format(_T("Line %d column %d: unexpected error."), t.line, t.GetColumn());
							return sResult;
					}
					CHECK(t.ReadInt(i,-1,MAX_SEQUENCE_ITEMS,&sResult));
					pSeq->SetLoopPoint(i);
					CHECK(t.ReadInt(i,-1,MAX_SEQUENCE_ITEMS,&sResult));
					pSeq->SetReleasePoint(i);
					CHECK(t.ReadInt(i,0,255,&sResult));
					pSeq->SetSetting(static_cast<seq_setting_t>(i));		// // //

					CHECK_COLON();

					int count = 0;
					while (!t.IsEOL())
					{
						CHECK(t.ReadInt(i,-128,127,&sResult));
						if (count >= MAX_SEQUENCE_ITEMS)
						{
							sResult.Format(_T("Line %d column %d: macro overflow, max size: %d."), t.line, t.GetColumn(), MAX_SEQUENCE_ITEMS);
							return sResult;
						}
						pSeq->SetItem(count, i);
						++count;
					}
					pSeq->SetItemCount(count);
				}
				break;
			case CT_N163WAVE:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					if (pDoc->GetInstrumentType(i) != INST_N163)
					{
						sResult.Format(_T("Line %d column %d: instrument %d is not defined as an N163 instrument."), t.line, t.GetColumn(), i);
						return sResult;
					}
					CInstrumentN163* pInst = (CInstrumentN163*)pDoc->GetInstrument(i);

					int iw;
					CHECK(t.ReadInt(iw,0,CInstrumentN163::MAX_WAVE_COUNT-1,&sResult));
					CHECK_COLON();
					for (int s=0; s < pInst->GetWaveSize(); ++s)
					{
						CHECK(t.ReadInt(i,0,15,&sResult));
						pInst->SetSample(iw, s, i);
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_TRACK:
				{
					if (track != 0)
					{
						if(pDoc->AddTrack() == -1)
						{
							sResult.Format(_T("Line %d column %d: unable to add new track."), t.line, t.GetColumn());
							return sResult;
						}
					}
					
					CHECK(t.ReadInt(i,1,MAX_PATTERN_LENGTH,&sResult));		// // //
					pDoc->SetPatternLength(track, i);
					pDoc->SetSongGroove(track, UseGroove[track]);		// // //
					CHECK(t.ReadInt(i,0,MAX_TEMPO,&sResult));
					pDoc->SetSongSpeed(track, i);
					CHECK(t.ReadInt(i,0,MAX_TEMPO,&sResult));
					pDoc->SetSongTempo(track, i);
					pDoc->SetTrackTitle(track, t.ReadToken());

					CHECK(t.ReadEOL(&sResult));
					++track;
				}
				break;
			case CT_COLUMNS:
				{
					CHECK_COLON();
					for (int c=0; c < pDoc->GetChannelCount(); ++c)
					{
						CHECK(t.ReadInt(i,1,MAX_EFFECT_COLUMNS,&sResult));
						pDoc->SetEffColumns(track-1,c,i-1);
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_ORDER:
				{
					int ifr;
					CHECK(t.ReadHex(ifr,0,MAX_FRAMES-1,&sResult));
					if (ifr >= (int)pDoc->GetFrameCount(track-1)) // expand to accept frames
					{
						pDoc->SetFrameCount(track-1,ifr+1);
					}
					CHECK_COLON();
					for (int c=0; c < pDoc->GetChannelCount(); ++c)
					{
						CHECK(t.ReadHex(i,0,MAX_PATTERN-1,&sResult));
						pDoc->SetPatternAtFrame(track-1,ifr, c, i);
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_PATTERN:
				CHECK(t.ReadHex(i,0,MAX_PATTERN-1,&sResult));
				pattern = i;
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_ROW:
				{
					if (track == 0)
					{
						sResult.Format(_T("Line %d column %d: no TRACK defined, cannot add ROW data."), t.line, t.GetColumn());
						return sResult;
					}

					CHECK(t.ReadHex(i,0,MAX_PATTERN_LENGTH-1,&sResult));
					for (int c=0; c < pDoc->GetChannelCount(); ++c)
					{
    					CHECK_COLON();
						if (!ImportCellText(pDoc, t, track-1, pattern, c, i, sResult))
						{
							return sResult;
						}
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_COUNT:
			default:
				sResult.Format(_T("Unrecognized command at line %d: '%s'."), t.line, command);
				return sResult;
		}
	}

	return sResult;
}

// =============================================================================

const CString& CTextExport::ExportRows(LPCTSTR FileName, CFamiTrackerDoc *pDoc)
{
	static CString sResult;
	sResult = _T("");

	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &oFileException))
	{
		TCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);
		
		sResult.Format(_T("Unable to open file:\n%s"), szError);
		return sResult;
	}

	f.WriteString(_T("ID,TRACK,CHANNEL,PATTERN,ROW,NOTE,OCTAVE,INST,VOLUME,FX1,FX1PARAM,FX2,FX2PARAM,FX3,FX3PARAM,FX4,FX4PARAM\n"));

	CString l, s = _T("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n");
	stChanNote stCell;
	int id = 0;
	
	for (unsigned int t = 0; t < pDoc->GetTrackCount(); t++)
		for (int c = 0; c < pDoc->GetChannelCount(); c++)
			for (int p = 0; p < MAX_PATTERN; p++)
				for (unsigned int r = 0; r < pDoc->GetPatternLength(t); r++) {
					pDoc->GetDataAtPattern(t,p,c,r,&stCell);
					bool isEmpty = true;
					if (stCell.Note != NONE || stCell.Instrument != MAX_INSTRUMENTS || stCell.Vol != 0x10) isEmpty = false;
					for (int fx = 0; fx < MAX_EFFECT_COLUMNS; fx++)
						if (stCell.EffNumber[fx] != EF_NONE) isEmpty = false;
					if (isEmpty) continue;
					l.Format(s, id++, t, c, p, r, stCell.Note, stCell.Octave, stCell.Instrument, stCell.Vol,
						stCell.EffNumber[0], stCell.EffParam[0],
						stCell.EffNumber[1], stCell.EffParam[1],
						stCell.EffNumber[2], stCell.EffParam[2],
						stCell.EffNumber[3], stCell.EffParam[3]);
					f.WriteString(l);
				}
	return sResult;
}

const CString& CTextExport::ExportFile(LPCTSTR FileName, CFamiTrackerDoc *pDoc)
{
	static CString sResult;
	sResult = _T("");

	CStdioFile f;
	CFileException oFileException;
	if (!f.Open(FileName, CFile::modeCreate | CFile::modeWrite | CFile::typeText, &oFileException))
	{
		TCHAR szError[256];
		oFileException.GetErrorMessage(szError, 256);
		
		sResult.Format(_T("Unable to open file:\n%s"), szError);
		return sResult;
	}

	CString s;

	f.WriteString(_T("# FamiTracker text export 0.4.2\n\n"));

	s.Format(_T("# Song information\n"
	            "%-15s %s\n"
	            "%-15s %s\n"
	            "%-15s %s\n"
	            "\n"),
	            CT[CT_TITLE],     ExportString(pDoc->GetSongName()),
	            CT[CT_AUTHOR],    ExportString(pDoc->GetSongArtist()),
	            CT[CT_COPYRIGHT], ExportString(pDoc->GetSongCopyright()));
	f.WriteString(s);

	f.WriteString(_T("# Song comment\n"));
	CString sComment = pDoc->GetComment();
	bool bCommentLines = false;
	do
	{
		int nPos = sComment.Find(TCHAR('\r'));
		bCommentLines = (nPos >= 0);
		if (bCommentLines)
		{
			CString sLine = sComment.Left(nPos);
			s.Format(_T("%s %s\n"), CT[CT_COMMENT], ExportString(sLine));
			f.WriteString(s);
			sComment = sComment.Mid(nPos+2); // +2 skips \r\n
		}
		else
		{
			s.Format(_T("%s %s\n"), CT[CT_COMMENT], ExportString(sComment));
			f.WriteString(s);
		}
	} while (bCommentLines);
	f.WriteString(_T("\n"));

	s.Format(_T("# Global settings\n"
	            "%-15s %d\n"
	            "%-15s %d\n"
	            "%-15s %d\n"
	            "%-15s %d\n"
	            "%-15s %d\n"
	            "\n"),
	            CT[CT_MACHINE],   pDoc->GetMachine(),
	            CT[CT_FRAMERATE], pDoc->GetEngineSpeed(),
	            CT[CT_EXPANSION], pDoc->GetExpansionChip(),
	            CT[CT_VIBRATO],   pDoc->GetVibratoStyle(),
	            CT[CT_SPLIT],     pDoc->GetSpeedSplitPoint() );
	f.WriteString(s);

	if (pDoc->ExpansionEnabled(SNDCHIP_N163))
	{
		s.Format(_T("# Namco 163 global settings\n"
		            "%-15s %d\n"
		            "\n"),
		            CT[CT_N163CHANNELS], pDoc->GetNamcoChannels());
		f.WriteString(s);
	}

	f.WriteString(_T("# Macros\n"));
	for (int c=0; c<4; ++c)
	{
		const int CHIP_MACRO[4] = { SNDCHIP_NONE, SNDCHIP_VRC6, SNDCHIP_N163, SNDCHIP_S5B };
		int chip = CHIP_MACRO[c];

		for (int st=0; st < SEQ_COUNT; ++st)
		for (int seq=0; seq < MAX_SEQUENCES; ++seq)
		{
			CSequence* pSequence = pDoc->GetSequence(chip, seq, st);
			if (pSequence && pSequence->GetItemCount() > 0)
			{
				s.Format(_T("%-9s %3d %3d %3d %3d %3d :"),
					CT[CT_MACRO+c],
					st,
					seq,
					pSequence->GetLoopPoint(),
					pSequence->GetReleasePoint(),
					pSequence->GetSetting());
				f.WriteString(s);
				for (unsigned int i=0; i < pSequence->GetItemCount(); ++i)
				{
					s.Format(_T(" %d"), pSequence->GetItem(i));
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));
			}
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# DPCM samples\n"));
	for (int smp=0; smp < MAX_DSAMPLES; ++smp)
	{
		const CDSample* pSample = pDoc->GetSample(smp);
		if (pSample && pSample->GetSize() > 0)
		{
			s.Format(_T("%s %3d %5d %s\n"),
				CT[CT_DPCMDEF],
				smp,
				pSample->GetSize(),
				ExportString(pSample->GetName()));
			f.WriteString(s);

			for (unsigned int i=0; i < pSample->GetSize(); i += 32)
			{
				s.Format(_T("%s :"), CT[CT_DPCM]);
				f.WriteString(s);
				for (unsigned int j=0; j<32 && (i+j)<pSample->GetSize(); ++j)
				{
					s.Format(_T(" %02X"), (unsigned char)(*(pSample->GetData() + (i+j))));
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));
			}
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# Detune settings\n"));		// // //
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++) {
		int Offset = pDoc->GetDetuneOffset(i, j);
		if (Offset != 0) {
			s.Format(_T("%s %3d %3d %3d %5d\n"), CT[CT_DETUNE], i, j / NOTE_RANGE, j % NOTE_RANGE, Offset);
			f.WriteString(s);
		}
	}
	f.WriteString(_T("\n"));
	
	f.WriteString(_T("# Grooves\n"));		// // //
	for (int i = 0; i < MAX_GROOVE; i++) {
		CGroove *Groove = pDoc->GetGroove(i);
		if (Groove != NULL) {
			s.Format(_T("%s %3d %3d :"), CT[CT_GROOVE], i, Groove->GetSize());
			f.WriteString(s);
			for (int j = 0; j < Groove->GetSize(); j++) {
				s.Format(" %d", Groove->GetEntry(j));
				f.WriteString(s);
			}
			f.WriteString(_T("\n"));
		}
	}
	f.WriteString(_T("\n"));
	
	f.WriteString(_T("# Tracks using default groove\n"));		// // //
	bool UsedGroove = false;
	for (unsigned int i = 0; i < pDoc->GetTrackCount(); i++)
		if (pDoc->GetSongGroove(i)) UsedGroove = true;
	if (UsedGroove) {
		s.Format(_T("%s :"), CT[CT_USEGROOVE]);
		f.WriteString(s);
		for (unsigned int i = 0; i < pDoc->GetTrackCount(); i++) if (pDoc->GetSongGroove(i)) {
			s.Format(_T(" %d"), i + 1);
			f.WriteString(s);
		}
		f.WriteString(_T("\n\n"));
	}
	
	f.WriteString(_T("# Instruments\n"));
	for (unsigned int i=0; i<MAX_INSTRUMENTS; ++i)
	{
		CInstrument* pInst = pDoc->GetInstrument(i);
		if (!pInst) continue;

		switch (pInst->GetType())
		{
			default:
			case INST_NONE:
				break;
			case INST_2A03:
				{
					CInstrument2A03* pDI = (CInstrument2A03*)pInst;
					s.Format(_T("%-8s %3d   %3d %3d %3d %3d %3d %s\n"),
						CT[CT_INST2A03],
						i,
						pDI->GetSeqEnable(0) ? pDI->GetSeqIndex(0) : -1,
						pDI->GetSeqEnable(1) ? pDI->GetSeqIndex(1) : -1,
						pDI->GetSeqEnable(2) ? pDI->GetSeqIndex(2) : -1,
						pDI->GetSeqEnable(3) ? pDI->GetSeqIndex(3) : -1,
						pDI->GetSeqEnable(4) ? pDI->GetSeqIndex(4) : -1,
						ExportString(pInst->GetName()));
					f.WriteString(s);

					for (int oct = 0; oct < OCTAVE_RANGE; ++oct)
					for (int key = 0; key < NOTE_RANGE; ++key)
					{
						int smp = pDI->GetSample(oct, key);
						if (smp != 0)
						{
							int d = pDI->GetSampleDeltaValue(oct, key);
							s.Format(_T("%s %3d %3d %3d   %3d %3d %3d %5d %3d\n"),
								CT[CT_KEYDPCM],
								i,
								oct, key,
								smp - 1,
								pDI->GetSamplePitch(oct, key) & 0x0F,
								pDI->GetSampleLoop(oct, key) ? 1 : 0,
								pDI->GetSampleLoopOffset(oct, key),
								(d >= 0 && d <= 127) ? d : -1);
							f.WriteString(s);
						}
					}
				}
				break;
			case INST_VRC6:
				{
					CInstrumentVRC6* pDI = (CInstrumentVRC6*)pInst;
					s.Format(_T("%-8s %3d   %3d %3d %3d %3d %3d %s\n"),
						CT[CT_INSTVRC6],
						i,
						pDI->GetSeqEnable(0) ? pDI->GetSeqIndex(0) : -1,
						pDI->GetSeqEnable(1) ? pDI->GetSeqIndex(1) : -1,
						pDI->GetSeqEnable(2) ? pDI->GetSeqIndex(2) : -1,
						pDI->GetSeqEnable(3) ? pDI->GetSeqIndex(3) : -1,
						pDI->GetSeqEnable(4) ? pDI->GetSeqIndex(4) : -1,
						ExportString(pInst->GetName()));
					f.WriteString(s);
				}
				break;
			case INST_VRC7:
				{
					CInstrumentVRC7* pDI = (CInstrumentVRC7*)pInst;
					s.Format(_T("%-8s %3d   %3d %02X %02X %02X %02X %02X %02X %02X %02X %s\n"),
						CT[CT_INSTVRC7],
						i,
						pDI->GetPatch(),
						pDI->GetCustomReg(0) & 0xFF,
						pDI->GetCustomReg(1) & 0xFF,
						pDI->GetCustomReg(2) & 0xFF,
						pDI->GetCustomReg(3) & 0xFF,
						pDI->GetCustomReg(4) & 0xFF,
						pDI->GetCustomReg(5) & 0xFF,
						pDI->GetCustomReg(6) & 0xFF,
						pDI->GetCustomReg(7) & 0xFF,
						ExportString(pInst->GetName()));
					f.WriteString(s);
				}
				break;
			case INST_FDS:
				{
					CInstrumentFDS* pDI = (CInstrumentFDS*)pInst;
					s.Format(_T("%-8s %3d   %3d %3d %3d %3d %s\n"),
						CT[CT_INSTFDS],
						i,
						pDI->GetModulationEnable(),
						pDI->GetModulationSpeed(),
						pDI->GetModulationDepth(),
						pDI->GetModulationDelay(),
						ExportString(pInst->GetName()));
					f.WriteString(s);

					s.Format(_T("%-8s %3d :"), CT[CT_FDSWAVE], i);
					f.WriteString(s);
					for (int smp=0; smp < CInstrumentFDS::WAVE_SIZE; ++smp)
					{
						s.Format(_T(" %2d"), pDI->GetSample(smp));
						f.WriteString(s);
					}
					f.WriteString(_T("\n"));

					s.Format(_T("%-8s %3d :"), CT[CT_FDSMOD], i);
					f.WriteString(s);
					for (int smp=0; smp < CInstrumentFDS::MOD_SIZE; ++smp)
					{
						s.Format(_T(" %2d"), pDI->GetModulation(smp));
						f.WriteString(s);
					}
					f.WriteString(_T("\n"));

					CSequence* pSeq[3] = { pDI->GetVolumeSeq(), pDI->GetArpSeq(), pDI->GetPitchSeq() };
					for (int seq=0; seq < 3; ++seq)
					{
						CSequence* pSequence = pSeq[seq];
						if (!pSequence || pSequence->GetItemCount() < 1) continue;

						s.Format(_T("%-8s %3d %3d %3d %3d %3d :"),
							CT[CT_FDSMACRO],
							i,
							seq,
							pSequence->GetLoopPoint(),
							pSequence->GetReleasePoint(),
							pSequence->GetSetting());
						f.WriteString(s);
						for (unsigned int i=0; i < pSequence->GetItemCount(); ++i)
						{
							s.Format(_T(" %d"), pSequence->GetItem(i));
							f.WriteString(s);
						}
						f.WriteString(_T("\n"));
					}
				}
				break;
			case INST_N163:
				{
					CInstrumentN163* pDI = (CInstrumentN163*)pInst;
					s.Format(_T("%-8s %3d   %3d %3d %3d %3d %3d %3d %3d %3d %s\n"),
						CT[CT_INSTN163],
						i,
						pDI->GetSeqEnable(0) ? pDI->GetSeqIndex(0) : -1,
						pDI->GetSeqEnable(1) ? pDI->GetSeqIndex(1) : -1,
						pDI->GetSeqEnable(2) ? pDI->GetSeqIndex(2) : -1,
						pDI->GetSeqEnable(3) ? pDI->GetSeqIndex(3) : -1,
						pDI->GetSeqEnable(4) ? pDI->GetSeqIndex(4) : -1,
						pDI->GetWaveSize(),
						pDI->GetWavePos(),
						pDI->GetWaveCount(),
						ExportString(pInst->GetName()));
					f.WriteString(s);

					for (int w=0; w < pDI->GetWaveCount(); ++w)
					{
						s.Format(_T("%s %3d %3d :"),
							CT[CT_N163WAVE], i, w);
						f.WriteString(s);

						for (int smp=0; smp < pDI->GetWaveSize(); ++smp)
						{
							s.Format(_T(" %d"), pDI->GetSample(w, smp));
							f.WriteString(s);
						}
						f.WriteString(_T("\n"));
					}
				}
				break;
			case INST_S5B:
				{
					CInstrumentS5B* pDI = (CInstrumentS5B*)pInst;
					s.Format(_T("%-8s %3d   %3d %3d %3d %3d %3d %s\n"),
						CT[CT_INSTS5B],
						i,
						pDI->GetSeqEnable(0) ? pDI->GetSeqIndex(0) : -1,
						pDI->GetSeqEnable(1) ? pDI->GetSeqIndex(1) : -1,
						pDI->GetSeqEnable(2) ? pDI->GetSeqIndex(2) : -1,
						pDI->GetSeqEnable(3) ? pDI->GetSeqIndex(3) : -1,
						pDI->GetSeqEnable(4) ? pDI->GetSeqIndex(4) : -1,
						ExportString(pInst->GetName()));
					f.WriteString(s);
				}
				break;
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# Tracks\n\n"));

	for (unsigned int t=0; t < pDoc->GetTrackCount(); ++t)
	{
		const char* zpTitle = pDoc->GetTrackTitle(t).GetString();
		if (zpTitle == NULL) zpTitle = "";

		s.Format(_T("%s %3d %3d %3d %s\n"),
			CT[CT_TRACK],
			pDoc->GetPatternLength(t),
			pDoc->GetSongSpeed(t),
			pDoc->GetSongTempo(t),
			ExportString(zpTitle));
		f.WriteString(s);

		s.Format(_T("%s :"), CT[CT_COLUMNS]);
		f.WriteString(s);
		for (int c=0; c < pDoc->GetChannelCount(); ++c)
		{
			s.Format(_T(" %d"), pDoc->GetEffColumns(t, c)+1);
			f.WriteString(s);
		}
		f.WriteString(_T("\n\n"));

		for (unsigned int o=0; o < pDoc->GetFrameCount(t); ++o)
		{
			s.Format(_T("%s %02X :"), CT[CT_ORDER], o);
			f.WriteString(s);
			for (int c=0; c < pDoc->GetChannelCount(); ++c)
			{
				s.Format(_T(" %02X"), pDoc->GetPatternAtFrame(t, o, c));
				f.WriteString(s);
			}
			f.WriteString(_T("\n"));
		}
		f.WriteString(_T("\n"));

		for (int p=0; p < MAX_PATTERN; ++p)
		{
			// detect and skip empty patterns
			bool bUsed = false;
			for (int c=0; c < pDoc->GetChannelCount(); ++c)
			{
				if (!pDoc->IsPatternEmpty(t, c, p))
				{
					bUsed = true;
					break;
				}
			}
			if (!bUsed) continue;

			s.Format(_T("%s %02X\n"), CT[CT_PATTERN], p);
			f.WriteString(s);

			for (unsigned int r=0; r < pDoc->GetPatternLength(t); ++r)
			{
				s.Format(_T("%s %02X"), CT[CT_ROW], r);
				f.WriteString(s);
				for (int c=0; c < pDoc->GetChannelCount(); ++c)
				{
					f.WriteString(_T(" : "));
					stChanNote stCell;
					pDoc->GetDataAtPattern(t,p,c,r,&stCell);
					f.WriteString(ExportCellText(stCell, pDoc->GetEffColumns(t, c)+1, c==3));
				}
				f.WriteString(_T("\n"));
			}
			f.WriteString(_T("\n"));
		}
	}

	f.WriteString(_T("# End of export\n"));
	return sResult;
}

// end of file
