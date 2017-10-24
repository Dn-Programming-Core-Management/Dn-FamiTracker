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

#include "FamiTrackerDocIO.h"
#include "DocumentFile.h" // stdafx.h
#include "ModuleException.h"
#include "FamiTrackerDoc.h"
#include "Settings.h"

#include "InstrumentManager.h"
#include "InstrumentVRC6.h" // error message
#include "InstrumentN163.h" // error message
#include "InstrumentS5B.h" // error message

#include "SequenceManager.h"
#include "SequenceCollection.h"

#include "SongData.h"
#include "PatternNote.h"

#include "DSampleManager.h"
#include "DSample.h"

#include "Groove.h"

#include "BookmarkManager.h"
#include "BookmarkCollection.h"

namespace {

const char *FILE_BLOCK_PARAMS			= "PARAMS";
const char *FILE_BLOCK_INFO				= "INFO";
const char *FILE_BLOCK_INSTRUMENTS		= "INSTRUMENTS";
const char *FILE_BLOCK_SEQUENCES		= "SEQUENCES";
const char *FILE_BLOCK_FRAMES			= "FRAMES";
const char *FILE_BLOCK_PATTERNS			= "PATTERNS";
const char *FILE_BLOCK_DSAMPLES			= "DPCM SAMPLES";
const char *FILE_BLOCK_HEADER			= "HEADER";
const char *FILE_BLOCK_COMMENTS			= "COMMENTS";

// VRC6
const char *FILE_BLOCK_SEQUENCES_VRC6	= "SEQUENCES_VRC6";

// N163
const char *FILE_BLOCK_SEQUENCES_N163	= "SEQUENCES_N163";
const char *FILE_BLOCK_SEQUENCES_N106	= "SEQUENCES_N106";

// Sunsoft
const char *FILE_BLOCK_SEQUENCES_S5B	= "SEQUENCES_S5B";

// // // 0CC-FamiTracker specific
const char *FILE_BLOCK_DETUNETABLES		= "DETUNETABLES";
const char *FILE_BLOCK_GROOVES			= "GROOVES";
const char *FILE_BLOCK_BOOKMARKS		= "BOOKMARKS";
const char *FILE_BLOCK_PARAMS_EXTRA		= "PARAMS_EXTRA";

// // // helper function for effect conversion
typedef std::array<effect_t, EF_COUNT> EffTable;
std::pair<EffTable, EffTable> MakeEffectConversion(std::initializer_list<std::pair<effect_t, effect_t>> List)
{
	EffTable forward, backward;
	for (int i = 0; i < EF_COUNT; ++i)
		forward[i] = backward[i] = static_cast<effect_t>(i);
	for (const auto &p : List) {
		forward[p.first] = p.second;
		backward[p.second] = p.first;
	}
	return std::make_pair(forward, backward);
}

static const auto EFF_CONVERSION_050 = MakeEffectConversion({
//	{EF_SUNSOFT_ENV_LO,		EF_SUNSOFT_ENV_TYPE},
//	{EF_SUNSOFT_ENV_TYPE,	EF_SUNSOFT_ENV_LO},
	{EF_SUNSOFT_NOISE,		EF_NOTE_RELEASE},
	{EF_VRC7_PORT,			EF_GROOVE},
	{EF_VRC7_WRITE,			EF_TRANSPOSE},
	{EF_NOTE_RELEASE,		EF_N163_WAVE_BUFFER},
	{EF_GROOVE,				EF_FDS_VOLUME},
	{EF_TRANSPOSE,			EF_FDS_MOD_BIAS},
	{EF_N163_WAVE_BUFFER,	EF_SUNSOFT_NOISE},
	{EF_FDS_VOLUME,			EF_VRC7_PORT},
	{EF_FDS_MOD_BIAS,		EF_VRC7_WRITE},
});

template <typename F> // (const CSequence &seq, int index, int seqType)
void VisitSequences(const CSequenceManager *manager, F&& f) {
	if (!manager)
		return;
	for (int j = 0, n = manager->GetCount(); j < n; ++j) {
		const auto &col = *manager->GetCollection(j);
		for (int i = 0; i < MAX_SEQUENCES; ++i) {
			if (const CSequence *pSeq = col.GetSequence(i); pSeq && pSeq->GetItemCount())
				f(*pSeq, i, j);
		}
	}
}

// void (*F)(CPatternData &pattern, unsigned song, unsigned channel, unsigned index)
template <typename F>
void VisitPatterns(CFamiTrackerDoc &doc, F f) {
	doc.VisitSongs([&] (CSongData &song, unsigned index) {
		for (unsigned ch = 0, n = doc.GetChannelCount(); ch < n; ++ch)
			for (unsigned p = 0; p < MAX_PATTERN; ++p)
				f(song.GetPattern(ch, p), index, ch, p);
	});
}
// void (*F)(const CPatternData &pattern, unsigned song, unsigned channel, unsigned index)
template <typename F>
void VisitPatterns(const CFamiTrackerDoc &doc, F f) {
	doc.VisitSongs([&] (const CSongData &song, unsigned index) {
		for (unsigned ch = 0, n = doc.GetChannelCount(); ch < n; ++ch)
			for (unsigned p = 0; p < MAX_PATTERN; ++p)
				f(song.GetPattern(ch, p), index, ch, p);
	});
}

} // namespace

