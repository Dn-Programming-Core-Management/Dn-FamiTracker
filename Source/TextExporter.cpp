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

#include "stdafx.h"
#include "FamiTrackerTypes.h"		// // //
#include "PatternData.h"		// // //
#include "TextExporter.h"
#include "FamiTrackerDoc.h"
#include "../version.h"		// // //
#include "Bookmark.h"		// !! !!
#include "BookmarkCollection.h"		// !! !!
#include "BookmarkManager.h"		// !! !!
#include "DocumentFile.h"		// !! !!

#include "DSample.h"		// // //
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"
#include "InstrumentFDS.h"
#include "InstrumentN163.h"
#include "InstrumentVRC7.h"
#include "InstrumentFactory.h"

#define DEBUG_OUT(...) { CString s__; s__.Format(__VA_ARGS__); OutputDebugString(s__); }

// command tokens
enum
{
	CT_COMMENTLINE,		// anything may follow
	// INFO block
	CT_TITLE,			// string
	CT_AUTHOR,			// string
	CT_COPYRIGHT,		// string
	// COMMENTS block
	CT_COMMENT,			// string (concatenates line)
	// PARAMS block
	CT_MACHINE,			// uint (0=NTSC, 1=PAL)
	CT_FRAMERATE,		// uint (0=default)
	CT_EXPANSION,		// uint (0=none, 1=VRC6, 2=VRC7, 4=FDS, 8=MMC5, 16=N163, 32=S5B)
	CT_VIBRATO,			// uint (0=old, 1=new)
	CT_SPLIT,			// uint (32=default)
	// // // 050B
	CT_PLAYBACKRATE,	// uint (0=default, 1=custom, 2=video) uint (us)
	CT_TUNING,			// uint (semitones) uint (cents)

	CT_N163CHANNELS,   // uint
	// SEQUENCES block
	CT_MACRO,			// uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	CT_MACROVRC6,		// uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	CT_MACRON163,		// uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	CT_MACROS5B,		// uint (type) uint (index) int (loop) int (release) int (setting) : int_list
	// DPCM SAMPLES block
	CT_DPCMDEF,			// uint (index) uint (size) string (name)
	CT_DPCM,			// : hex_list
	// // // DETUNETABLES block
	CT_DETUNE,			// uint (chip) uint (oct) uint (note) int (offset)
	// // // GROOVES block
	CT_GROOVE,			// uint (index) uint (size) : int_list
	CT_USEGROOVE,		// : int_list
	// INSTRUMENTS block
	CT_INST2A03,		// uint (index) int int int int int string (name)
	CT_INSTVRC6,		// uint (index) int int int int int string (name)
	CT_INSTVRC7,		// uint (index) int (patch) hex hex hex hex hex hex hex hex string (name)
	CT_INSTFDS,			// uint (index) int (mod enable) int (m speed) int (m depth) int (m delay) string (name)
	CT_INSTN163,		// uint (index) int int int int int uint (w size) uint (w pos) uint (w count) string (name)
	CT_INSTS5B,			// uint (index) int int int int int  string (name)

	CT_KEYDPCM,			// uint (inst) uint (oct) uint (note) uint (sample) uint (pitch) uint (loop) uint (loop_point)
	CT_FDSWAVE,			// uint (inst) : uint_list x 64
	CT_FDSMOD,			// uint (inst) : uint_list x 32
	CT_FDSMACRO,		// uint (inst) uint (type) int (loop) int (release) int (setting) : int_list
	CT_N163WAVE,		// uint (inst) uint (wave) : uint_list
	// HEADER block
	CT_TRACK,			// uint (pat length) uint (speed) uint (tempo) string (name)
	CT_COLUMNS,			// : uint_list (effect columns)
	// FRAMES block
	CT_ORDER,			// hex (frame) : hex_list
	// PATTERNS block
	CT_PATTERN,			// hex (pattern)
	CT_ROW,				// row data
	// BOOKMARKS block
	CT_BOOKMARK,		// hex (frame) hex (row) int (highlight_1) int (highlight_2) uint (persist; 0 = false, 1 = true) string (name)
	// PARAMS_EXTRA block
	CT_LINEARPITCH,		// uint (0 = linear period, 1 = linear pitch)
	// JSON block
	CT_JSON,			// string (JSON data)
	// PARAMS_EMU block
	CT_USEEXTOPLL,		// uint (0 = VRC7, 1 = external OPLL)
	CT_OPLLPATCH,		// uint (patch number) : hex x 8 (patch bytes) string (patch name)
	// end of command list
	CT_COUNT
};