// // // save/load functionality

CFamiTrackerDocIO::CFamiTrackerDocIO(CDocumentFile &file) :
	file_(file)
{
}

void CFamiTrackerDocIO::Load(CFamiTrackerDoc &doc) {
}

bool CFamiTrackerDocIO::Save(const CFamiTrackerDoc &doc) {
	using block_info_t = std::tuple<void (CFamiTrackerDocIO::*)(const CFamiTrackerDoc &, int), int, const char *>;
	static const block_info_t MODULE_WRITE_FUNC[] = {		// // //
		{&CFamiTrackerDocIO::SaveParams,		6, FILE_BLOCK_PARAMS},
		{&CFamiTrackerDocIO::SaveSongInfo,		1, FILE_BLOCK_INFO},
		{&CFamiTrackerDocIO::SaveHeader,		3, FILE_BLOCK_HEADER},
		{&CFamiTrackerDocIO::SaveInstruments,	6, FILE_BLOCK_INSTRUMENTS},
		{&CFamiTrackerDocIO::SaveSequences,		6, FILE_BLOCK_SEQUENCES},
		{&CFamiTrackerDocIO::SaveFrames,		3, FILE_BLOCK_FRAMES},
#ifdef TRANSPOSE_FDS
		{&CFamiTrackerDocIO::SavePatterns,		5, FILE_BLOCK_PATTERNS},
#else
		{&CFamiTrackerDocIO::SavePatterns,		4, FILE_BLOCK_PATTERNS},
#endif
		{&CFamiTrackerDocIO::SaveDSamples,		1, FILE_BLOCK_DSAMPLES},
		{&CFamiTrackerDocIO::SaveComments,		1, FILE_BLOCK_COMMENTS},
		{&CFamiTrackerDocIO::SaveSequencesVRC6,	6, FILE_BLOCK_SEQUENCES_VRC6},		// // //
		{&CFamiTrackerDocIO::SaveSequencesN163,	1, FILE_BLOCK_SEQUENCES_N163},
		{&CFamiTrackerDocIO::SaveSequencesS5B,	1, FILE_BLOCK_SEQUENCES_S5B},
		{&CFamiTrackerDocIO::SaveParamsExtra,	2, FILE_BLOCK_PARAMS_EXTRA},		// // //
		{&CFamiTrackerDocIO::SaveDetuneTables,	1, FILE_BLOCK_DETUNETABLES},		// // //
		{&CFamiTrackerDocIO::SaveGrooves,		1, FILE_BLOCK_GROOVES},				// // //
		{&CFamiTrackerDocIO::SaveBookmarks,		1, FILE_BLOCK_BOOKMARKS},			// // //
	};

	file_.BeginDocument();
	for (auto [fn, ver, name] : MODULE_WRITE_FUNC) {
		file_.CreateBlock(name, ver);
		(this->*fn)(doc, ver);
		if (!file_.FlushBlock())
			return false;
	}
	file_.EndDocument();
	return true;
}

void CFamiTrackerDocIO::SaveParams(const CFamiTrackerDoc &doc, int ver) {
	if (ver >= 2)
		file_.WriteBlockChar(doc.GetExpansionChip());		// ver 2 change
	else
		file_.WriteBlockInt(doc.GetSongData(0).GetSongSpeed());

	file_.WriteBlockInt(doc.GetChannelCount());
	file_.WriteBlockInt(doc.GetMachine());
	file_.WriteBlockInt(doc.GetEngineSpeed());
	
	if (ver >= 3) {
		file_.WriteBlockInt(doc.GetVibratoStyle());

		if (ver >= 4) {
			auto hl = doc.GetHighlight();
			file_.WriteBlockInt(hl.First);
			file_.WriteBlockInt(hl.Second);

			if (ver >= 5) {
				if (doc.ExpansionEnabled(SNDCHIP_N163))
					file_.WriteBlockInt(doc.GetNamcoChannels());

				if (ver >= 6)
					file_.WriteBlockInt(doc.GetSpeedSplitPoint());

				if (ver >= 8) {		// // // 050B
					file_.WriteBlockChar(doc.GetTuningSemitone());
					file_.WriteBlockChar(doc.GetTuningCent());
				}
			}
		}
	}
}