static const TCHAR* CT[CT_COUNT] =
{
	// comment
	_T("#"),
	// INFO block
	_T("TITLE"),
	_T("AUTHOR"),
	_T("COPYRIGHT"),
	// COMMENTS block
	_T("COMMENT"),
	// PARAMS block
	_T("MACHINE"),
	_T("FRAMERATE"),
	_T("EXPANSION"),
	_T("VIBRATO"),
	_T("SPLIT"),
	// // // 050B
	_T("PLAYBACKRATE"),
	_T("TUNING"),

	_T("N163CHANNELS"),
	// SEQUENCES block
	_T("MACRO"),
	_T("MACROVRC6"),
	_T("MACRON163"),
	_T("MACROS5B"),
	// DPCM SAMPLES block
	_T("DPCMDEF"),
	_T("DPCM"),
	// // // DETUNETABLES block
	_T("DETUNE"),
	// // // GROOVES block
	_T("GROOVE"),
	_T("USEGROOVE"),
	// INSTRUMENTS block
	_T("INST2A03"),
	_T("INSTVRC6"),
	_T("INSTVRC7"),
	_T("INSTFDS"),
	_T("INSTN163"),
	_T("INSTS5B"),

	_T("KEYDPCM"),
	_T("FDSWAVE"),
	_T("FDSMOD"),
	_T("FDSMACRO"),
	_T("N163WAVE"),
	// HEADER block
	_T("TRACK"),
	_T("COLUMNS"),
	// FRAMES block
	_T("ORDER"),
	// PATTERNS block
	_T("PATTERN"),
	_T("ROW"),
	// BOOKMARKS block
	_T("BOOKMARK"),
	// PARAMS_EXTRA block
	_T("LINEARPITCH"),
	// JSON block
	_T("JSON"),
	// PARAMS_EMU block
	_T("USEEXTERNALOPLL"),
	_T("OPLLPATCH"),
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
	stChanNote Cell { };		// // //

	CString sNote = t.ReadToken();
	if      (sNote == _T("...")) { Cell.Note = NONE; }
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
			if (o < 0 || o > ECHO_BUFFER_LENGTH) {
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
	else if (sInst == _T("&&")) { Cell.Instrument = HOLD_INSTRUMENT; }		// // // 050B
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

			TCHAR pC = sEff.GetAt(0);
			if (pC >= TCHAR('a') && pC <= TCHAR('z')) pC += TCHAR('A') - TCHAR('a');

			bool Valid;		// // //
			effect_t Eff = GetEffectFromChar(pC, pDoc->GetChipType(channel), &Valid);
			if (!Valid)
			{
				sResult.Format(_T("Line %d column %d: unrecognized effect '%s'."), t.line, t.GetColumn(), sEff);
				return false;
			}
			Cell.EffNumber[e] = Eff;

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
	s += (stCell.Instrument == MAX_INSTRUMENTS) ? _T(" ..") :
		(stCell.Instrument == HOLD_INSTRUMENT) ? _T(" &&") : tmp;		// // // 050B

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
			tmp.Format(_T(" %c%02X"), EFF_CHAR[stCell.EffNumber[e]], stCell.EffParam[e]);
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
	CDSample *dpcm_sample = nullptr;
	unsigned int track = 0;
	unsigned int pattern = 0;
	int N163count = -1;		// // //
	bool UseGroove[MAX_TRACKS] = {};		// // //
	int BookmarkCount = 0;		// !! !!

	std::string jsonparse;

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
				// Do not redraw pattern editor since we're in the middle of loading document,
				// and the program is in an inconsistent state.
				pDoc->SetMachine(static_cast<machine_t>(i), false);
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
			case CT_PLAYBACKRATE:		// // // 050B
			{
				CHECK(t.ReadInt(i, 0, 2, &sResult));
				int rate = 0;
				CHECK(t.ReadInt(rate, 0, 0xFFFF, &sResult));
				pDoc->SetPlaybackRate(i, rate);
				CHECK(t.ReadEOL(&sResult)); 
			}
				break;
			case CT_TUNING:		// // // 050B
			{
				CHECK(t.ReadInt(i,-12,12,&sResult));
				int cent;
				CHECK(t.ReadInt(cent,-100,100,&sResult));
				pDoc->SetTuning(i, cent);
				CHECK(t.ReadEOL(&sResult));
			}
				break;
			case CT_LINEARPITCH:		// !! !!
			{
				CHECK(t.ReadInt(i, 0, 1, &sResult));
				pDoc->SetLinearPitch(static_cast<bool>(i));
				CHECK(t.ReadEOL(&sResult)); 
			}
				break;
			case CT_N163CHANNELS:
				CHECK(t.ReadInt(i,1,8,&sResult));
				N163count = i;		// // //
				pDoc->SetNamcoChannels(8);
				pDoc->SelectExpansionChip(pDoc->GetExpansionChip());
				CHECK(t.ReadEOL(&sResult));
				break;
			case CT_MACRO:
			case CT_MACROVRC6:
			case CT_MACRON163:
			case CT_MACROS5B:
				{
					static const inst_type_t CHIP_MACRO[4] = { INST_2A03, INST_VRC6, INST_N163, INST_S5B };		// // //
					int chip = c - CT_MACRO;

					int mt, loop, release;
					CHECK(t.ReadInt(mt,0,SEQ_COUNT-1,&sResult));
					CHECK(t.ReadInt(i,0,MAX_SEQUENCES-1,&sResult));
					CSequence* pSeq = pDoc->GetSequence(CHIP_MACRO[chip], i, mt);

					CHECK(t.ReadInt(loop,-1,MAX_SEQUENCE_ITEMS,&sResult));
					CHECK(t.ReadInt(release,-1,MAX_SEQUENCE_ITEMS,&sResult));
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
					pSeq->SetLoopPoint(loop);
					pSeq->SetReleasePoint(release);
				}
				break;
			case CT_DPCMDEF:
				{
					CHECK(t.ReadInt(i,0,MAX_DSAMPLES-1,&sResult));
					dpcm_index = i;
					dpcm_pos = 0;

					CHECK(t.ReadInt(i,0,CDSample::MAX_SIZE,&sResult));
					dpcm_sample = new CDSample();		// // //
					pDoc->SetSample(dpcm_index, dpcm_sample);
					char *blank = new char[i]();
					dpcm_sample->SetData(i, blank);
					dpcm_sample->SetName(Charify(t.ReadToken()));

					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_DPCM:
				{
					CHECK_COLON();
					while (!t.IsEOL())
					{
						CHECK(t.ReadHex(i,0x00,0xFF,&sResult));
						if (dpcm_pos >= dpcm_sample->GetSize())
						{
							sResult.Format(_T("Line %d column %d: DPCM sample %d overflow, increase size used in %s."), t.line, t.GetColumn(), dpcm_index, CT[CT_DPCMDEF]);
							return sResult;
						}
						*(dpcm_sample->GetData() + dpcm_pos) = (char)(i);
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
					SAFE_RELEASE(Groove);
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_USEGROOVE:		// // //
				{
					CHECK_COLON();
					int oldTrack = track;
					while (!t.IsEOL()) {
						CHECK(t.ReadInt(i,1,MAX_TRACKS,&sResult));
						UseGroove[--i] = true;
						if (static_cast<unsigned int>(i) < pDoc->GetTrackCount()) {
							pDoc->SetSongGroove(i, true);
						}
					}
				}
				break;
			case CT_INST2A03:		// // //
			case CT_INSTVRC6:
			case CT_INSTN163:
			case CT_INSTS5B:
				{
					inst_type_t Type = INST_NONE;
					switch (c) {
					case CT_INST2A03: Type = INST_2A03; break;
					case CT_INSTVRC6: Type = INST_VRC6; break;
					case CT_INSTN163: Type = INST_N163; break;
					case CT_INSTS5B:  Type = INST_S5B; break;
					}
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					auto seqInst = dynamic_cast<CSeqInstrument*>(CInstrumentFactory::CreateNew(Type));		// // //
					pDoc->AddInstrument(seqInst, i);
					for (int s=0; s < SEQ_COUNT; ++s)
					{
						CHECK(t.ReadInt(i,-1,MAX_SEQUENCES-1,&sResult));
						seqInst->SetSeqEnable(s, (i == -1) ? 0 : 1);
						seqInst->SetSeqIndex(s, (i == -1) ? 0 : i);
					}
					if (c == CT_INSTN163) {
						auto pInst = static_cast<CInstrumentN163*>(seqInst);
						CHECK(t.ReadInt(i,0,256-16*N163count,&sResult));		// // //
						pInst->SetWaveSize(i);
						CHECK(t.ReadInt(i,0,256-16*N163count-1,&sResult));		// // //
						pInst->SetWavePos(i);
						CHECK(t.ReadInt(i,0,CInstrumentN163::MAX_WAVE_COUNT,&sResult));
						pInst->SetWaveCount(i);
					}
					seqInst->SetName(Charify(t.ReadToken()));
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_INSTVRC7:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					CInstrumentVRC7 *pInst = new CInstrumentVRC7();
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
					CInstrumentFDS *pInst = new CInstrumentFDS();
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
			case CT_KEYDPCM:
				{
					CHECK(t.ReadInt(i,0,MAX_INSTRUMENTS-1,&sResult));
					if (pDoc->GetInstrumentType(i) != INST_2A03)
					{
						sResult.Format(_T("Line %d column %d: instrument %d is not defined as a 2A03 instrument."), t.line, t.GetColumn(), i);
						return sResult;
					}
					auto pInst = std::static_pointer_cast<CInstrument2A03>(pDoc->GetInstrument(i));

					int io, in;
					CHECK(t.ReadInt(io,0,OCTAVE_RANGE,&sResult));
					CHECK(t.ReadInt(in,0,12,&sResult));

					CHECK(t.ReadInt(i,0,MAX_DSAMPLES-1,&sResult));
					pInst->SetSampleIndex(io, in, i+1);
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
					auto pInst = std::static_pointer_cast<CInstrumentFDS>(pDoc->GetInstrument(i));
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
					auto pInst = std::static_pointer_cast<CInstrumentFDS>(pDoc->GetInstrument(i));
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
					auto pInst = std::static_pointer_cast<CInstrumentFDS>(pDoc->GetInstrument(i));
					int loop, release;
					CHECK(t.ReadInt(i,0,CInstrumentFDS::SEQUENCE_COUNT-1,&sResult));
					CSequence *pSeq = new CSequence();		// // //
					pInst->SetSequence(i, pSeq);
					CHECK(t.ReadInt(loop,-1,MAX_SEQUENCE_ITEMS,&sResult));
					CHECK(t.ReadInt(release,-1,MAX_SEQUENCE_ITEMS,&sResult));
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
					pSeq->SetLoopPoint(loop);
					pSeq->SetReleasePoint(release);
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
					auto pInst = std::static_pointer_cast<CInstrumentN163>(pDoc->GetInstrument(i));

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
			case CT_BOOKMARK:
				{
					if (track == 0)
					{
						sResult.Format(_T("Line %d column %d: no TRACK defined, cannot add bookmark data."), t.line, t.GetColumn());
						return sResult;
					}
					CBookmark* pMark = new CBookmark();
					CHECK(t.ReadHex(i, 0, (int)pDoc->GetFrameCount(track - 1) - 1, &sResult));
					pMark->m_iFrame = i;
					CHECK(t.ReadHex(i, 0, (int)pDoc->GetPatternLength(track - 1) - 1, &sResult));
					pMark->m_iRow = i;
					CHECK(t.ReadInt(i, 0, MAX_PATTERN_LENGTH, &sResult));
					pMark->m_Highlight.First = i;
					CHECK(t.ReadInt(i, 0, MAX_PATTERN_LENGTH, &sResult));
					pMark->m_Highlight.Second = i;
					CHECK(t.ReadInt(i, 0, 1, &sResult));
					pMark->m_bPersist = static_cast<bool>(i);
					pMark->m_sName = std::string(t.ReadToken());
					if (!(pDoc->GetBookmarkManager()->GetCollection(track - 1)->AddBookmark(pMark))) {
						sResult.Format(_T("Line %d column %d: Failed to add bookmark."), t.line, t.GetColumn());
						return sResult;
					}
					CHECK(t.ReadEOL(&sResult));
				}
				break;
			case CT_JSON:
			{
				jsonparse += t.ReadToken();
				CHECK(t.ReadEOL(&sResult));
			}
				break;
			case CT_USEEXTOPLL:
			{
				CHECK(t.ReadInt(i, 0, 1, &sResult));
				pDoc->SetExternalOPLLChipCheck(static_cast<bool>(i));
				CHECK(t.ReadEOL(&sResult));
			}
				break;
			case CT_OPLLPATCH:
			{
				int patchnum = 0;
				int patchbyte[8]{};
				CHECK(t.ReadInt(patchnum, 0, 18, &sResult));
				CHECK_COLON();
				for (int index = 0; index < 8; index++) {
					CHECK(t.ReadHex(patchbyte[index], 0x00, 0xFF, &sResult));
					pDoc->SetOPLLPatchByte(((8 * patchnum) + index), patchbyte[index]);
				}
				pDoc->SetOPLLPatchName(patchnum, std::string(t.ReadToken()));
				CHECK(t.ReadEOL(&sResult));
			}
			break;
			case CT_COUNT:
			default:
				sResult.Format(_T("Unrecognized command at line %d: '%s'."), t.line, command);
				return sResult;
		}
	}

	if (jsonparse.size()) {
		try {
			json j = json::parse(jsonparse);
			pDoc->OptionalJSONToInterface(j);
		}
		catch (json::parse_error& e) {
			sResult.Format(_T("JSON parsing error:\n%s\n\nException ID: %d\nByte position of error: %d\n\nOptional JSON data will be ignored."), e.what(), e.id, e.byte);
		}
	}

	if (N163count != -1) {		// // //
		pDoc->SetNamcoChannels(N163count, true);
		pDoc->SelectExpansionChip(pDoc->GetExpansionChip()); // calls ApplyExpansionChip()
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
					if (stCell.Note != NONE || stCell.Instrument != MAX_INSTRUMENTS || stCell.Vol != MAX_VOLUME) isEmpty = false;
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

	s.Format(_T("# " APP_NAME " text export %i.%i.%i.%i\n"), VERSION);		// // //
	f.WriteString(s);
	s.Format(_T("# Module version %04X\n\n"), CDocumentFile::FILE_VER);		// // //
	f.WriteString(s);

	f.WriteString(_T("# INFO block\n"));

	s.Format(_T("%-15s %s\n"
				"%-15s %s\n"
				"%-15s %s\n"
				"\n"),
				CT[CT_TITLE],     ExportString(pDoc->GetSongName()),
				CT[CT_AUTHOR],    ExportString(pDoc->GetSongArtist()),
				CT[CT_COPYRIGHT], ExportString(pDoc->GetSongCopyright()));
	f.WriteString(s);

	f.WriteString(_T("# COMMENTS block\n"));
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

	f.WriteString(_T("# PARAMS block\n"));
	s.Format(_T("%-15s %d\n"
				"%-15s %d\n"
				"%-15s %d\n"
				"%-15s %d\n"
				"%-15s %d\n"
				"%-15s %d %d\n"		// // // 050B
				),
				CT[CT_MACHINE],   pDoc->GetMachine(),
				CT[CT_FRAMERATE], pDoc->GetEngineSpeed(),
				CT[CT_EXPANSION], pDoc->GetExpansionChip(),
				CT[CT_VIBRATO],   pDoc->GetVibratoStyle(),
				CT[CT_SPLIT],     pDoc->GetSpeedSplitPoint(),
				CT[CT_PLAYBACKRATE], pDoc->GetPlaybackRateType(), pDoc->GetPlaybackRate()
				);
	if (pDoc->GetTuningSemitone() || pDoc->GetTuningCent())		// // // 050B
		s.AppendFormat(_T("%-15s %d %d\n"), CT[CT_TUNING], pDoc->GetTuningSemitone(), pDoc->GetTuningCent());
	if (pDoc->GetLinearPitch())
		s.AppendFormat(_T("%-15s %d\n"), CT[CT_LINEARPITCH], static_cast<int>(pDoc->GetLinearPitch()));
	f.WriteString(s);

	f.WriteString(_T("\n"));

	int N163count = -1;		// // //
	if (pDoc->ExpansionEnabled(SNDCHIP_N163))
	{
		N163count = pDoc->GetNamcoChannels();
		pDoc->SetNamcoChannels(8, true);
		pDoc->SelectExpansionChip(pDoc->GetExpansionChip()); // calls ApplyExpansionChip()
		s.Format(_T("%-15s %d\n"
					"\n"),
					CT[CT_N163CHANNELS], N163count);
		f.WriteString(s);
	}

	f.WriteString(_T("# SEQUENCES block\n"));
	for (int c=0; c<4; ++c)
	{
		const inst_type_t CHIP_MACRO[4] = { INST_2A03, INST_VRC6, INST_N163, INST_S5B };

		for (int st=0; st < SEQ_COUNT; ++st)
		for (int seq=0; seq < MAX_SEQUENCES; ++seq)
		{
			CSequence* pSequence = pDoc->GetSequence(CHIP_MACRO[c], seq, st);
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

	f.WriteString(_T("# DPCM SAMPLES block\n"));
	for (int smp=0; smp < MAX_DSAMPLES; ++smp)
	{
		if (const CDSample* pSample = pDoc->GetSample(smp))		// // //
		{
			const unsigned int size = pSample->GetSize();
			s.Format(_T("%s %3d %5d %s\n"),
				CT[CT_DPCMDEF],
				smp,
				size,
				ExportString(pSample->GetName()));
			f.WriteString(s);

			for (unsigned int i=0; i < size; i += 32)
			{
				s.Format(_T("%s :"), CT[CT_DPCM]);
				f.WriteString(s);
				for (unsigned int j=0; j<32 && (i+j)<size; ++j)
				{
					s.Format(_T(" %02X"), (unsigned char)(*(pSample->GetData() + (i+j))));
					f.WriteString(s);
				}
				f.WriteString(_T("\n"));
			}
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# DETUNETABLES block\n"));		// // //
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++) {
		int Offset = pDoc->GetDetuneOffset(i, j);
		if (Offset != 0) {
			s.Format(_T("%s %3d %3d %3d %5d\n"), CT[CT_DETUNE], i, j / NOTE_RANGE, j % NOTE_RANGE, Offset);
			f.WriteString(s);
		}
	}
	f.WriteString(_T("\n"));
	
	f.WriteString(_T("# GROOVES block\n"));		// // //
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
	
	f.WriteString(_T("# Tracks using default groove:\n"));		// // //
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
		f.WriteString(_T("\n"));
	}
	f.WriteString(_T("\n"));
	
	f.WriteString(_T("# INSTRUMENTS block\n"));
	for (unsigned int i=0; i<MAX_INSTRUMENTS; ++i)
	{
		auto pInst = pDoc->GetInstrument(i);
		if (!pInst) continue;

		const TCHAR *CTstr = nullptr;		// // //
		switch (pInst->GetType()) {
		case INST_2A03:	CTstr = CT[CT_INST2A03]; break;
		case INST_VRC6:	CTstr = CT[CT_INSTVRC6]; break;
		case INST_VRC7:	CTstr = CT[CT_INSTVRC7]; break;
		case INST_FDS:	CTstr = CT[CT_INSTFDS];  break;
		case INST_N163:	CTstr = CT[CT_INSTN163]; break;
		case INST_S5B:	CTstr = CT[CT_INSTS5B];  break;
		case INST_NONE: default:
			pDoc->GetInstrument(i).reset(); continue;
		}
		s.Format(_T("%-8s %3d   "), CTstr, i);
		f.WriteString(s);

		auto seqInst = std::dynamic_pointer_cast<CSeqInstrument>(pInst);
		if (seqInst && pInst->GetType() != INST_FDS) {
			s.Empty();
			for (int j = 0; j < SEQ_COUNT; j++)
				s.AppendFormat(_T("%3d "), seqInst->GetSeqEnable(j) ? seqInst->GetSeqIndex(j) : -1);
			f.WriteString(s);
		}

		switch (pInst->GetType())
		{
		case INST_N163:
			{
				auto pDI = std::static_pointer_cast<CInstrumentN163>(pInst);
				s.Format(_T("%3d %3d %3d "),
					pDI->GetWaveSize(),
					pDI->GetWavePos(),
					pDI->GetWaveCount());
				f.WriteString(s);
			}
			break;
		case INST_VRC7:
			{
				auto pDI = std::static_pointer_cast<CInstrumentVRC7>(pInst);
				s.Format(_T("%3d "), pDI->GetPatch());
				for (int j = 0; j < 8; j++)
					s.AppendFormat(_T("%02X "), pDI->GetCustomReg(j));
				f.WriteString(s);
			}
			break;
		case INST_FDS:
			{
				auto pDI = std::static_pointer_cast<CInstrumentFDS>(pInst);
				s.Format(_T("%3d %3d %3d %3d "),
					pDI->GetModulationEnable(),
					pDI->GetModulationSpeed(),
					pDI->GetModulationDepth(),
					pDI->GetModulationDelay());
				f.WriteString(s);
			}
			break;
		}

		f.WriteString(ExportString(pInst->GetName()));
		f.WriteString(_T("\n"));

		switch (pInst->GetType())
		{
		case INST_2A03:
			{
				auto pDI = std::static_pointer_cast<CInstrument2A03>(pInst);
				for (int oct = 0; oct < OCTAVE_RANGE; ++oct)
				for (int key = 0; key < NOTE_RANGE; ++key)
				{
					int smp = pDI->GetSampleIndex(oct, key);
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
		case INST_N163:
			{
				auto pDI = std::static_pointer_cast<CInstrumentN163>(pInst);
				for (int w=0; w < pDI->GetWaveCount(); ++w)
				{
					s.Format(_T("%s %3d %3d :"),
						CT[CT_N163WAVE], i, w);
					f.WriteString(s);

					for (int smp=0; smp < pDI->GetWaveSize(); ++smp)
					{
						s.Format(_T(" %2d"), pDI->GetSample(w, smp));
						f.WriteString(s);
					}
					f.WriteString(_T("\n"));
				}
			}
			break;
		case INST_FDS:
			{
				auto pDI = std::static_pointer_cast<CInstrumentFDS>(pInst);
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

				for (int seq=0; seq < 3; ++seq)
				{
					const CSequence* pSequence = pDI->GetSequence(seq);		// // //
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
		}
	}
	f.WriteString(_T("\n"));

	for (unsigned int t=0; t < pDoc->GetTrackCount(); ++t)
	{
		const char* zpTitle = pDoc->GetTrackTitle(t).GetString();
		if (zpTitle == NULL) zpTitle = "";

		f.WriteString(_T("# track HEADER block\n"));

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

		f.WriteString(_T("# track FRAMES block\n"));

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

		f.WriteString(_T("# track PATTERNS block\n"));

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
		}
		f.WriteString(_T("\n"));

		f.WriteString(_T("# track BOOKMARKS block\n"));

		CBookmarkCollection *pBookmarkCollection = pDoc->GetBookmarkManager()->GetCollection(t);

		unsigned int bookmarkcount = pBookmarkCollection->GetCount();
		if (bookmarkcount) for (unsigned int b = 0; b < bookmarkcount; ++b) {
			CBookmark* pMark = pBookmarkCollection->GetBookmark(b);
			s.Format(_T("%s %02X %02X %3d %3d %3d %s"),
				CT[CT_BOOKMARK],
				pMark->m_iFrame,
				pMark->m_iRow,
				pMark->m_Highlight.First,
				pMark->m_Highlight.Second,
				pMark->m_bPersist,
				ExportString(pMark->m_sName.c_str()));
			f.WriteString(s);
			f.WriteString(_T("\n"));
		}
		f.WriteString(_T("\n"));
	}

	f.WriteString(_T("# JSON block\n"));
	{
		json j = pDoc->InterfaceToOptionalJSON();
		if (!j.is_null()) {
			std::string& jsondump = j.dump(4, ' ', true);
			std::string& delimiter = std::string("\n");
			std::string::size_type pos = 0, prev = 0;
			while ((pos = jsondump.find(delimiter, prev)) != std::string::npos) {
				s.Format(_T("%s %s\n"), CT[CT_JSON], ExportString(jsondump.substr(prev, pos - prev).c_str()));
				f.WriteString(s);
				prev = pos + delimiter.size();
			}
			s.Format(_T("%s %s\n"), CT[CT_JSON], ExportString(jsondump.substr(prev).c_str()));
			f.WriteString(s);
		}
	}
	f.WriteString(_T("\n"));

	f.WriteString(_T("# PARAMS_EMU block\n"));
	if (pDoc->GetExternalOPLLChipCheck() && (pDoc->GetExpansionChip() & SNDCHIP_VRC7)) {
		s.Format(_T("%s %d\n"), CT[CT_USEEXTOPLL], static_cast<int>(pDoc->GetExternalOPLLChipCheck()));
		f.WriteString(s);

		for (int patch = 0; patch < 19; patch++) {
			s.Format(_T("%s %2d :"), CT[CT_OPLLPATCH], patch);
			f.WriteString(s);
			for (int patchbyte = 0; patchbyte < 8; patchbyte++) {
				s.Format(_T(" %02X"), pDoc->GetOPLLPatchByte((8 * patch) + patchbyte));
				f.WriteString(s);
			}
			s.Format(_T(" %s"), ExportString(pDoc->GetOPLLPatchName(patch).c_str()));
			f.WriteString(s);
			f.WriteString(_T("\n"));
		}
	}
	f.WriteString(_T("\n"));
	
	if (N163count != -1) {		// // //
		pDoc->SetNamcoChannels(N163count, true);
		pDoc->SelectExpansionChip(pDoc->GetExpansionChip()); // calls ApplyExpansionChip()

		// Do not set modules modified when exporting text
		pDoc->SetModifiedFlag(FALSE);
		pDoc->SetExceededFlag(FALSE);
	}

	f.WriteString(_T("# End of export\n"));
	pDoc->UpdateAllViews(NULL, UPDATE_FRAME);
	pDoc->UpdateAllViews(NULL, UPDATE_PATTERN);
	return sResult;
}

// end of file