void CFamiTrackerDocIO::LoadSongInfo(CFamiTrackerDoc &doc, int ver) {
	char buf[32];
	file_.GetBlock(buf, std::size(buf));
	doc.SetSongName(buf);
	file_.GetBlock(buf, std::size(buf));
	doc.SetSongArtist(buf);
	file_.GetBlock(buf, std::size(buf));
	doc.SetSongCopyright(buf);
}

void CFamiTrackerDocIO::SaveSongInfo(const CFamiTrackerDoc &doc, int ver) {
	file_.WriteBlock(doc.GetSongName(), 32);
	file_.WriteBlock(doc.GetSongArtist(), 32);
	file_.WriteBlock(doc.GetSongCopyright(), 32);
}

void CFamiTrackerDocIO::LoadHeader(CFamiTrackerDoc &doc, int ver) {
	if (ver == 1) {
		// Single track
		auto &Song = doc.GetSongData(0);
		for (int i = 0; i < doc.GetChannelCount(); ++i) try {
			// Channel type (unused)
			AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockChar(), 0, CHANNELS - 1, "Channel type index");
			// Effect columns
			Song.SetEffectColumnCount(i, AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockChar(), 0, MAX_EFFECT_COLUMNS - 1, "Effect column count"));
		}
		catch (CModuleException *e) {
			e->AppendError("At channel %d", i + 1);
			throw;
		}
	}
	else if (ver >= 2) {
		// Multiple tracks
		unsigned Tracks = AssertRange(file_.GetBlockChar() + 1, 1, static_cast<int>(MAX_TRACKS), "Track count");	// 0 means one track
		doc.GetSongData(Tracks - 1); // allocate

		// Track names
		if (ver >= 3)
			doc.VisitSongs([&] (CSongData &song) { song.SetTitle((LPCTSTR)file_.ReadString()); });

		for (int i = 0; i < doc.GetChannelCount(); ++i) try {
			AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockChar(), 0, CHANNELS - 1, "Channel type index"); // Channel type (unused)
			doc.VisitSongs([&] (CSongData &song, unsigned index) {
				try {
					song.SetEffectColumnCount(i, AssertRange<MODULE_ERROR_STRICT>(
						file_.GetBlockChar(), 0, MAX_EFFECT_COLUMNS - 1, "Effect column count"));
				}
				catch (CModuleException *e) {
					e->AppendError("At song %d,", index + 1);
					throw;
				}
			});
		}
		catch (CModuleException *e) {
			e->AppendError("At channel %d,", i + 1);
			throw;
		}

		if (ver >= 4)		// // // 050B
			for (unsigned int i = 0; i < Tracks; ++i) {
				int First = static_cast<unsigned char>(file_.GetBlockChar());
				int Second = static_cast<unsigned char>(file_.GetBlockChar());
				if (!i)
					doc.SetHighlight({First, Second});
			}
		doc.VisitSongs([&] (CSongData &song) { song.SetHighlight(doc.GetHighlight()); });		// // //
	}
}

void CFamiTrackerDocIO::SaveHeader(const CFamiTrackerDoc &doc, int ver) {
	// Write number of tracks
	if (ver >= 2)
		file_.WriteBlockChar(doc.GetTrackCount() - 1);

	// Ver 3, store track names
	if (ver >= 3)
		doc.VisitSongs([&] (const CSongData &song) { file_.WriteString(song.GetTitle().c_str()); });

	for (int i = 0; i < doc.GetChannelCount(); ++i) {
		// Channel type
		file_.WriteBlockChar(doc.GetChannelType(i));		// // //
		for (unsigned int j = 0; j < doc.GetTrackCount(); ++j) {
			// Effect columns
			file_.WriteBlockChar(doc.GetSongData(j).GetEffectColumnCount(i));
			if (ver <= 1)
				break;
		}
	}
}

void CFamiTrackerDocIO::LoadInstruments(CFamiTrackerDoc &doc, int ver) {
	/*
	 * Version changes
	 *
	 *  2 - Extended DPCM octave range
	 *  3 - Added settings to the arpeggio sequence
	 *
	 */

	// Number of instruments
	const int Count = AssertRange(file_.GetBlockInt(), 0, CInstrumentManager::MAX_INSTRUMENTS, "Instrument count");
	auto &Manager = *doc.GetInstrumentManager();

	for (int i = 0; i < Count; ++i) {
		// Instrument index
		int index = AssertRange(file_.GetBlockInt(), 0, CInstrumentManager::MAX_INSTRUMENTS - 1, "Instrument index");

		// Read instrument type and create an instrument
		inst_type_t Type = (inst_type_t)file_.GetBlockChar();
		auto pInstrument = CInstrumentManager::CreateNew(Type);		// // //

		try {
			// Load the instrument
			AssertFileData(pInstrument.get() != nullptr, "Failed to create instrument");
			pInstrument->Load(&file_);
			// Read name
			int size = AssertRange(file_.GetBlockInt(), 0, CInstrument::INST_NAME_MAX, "Instrument name length");
			char Name[CInstrument::INST_NAME_MAX + 1];
			file_.GetBlock(Name, size);
			Name[size] = 0;
			pInstrument->SetName(Name);
			Manager.InsertInstrument(index, std::move(pInstrument));		// // // this registers the instrument content provider
		}
		catch (CModuleException *e) {
			file_.SetDefaultFooter(e);
			e->AppendError("At instrument %02X,", index);
			Manager.RemoveInstrument(index);
			throw;
		}
	}
}

void CFamiTrackerDocIO::SaveInstruments(const CFamiTrackerDoc &doc, int ver) {
	// A bug in v0.3.0 causes a crash if this is not 2, so change only when that ver is obsolete!
	//
	// Log:
	// - v6: adds DPCM delta settings
	//

	// If FDS is used then version must be at least 4 or recent files won't load

	// Fix for FDS instruments
/*	if (m_iExpansionChip & SNDCHIP_FDS)
		ver = 4;

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pInstrumentManager->GetInstrument(i)->GetType() == INST_FDS)
			ver = 4;
	}
*/
	char Name[CInstrument::INST_NAME_MAX];

	const auto &Manager = *doc.GetInstrumentManager();

	// Instruments block
	const int Count = Manager.GetInstrumentCount();
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		// Only write instrument if it's used
		if (auto pInst = Manager.GetInstrument(i)) {
			// Write index and type
			file_.WriteBlockInt(i);
			file_.WriteBlockChar(static_cast<char>(pInst->GetType()));

			// Store the instrument
			pInst->Store(&file_);

			// Store the name
			pInst->GetName(Name);
			file_.WriteBlockInt((int)strlen(Name));
			file_.WriteBlock(Name, (int)strlen(Name));
		}
	}
}

void CFamiTrackerDocIO::SaveSequences(const CFamiTrackerDoc &doc, int ver) {
	int Count = doc.GetTotalSequenceCount(INST_2A03);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(doc.GetSequenceManager(INST_2A03), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(seqType);
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});

	// v6
	VisitSequences(doc.GetSequenceManager(INST_2A03), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
	});
}

void CFamiTrackerDocIO::LoadFrames(CFamiTrackerDoc &doc, int ver) {
	if (ver == 1) {
		unsigned int FrameCount = AssertRange(file_.GetBlockInt(), 1, MAX_FRAMES, "Track frame count");
		/*m_iChannelsAvailable =*/ AssertRange(file_.GetBlockInt(), 0, MAX_CHANNELS, "Channel count");
		auto &Song = doc.GetSongData(0);
		Song.SetFrameCount(FrameCount);
		for (unsigned i = 0; i < FrameCount; ++i) {
			for (int j = 0; j < doc.GetChannelCount(); ++j) {
				unsigned Pattern = static_cast<unsigned char>(file_.GetBlockChar());
				AssertRange(Pattern, 0U, static_cast<unsigned>(MAX_PATTERN - 1), "Pattern index");
				Song.SetFramePattern(i, j, Pattern);
			}
		}
	}
	else if (ver > 1) {
		doc.VisitSongs([&] (CSongData &song) {
			unsigned int FrameCount = AssertRange(file_.GetBlockInt(), 1, MAX_FRAMES, "Track frame count");
			unsigned int Speed = AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockInt(), 0, MAX_TEMPO, "Track default speed");
			song.SetFrameCount(FrameCount);

			if (ver >= 3) {
				unsigned int Tempo = AssertRange<MODULE_ERROR_STRICT>(file_.GetBlockInt(), 0, MAX_TEMPO, "Track default tempo");
				song.SetSongTempo(Tempo);
				song.SetSongSpeed(Speed);
			}
			else {
				if (Speed < 20) {
					song.SetSongTempo(doc.GetMachine() == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
					song.SetSongSpeed(Speed);
				}
				else {
					song.SetSongTempo(Speed);
					song.SetSongSpeed(DEFAULT_SPEED);
				}
			}

			unsigned PatternLength = AssertRange(file_.GetBlockInt(), 1, MAX_PATTERN_LENGTH, "Track default row count");
			song.SetPatternLength(PatternLength);
			
			for (unsigned i = 0; i < FrameCount; ++i) {
				for (int j = 0; j < doc.GetChannelCount(); ++j) {
					// Read pattern index
					int Pattern = static_cast<unsigned char>(file_.GetBlockChar());
					song.SetFramePattern(i, j, AssertRange(Pattern, 0, MAX_PATTERN - 1, "Pattern index"));
				}
			}
		});
	}
}

void CFamiTrackerDocIO::SaveFrames(const CFamiTrackerDoc &doc, int ver) {
	doc.VisitSongs([&] (const CSongData &Song) {
		file_.WriteBlockInt(Song.GetFrameCount());
		file_.WriteBlockInt(Song.GetSongSpeed());
		file_.WriteBlockInt(Song.GetSongTempo());
		file_.WriteBlockInt(Song.GetPatternLength());

		for (unsigned int j = 0; j < Song.GetFrameCount(); ++j)
			for (int k = 0; k < doc.GetChannelCount(); ++k)
				file_.WriteBlockChar((unsigned char)Song.GetFramePattern(j, k));
	});
}

void CFamiTrackerDocIO::SavePatterns(const CFamiTrackerDoc &doc, int ver) {
	/*
	 * Version changes: 
	 *
	 *  2: Support multiple tracks
	 *  3: Changed portamento effect
	 *  4: Switched portamento effects for VRC7 (1xx & 2xx), adjusted Pxx for FDS
	 *  5: Adjusted FDS octave
	 *  (6: Noise pitch slide effects fix)
	 *
	 */ 

	VisitPatterns(doc, [&] (const CPatternData &pattern, int song, int ch, int index) {
		// Save all rows
		unsigned int PatternLen = MAX_PATTERN_LENGTH;
		//unsigned int PatternLen = Song.GetPatternLength();

		unsigned Items = pattern.GetNoteCount(PatternLen);
		if (!Items)
			return;
		file_.WriteBlockInt(song);		// Write track
		file_.WriteBlockInt(ch);		// Write channel
		file_.WriteBlockInt(index);		// Write pattern
		file_.WriteBlockInt(Items);		// Number of items

		pattern.VisitRows(PatternLen, [&] (const stChanNote &note, unsigned row) {
			if (note == stChanNote { })
				return;
			file_.WriteBlockInt(row);
			file_.WriteBlockChar(note.Note);
			file_.WriteBlockChar(note.Octave);
			file_.WriteBlockChar(note.Instrument);
			file_.WriteBlockChar(note.Vol);
			for (int n = 0, EffColumns = doc.GetEffColumns(song, ch) + 1; n < EffColumns; ++n) {
				file_.WriteBlockChar(EFF_CONVERSION_050.second[note.EffNumber[n]]);		// // // 050B
				file_.WriteBlockChar(note.EffParam[n]);
			}
		});
	});
}

void CFamiTrackerDocIO::LoadDSamples(CFamiTrackerDoc &doc, int ver) {
	unsigned int Count = AssertRange(
		static_cast<unsigned char>(file_.GetBlockChar()), 0U, CDSampleManager::MAX_DSAMPLES, "DPCM sample count");

	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = AssertRange(
			static_cast<unsigned char>(file_.GetBlockChar()), 0U, CDSampleManager::MAX_DSAMPLES - 1, "DPCM sample index");
		try {
			auto pSample = std::make_unique<CDSample>();		// // //
			unsigned int Len = AssertRange(file_.GetBlockInt(), 0, CDSample::MAX_NAME_SIZE - 1, "DPCM sample name length");
			char Name[CDSample::MAX_NAME_SIZE] = { };
			file_.GetBlock(Name, Len);
			pSample->SetName(Name);
			int Size = AssertRange(file_.GetBlockInt(), 0, 0x7FFF, "DPCM sample size");
			AssertFileData<MODULE_ERROR_STRICT>(Size <= 0xFF1 && Size % 0x10 == 1, "Bad DPCM sample size");
			int TrueSize = Size + ((1 - Size) & 0x0F);		// // //
			auto pData = std::make_unique<char[]>(TrueSize);
			file_.GetBlock(pData.get(), Size);
			memset(pData.get() + Size, 0xAA, TrueSize - Size);
			pSample->SetData(TrueSize, pData.release());
			doc.SetSample(Index, pSample.release());
		}
		catch (CModuleException *e) {
			e->AppendError("At DPCM sample %d,", Index);
			throw;
		}
	}
}

void CFamiTrackerDocIO::SaveDSamples(const CFamiTrackerDoc &doc, int ver) {
	const auto &manager = *doc.GetDSampleManager();
	if (int Count = manager.GetSampleCount()) {		// // //
		// Write sample count
		file_.WriteBlockChar(Count);

		for (unsigned int i = 0; i < CDSampleManager::MAX_DSAMPLES; ++i) {
			if (const CDSample *pSamp = manager.GetDSample(i)) {
				// Write sample
				file_.WriteBlockChar(i);
				int Length = strlen(pSamp->GetName());
				file_.WriteBlockInt(Length);
				file_.WriteBlock(pSamp->GetName(), Length);
				file_.WriteBlockInt(pSamp->GetSize());
				file_.WriteBlock(pSamp->GetData(), pSamp->GetSize());
			}
		}
	}
}

void CFamiTrackerDocIO::LoadComments(CFamiTrackerDoc &doc, int ver) {
	bool disp = file_.GetBlockInt() == 1;
	doc.SetComment(file_.ReadString().GetString(), disp);
}

void CFamiTrackerDocIO::SaveComments(const CFamiTrackerDoc &doc, int ver) {
	if (const auto &str = doc.GetComment(); !str.empty()) {
		file_.WriteBlockInt(doc.ShowCommentOnOpen() ? 1 : 0);
		file_.WriteString(str.c_str());
	}
}

void CFamiTrackerDocIO::LoadSequencesVRC6(CFamiTrackerDoc &doc, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "VRC6 sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES), "VRC6 sequence count");		// // //

	CSequenceManager *pManager = doc.GetSequenceManager(INST_VRC6);		// // //

	int Indices[MAX_SEQUENCES * SEQ_COUNT];
	int Types[MAX_SEQUENCES * SEQ_COUNT];
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = Indices[i] = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int Type = Types[i] = AssertRange(file_.GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = file_.GetBlockChar();
			CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point"));

			if (ver == 4) {
				pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
					file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
				pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //
			}

			// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
			for (unsigned int j = 0; j < SeqCount; ++j) {
				char Value = file_.GetBlockChar();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At VRC6 %s sequence %d,", CInstrumentVRC6::SEQUENCE_NAME[Type], Index);
			throw;
		}
	}

	if (ver == 5) {
		// Version 5 saved the release points incorrectly, this is fixed in ver 6
		for (int i = 0; i < MAX_SEQUENCES; ++i) {
			for (int j = 0; j < SEQ_COUNT; ++j) try {
				int ReleasePoint = file_.GetBlockInt();
				int Settings = file_.GetBlockInt();
				CSequence *pSeq = pManager->GetCollection(j)->GetSequence(i);
				int Length = pSeq->GetItemCount();
				if (Length > 0) {
					pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
						ReleasePoint, -1, Length - 1, "Sequence release point"));
					pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
				}
			}
			catch (CModuleException *e) {
				e->AppendError("At VRC6 %s sequence %d,", CInstrumentVRC6::SEQUENCE_NAME[j], i);
				throw;
			}
		}
	}
	else if (ver >= 6) {
		for (unsigned int i = 0; i < Count; ++i) try {
			CSequence *pSeq = pManager->GetCollection(Types[i])->GetSequence(Indices[i]);
			pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(pSeq->GetItemCount()) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //
		}
		catch (CModuleException *e) {
			e->AppendError("At VRC6 %s sequence %d,", CInstrumentVRC6::SEQUENCE_NAME[Types[i]], Indices[i]);
			throw;
		}
	}
}

void CFamiTrackerDocIO::SaveSequencesVRC6(const CFamiTrackerDoc &doc, int ver) {
	int Count = doc.GetTotalSequenceCount(INST_VRC6);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(doc.GetSequenceManager(INST_VRC6), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(seqType);
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});

	// v6
	VisitSequences(doc.GetSequenceManager(INST_VRC6), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
	});
}

void CFamiTrackerDocIO::LoadSequencesN163(CFamiTrackerDoc &doc, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "N163 sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "N163 sequence count");		// // //

	CSequenceManager *pManager = doc.GetSequenceManager(INST_N163);		// // //

	for (unsigned int i = 0; i < Count; i++) {
		unsigned int  Index		   = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int  Type		   = AssertRange(file_.GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = file_.GetBlockChar();
			CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point"));
			pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //

			// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
			for (int j = 0; j < SeqCount; ++j) {
				char Value = file_.GetBlockChar();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At N163 %s sequence %d,", CInstrumentN163::SEQUENCE_NAME[Type], Index);
			throw;
		}
	}
}

void CFamiTrackerDocIO::SaveSequencesN163(const CFamiTrackerDoc &doc, int ver) {
	/* 
	 * Store N163 sequences
	 */ 

	int Count = doc.GetTotalSequenceCount(INST_N163);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(doc.GetSequenceManager(INST_N163), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(seqType);
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});
}

void CFamiTrackerDocIO::LoadSequencesS5B(CFamiTrackerDoc &doc, int ver) {
	unsigned int Count = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "5B sequence count");
	AssertRange<MODULE_ERROR_OFFICIAL>(Count, 0U, static_cast<unsigned>(MAX_SEQUENCES * SEQ_COUNT - 1), "5B sequence count");		// // //

	CSequenceManager *pManager = doc.GetSequenceManager(INST_S5B);		// // //

	for (unsigned int i = 0; i < Count; i++) {
		unsigned int  Index		   = AssertRange(file_.GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int  Type		   = AssertRange(file_.GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = file_.GetBlockChar();
			CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point"));
			pSeq->SetReleasePoint(AssertRange<MODULE_ERROR_STRICT>(
				file_.GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(file_.GetBlockInt()));		// // //

			// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
			for (int j = 0; j < SeqCount; ++j) {
				char Value = file_.GetBlockChar();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At 5B %s sequence %d,", CInstrumentS5B::SEQUENCE_NAME[Type], Index);
			throw;
		}
	}
}

void CFamiTrackerDocIO::SaveSequencesS5B(const CFamiTrackerDoc &doc, int ver) {
	int Count = doc.GetTotalSequenceCount(INST_S5B);
	if (!Count)
		return;		// // //
	file_.WriteBlockInt(Count);

	VisitSequences(doc.GetSequenceManager(INST_S5B), [&] (const CSequence &seq, int index, int seqType) {
		file_.WriteBlockInt(index);
		file_.WriteBlockInt(seqType);
		file_.WriteBlockChar(seq.GetItemCount());
		file_.WriteBlockInt(seq.GetLoopPoint());
		file_.WriteBlockInt(seq.GetReleasePoint());
		file_.WriteBlockInt(seq.GetSetting());
		for (int k = 0, Count = seq.GetItemCount(); k < Count; ++k)
			file_.WriteBlockChar(seq.GetItem(k));
	});
}

void CFamiTrackerDocIO::LoadParamsExtra(CFamiTrackerDoc &doc, int ver) {
	doc.SetLinearPitch(file_.GetBlockInt() != 0);
	if (ver >= 2) {
		int semitone = AssertRange(file_.GetBlockChar(), -12, 12, "Global semitone tuning");
		int cent = AssertRange(file_.GetBlockChar(), -100, 100, "Global cent tuning");
		doc.SetTuning(semitone, cent);
	}
}

void CFamiTrackerDocIO::SaveParamsExtra(const CFamiTrackerDoc &doc, int ver) {
	bool linear = doc.GetLinearPitch();
	char semitone = doc.GetTuningSemitone();
	char cent = doc.GetTuningCent();
	if (linear || semitone || cent) {
		file_.WriteBlockInt(linear);
		if (ver >= 2) {
			file_.WriteBlockChar(semitone);
			file_.WriteBlockChar(cent);
		}
	}
}

#include "DetuneDlg.h" // TODO: bad, encapsulate detune tables

void CFamiTrackerDocIO::LoadDetuneTables(CFamiTrackerDoc &doc, int ver) {
	int Count = AssertRange(file_.GetBlockChar(), 0, 6, "Detune table count");
	for (int i = 0; i < Count; ++i) {
		int Chip = AssertRange(file_.GetBlockChar(), 0, 5, "Detune table index");
		try {
			int Item = AssertRange(file_.GetBlockChar(), 0, NOTE_COUNT, "Detune table note count");
			for (int j = 0; j < Item; ++j) {
				int Note = AssertRange(file_.GetBlockChar(), 0, NOTE_COUNT - 1, "Detune table note index");
				int Offset = file_.GetBlockInt();
				doc.SetDetuneOffset(Chip, Note, Offset);
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At %s detune table,", CDetuneDlg::CHIP_STR[Chip]);
			throw;
		}
	}
}

void CFamiTrackerDocIO::SaveDetuneTables(const CFamiTrackerDoc &doc, int ver) {
	int NoteUsed[6] = { };
	int ChipCount = 0;
	for (size_t i = 0; i < std::size(NoteUsed); ++i) {
		for (int j = 0; j < NOTE_COUNT; j++)
			if (doc.GetDetuneOffset(i, j) != 0)
				++NoteUsed[i];
		if (NoteUsed[i])
			++ChipCount;
	}
	if (!ChipCount)
		return;

	file_.WriteBlockChar(ChipCount);
	for (int i = 0; i < 6; i++) {
		if (!NoteUsed[i])
			continue;
		file_.WriteBlockChar(i);
		file_.WriteBlockChar(NoteUsed[i]);
		for (int j = 0; j < NOTE_COUNT; j++)
			if (int detune = doc.GetDetuneOffset(i, j)) {
				file_.WriteBlockChar(j);
				file_.WriteBlockInt(detune);
			}
	}
}

void CFamiTrackerDocIO::LoadGrooves(CFamiTrackerDoc &doc, int ver) {
	const int Count = AssertRange(file_.GetBlockChar(), 0, MAX_GROOVE, "Groove count");

	for (int i = 0; i < Count; i++) {
		int Index = AssertRange(file_.GetBlockChar(), 0, MAX_GROOVE - 1, "Groove index");
		try {
			int Size = AssertRange(file_.GetBlockChar(), 1, MAX_GROOVE_SIZE, "Groove size");
			auto pGroove = std::make_unique<CGroove>();
			pGroove->SetSize(Size);
			for (int j = 0; j < Size; ++j) try {
				pGroove->SetEntry(j, AssertRange(
					static_cast<unsigned char>(file_.GetBlockChar()), 1U, 0xFFU, "Groove item"));
			}
			catch (CModuleException *e) {
				e->AppendError("At position %i,", j);
				throw;
			}
			doc.SetGroove(Index, std::move(pGroove));
		}
		catch (CModuleException *e) {
			e->AppendError("At groove %i,", Index);
			throw;
		}
	}

	unsigned int Tracks = file_.GetBlockChar();
	AssertFileData<MODULE_ERROR_STRICT>(Tracks == doc.GetTrackCount(), "Use-groove flag count does not match track count");
	for (unsigned i = 0; i < Tracks; ++i) try {
		bool Use = file_.GetBlockChar() == 1;
		if (i >= doc.GetTrackCount())
			continue;
		int Speed = doc.GetSongSpeed(i);
		doc.SetSongGroove(i, Use);
		if (Use)
			AssertRange(Speed, 0, MAX_GROOVE - 1, "Track default groove index");
		else
			AssertRange(Speed, 1, MAX_TEMPO, "Track default speed");
	}
	catch (CModuleException *e) {
		e->AppendError("At track %d,", i + 1);
		throw;
	}
}

void CFamiTrackerDocIO::SaveGrooves(const CFamiTrackerDoc &doc, int ver) {
	int Count = 0;
	for (int i = 0; i < MAX_GROOVE; ++i)
		if (doc.GetGroove(i))
			++Count;
	if (!Count)
		return;

	file_.WriteBlockChar(Count);
	for (int i = 0; i < MAX_GROOVE; ++i)
		if (const auto pGroove = doc.GetGroove(i)) {
			int Size = pGroove->GetSize();
			file_.WriteBlockChar(i);
			file_.WriteBlockChar(Size);
			for (int j = 0; j < Size; ++j)
				file_.WriteBlockChar(pGroove->GetEntry(j));
		}

	file_.WriteBlockChar(doc.GetTrackCount());
	doc.VisitSongs([&] (const CSongData &song) { file_.WriteBlockChar(song.GetSongGroove()); });
}

void CFamiTrackerDocIO::LoadBookmarks(CFamiTrackerDoc &doc, int ver) {
	auto &Manager = *doc.GetBookmarkManager();

	for (int i = 0, n = file_.GetBlockInt(); i < n; ++i) {
		auto pMark = std::make_unique<CBookmark>();
		unsigned int Track = AssertRange(
			static_cast<unsigned char>(file_.GetBlockChar()), 0, doc.GetTrackCount() - 1, "Bookmark track index");
		int Frame = static_cast<unsigned char>(file_.GetBlockChar());
		int Row = static_cast<unsigned char>(file_.GetBlockChar());
		pMark->m_iFrame = AssertRange(Frame, 0, static_cast<int>(doc.GetFrameCount(Track)) - 1, "Bookmark frame index");
		pMark->m_iRow = AssertRange(Row, 0, static_cast<int>(doc.GetPatternLength(Track)) - 1, "Bookmark row index");
		pMark->m_Highlight.First = file_.GetBlockInt();
		pMark->m_Highlight.Second = file_.GetBlockInt();
		pMark->m_bPersist = file_.GetBlockChar() != 0;
		pMark->m_sName = std::string(file_.ReadString());
		Manager.GetCollection(Track)->AddBookmark(pMark.release());
	}
}

void CFamiTrackerDocIO::SaveBookmarks(const CFamiTrackerDoc &doc, int ver) {
	auto &Manager = *doc.GetBookmarkManager();
	int Count = Manager.GetBookmarkCount();
	if (!Count)
		return;
	file_.WriteBlockInt(Count);
	
	for (unsigned int i = 0, n = doc.GetTrackCount(); i < n; ++i) {
		for (const auto &pMark : *Manager.GetCollection(i)) {
			file_.WriteBlockChar(i);
			file_.WriteBlockChar(pMark->m_iFrame);
			file_.WriteBlockChar(pMark->m_iRow);
			file_.WriteBlockInt(pMark->m_Highlight.First);
			file_.WriteBlockInt(pMark->m_Highlight.Second);
			file_.WriteBlockChar(pMark->m_bPersist);
			//file_.WriteBlockInt(pMark->m_sName.size());
			//file_.WriteBlock(pMark->m_sName, (int)strlen(Name));	
			file_.WriteString(pMark->m_sName.c_str());
		}
	}
}

template <module_error_level_t l>
void CFamiTrackerDocIO::AssertFileData(bool Cond, const std::string &Msg) const {
	if (l <= theApp.GetSettings()->Version.iErrorLevel && !Cond) {
		CModuleException *e = file_.GetException();
//		CModuleException *e = m_pCurrentDocument ? m_pCurrentDocument->GetException() :
//			std::make_unique<CModuleException>().release();
		e->AppendError(Msg);
		e->Raise();
	}
}

template<module_error_level_t l, typename T, typename U, typename V>
T CFamiTrackerDocIO::AssertRange(T Value, U Min, V Max, const std::string &Desc) const try {
	return CModuleException::AssertRangeFmt<l>(Value, Min, Max, Desc);
}
catch (CModuleException *e) {
	file_.SetDefaultFooter(e);
//	if (m_pCurrentDocument)
//		m_pCurrentDocument->SetDefaultFooter(e);
	throw;
}
