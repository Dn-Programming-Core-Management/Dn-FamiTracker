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

#include "stdafx.h"
#include "FamiTrackerTypes.h"		// // //
#include "PatternData.h"		// // //
#include "JsonExporter.h"
#include "FamiTrackerDoc.h"
#include "../version.h"		// // //
#include "Bookmark.h"		// !! !!
#include "BookmarkCollection.h"		// !! !!
#include "BookmarkManager.h"		// !! !!
#include "DocumentFile.h"		// !! !!
#include "TrackerChannel.h"
#include "InstrumentManager.h"
#include "SequenceManager.h"
#include "DSampleManager.h"
#include "SequenceCollection.h"

#include "DSample.h"		// // //
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"
#include "InstrumentFDS.h"
#include "InstrumentN163.h"
#include "InstrumentVRC7.h"
#include "InstrumentFactory.h"

#include <optional>

#define DEBUG_OUT(...) { CString s__; s__.Format(__VA_ARGS__); OutputDebugString(s__); }

using json = nlohmann::json;

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {

	template <typename T, typename KeyT>
	auto get_maybe(const json& j, const KeyT& k) {
		if (auto it = j.find(k); it != j.end())
			return it->template get<T>();
		return T{ };
	}

	template <typename T>
	T json_get_between(const json& j, const std::string& k, T lo, T hi) {
		auto v = j.at(k).get<json::number_integer_t>();
		if (v < static_cast<json::number_integer_t>(lo) || static_cast<json::number_integer_t>(hi) < v)
			throw std::invalid_argument{ "Value at " + k + " must be between [" +
				std::to_string(lo) + ", " + std::to_string(hi) + "], got " + std::to_string(v) };
		return static_cast<T>(v);
	}

	template <typename T>
	T json_get_between(const json& j, std::size_t k, T lo, T hi) {
		auto v = j.at(k).get<json::number_integer_t>();
		if (v < static_cast<json::number_integer_t>(lo) || static_cast<json::number_integer_t>(hi) < v)
			throw std::invalid_argument{ "Value at " + std::to_string(k) + " must be between [" +
				std::to_string(lo) + ", " + std::to_string(hi) + "], got " + std::to_string(v) };
		return static_cast<T>(v);
	}

	template <typename T, typename KeyT>
	auto get_maybe(const json& j, const KeyT& k, T&& def) {
		auto it = j.find(k);
		return it != j.end() ? it->template get<T>() : std::forward<T>(def);
	}

	template <typename F, typename KeyT>
	void json_maybe(const json& j, const KeyT& k, F f) {
		if (auto it = j.find(k); it != j.end())
			f(*it);
	}

} // namespace

namespace nlohmann {

	template <typename T>
	struct adl_serializer<std::optional<T>> {
		static void to_json(json& j, const std::optional<T>& x) {
			if (x.has_value())
				j = *x;
			else
				j = nullptr;
		}

		static void from_json(const json& j, std::optional<T>& x) {
			if (j.is_null())
				x = std::nullopt;
			else
				x = j.get<T>();
		}
	};

} // namespace nlohmann

namespace {

	constexpr std::string_view GetInstrumentChipName(inst_type_t inst_type) noexcept {
		switch (inst_type) {
		case inst_type_t::INST_2A03: return "2A03"sv;
		case inst_type_t::INST_VRC6: return "VRC6"sv;
		case inst_type_t::INST_VRC7: return "VRC7"sv;
		case inst_type_t::INST_FDS:  return "FDS"sv;
		case inst_type_t::INST_N163: return "N163"sv;
		case inst_type_t::INST_S5B:  return "5B"sv;
		default: return ""sv;
		}
	}

	constexpr std::string_view GetChannelChipName(uint8_t sndchip) noexcept {
		switch (sndchip) {
		case SNDCHIP_NONE: return "2A03"sv;
		case SNDCHIP_VRC6: return "VRC6"sv;
		case SNDCHIP_VRC7: return "VRC7"sv;
		case SNDCHIP_FDS:  return "FDS"sv;
		case SNDCHIP_MMC5: return "MMC5"sv;
		case SNDCHIP_N163: return "N163"sv;
		case SNDCHIP_S5B:  return "S5B"sv;
		default: return ""sv;
		}
	}


} // namespace

void to_json(json& j, const stChanNote& note) {
	switch (note.Note) {
	case note_t::NONE: j["kind"] = "none"; break;
	case note_t::HALT: j["kind"] = "halt"; break;
	case note_t::RELEASE: j["kind"] = "release"; break;
	case note_t::ECHO:
		j["kind"] = "echo";
		j["value"] = note.Octave;
		break;
	default:
		if (note.Note >= note_t::NOTE_C && note.Note <= note_t::NOTE_B) {
			j["kind"] = "note";
			j["value"] = MIDI_NOTE(note.Octave, note.Note);
		}
	}

	if (note.Vol < MAX_VOLUME)
		j["volume"] = note.Vol;
	if (note.Instrument < MAX_INSTRUMENTS)
		j["inst_index"] = note.Instrument;
	else if (note.Instrument == HOLD_INSTRUMENT)
		j["inst_index"] = -1;

	for (int i = 0; i < MAX_EFFECT_COLUMNS; ++i) {
		if (note.EffNumber[i] != effect_t::EF_NONE) {
			j["effects"] = json::array();
			for (int k = 0; k < MAX_EFFECT_COLUMNS; ++k) {
				if (note.EffNumber[k] != effect_t::EF_NONE) {
					j["effects"].push_back(json{
						{"column", k},
						{"name", std::string {EFF_CHAR[note.EffNumber[k]]}},
						{"param", (unsigned int)note.EffParam[k]},
					});
				}
			}
			break;
		}
	}
}

void to_json(json& j, const CBookmark& bm) {
	j = json{
		{"name", bm.m_sName},
		// Note: 0CC exports only a two-value array with no Offset field.
		{"highlight", json::array({ bm.m_Highlight.First, bm.m_Highlight.Second, bm.m_Highlight.Offset })},
		{"frame", bm.m_iFrame},
		{"row", bm.m_iRow},
		{"persist", bm.m_bPersist},
	};
}

void to_json(json& j, const CBookmarkCollection& bmcol) {
	j = json::array();
	for (unsigned int i = 0; i < bmcol.GetCount(); ++i)
	{
		CBookmark* bm = bmcol.GetBookmark(i);
		j.push_back(json(*bm));
	}
}

void to_json(json& j, const CSequence& seq) {
	j = json{
		{"items", json::array()},
		{"setting_id", seq.GetSetting()},
	};
	for (unsigned i = 0; i < seq.GetItemCount(); ++i)
		j["items"].push_back(seq.GetItem(i));

	if (auto loop = seq.GetLoopPoint(); loop != (unsigned)-1)
		j["loop"] = loop;
	if (auto release = seq.GetReleasePoint(); release != (unsigned)-1)
		j["release"] = release;
}

void to_json(json& j, const CDSample& dpcm) {
	j = json{
		{"name", std::string {dpcm.GetName() ? dpcm.GetName() : ""}},
		{"values", json::array()},
	};
	char* data = dpcm.GetData();
	if (data != nullptr)
	{
		for (std::size_t i = 0, n = dpcm.GetSize(); i < n; ++i)
		{
			j["values"].push_back((unsigned int)(unsigned char)data[i]);
		}
	}
}

void to_json(json& j, const CDSampleManager& dmanager) {
	j = json::array();
	for (unsigned i = 0; i < MAX_DSAMPLES; ++i) {
		if (const CDSample* sample = dmanager.GetDSample(i)) {
			json dj = json(*sample);
			dj["index"] = i;
			j.push_back(std::move(dj));
		}
	}
}

namespace {

	void to_json_common(json& j, const CInstrument& inst) {
		j = json{
			{"name", std::string {inst.GetName()}},
			{"chip", std::string {GetInstrumentChipName(inst.GetType())}},
		};
	}

} // namespace

void to_json_seq(json& j, const CSeqInstrument& inst) {
	j["sequence_flags"] = json::array();
	for (sequence_t t = sequence_t::SEQ_VOLUME; t < sequence_t::SEQ_COUNT; t = (sequence_t)(unsigned int)(t+1))
		if (inst.GetSeqEnable(t))
			j["sequence_flags"].push_back(json{
				{"macro_id", (unsigned int)t},
				{"seq_index", inst.GetSeqIndex(t)},
				});
}

void to_json_2a03(json& j, const CInstrument2A03& inst) {
	to_json_seq(j, static_cast<const CSeqInstrument&>(inst));

	j["dpcm_map"] = json::array();
	// FIXME: what's causing this off-by-one?
	for (int n = 1; n < NOTE_COUNT; ++n)
	{
		int oct = GET_OCTAVE(n-1);
		int note = GET_NOTE(n-1);
		if (auto d_index = inst.GetSampleIndex(oct, note); d_index != 0)
		{
			j["dpcm_map"].push_back(json{
				{"dpcm_index", d_index-1},
				{"pitch", inst.GetSamplePitch(oct, note) & 0x0Fu},
				{"loop", inst.GetSampleLoop(oct, note)},
				{"delta", inst.GetSampleDeltaValue(oct, note)},
				{"note", n}, 
				});
		}
	}
}

void to_json_vrc7(json& j, const CInstrumentVRC7& inst) {
	if (inst.GetPatch() > 0)
		j["patch"] = inst.GetPatch();
	else {
		j["patch"] = json::array();
		for (int i = 0; i < 8; ++i)
			j["patch"].push_back(inst.GetCustomReg(i));
	}
}


void to_json_fds(json& j, const CInstrumentFDS& inst) {
	const auto InsertSeq = [&](sequence_t seq_type) {
		if (inst.GetSeqEnable(seq_type)) {
			auto sj = json(*inst.GetSequence(seq_type));
			sj["macro_id"] = (unsigned int)seq_type;
			j["sequences"].push_back(std::move(sj));
		}
	};

	j["sequences"] = json::array();
	InsertSeq(sequence_t::SEQ_VOLUME);
	InsertSeq(sequence_t::SEQ_ARPEGGIO);
	InsertSeq(sequence_t::SEQ_PITCH);

	json jw = json::array();
	for (int i = 0; i < CInstrumentFDS::WAVE_SIZE; ++i)
	{
		jw.push_back(inst.GetSample(i));
	}
	j["wave"] = jw;

	if (inst.GetModulationEnable()) {
		json jm = json::array();
		for (int i = 0; i < CInstrumentFDS::MOD_SIZE; ++i)
		{
			jm.push_back(inst.GetModulation(i));
		}
		j["modulation"] = json{
			{"table", jm},
			{"rate", inst.GetModulationSpeed()},
			{"depth", inst.GetModulationDepth()},
			{"delay", inst.GetModulationDelay()},
		};
	}
}

void to_json_n163(json& j, const CInstrumentN163& inst) {
	to_json_seq(j, static_cast<const CSeqInstrument&>(inst));

	j["waves"] = json::array();
	for (int w = 0; w < inst.GetWaveCount(); ++w)
	{
		json jw = json::array();
		for (int i = 0; i < CInstrumentFDS::WAVE_SIZE; ++i)
		{
			jw.push_back(inst.GetSample(w, i));
		}
		j["waves"].push_back(jw);
	}

	j["wave_position"] = inst.GetWavePos();
}

void to_json(json& j, const CInstrument& inst) {
	to_json_common(j, inst);

	if (auto p2A03 = dynamic_cast<const CInstrument2A03*>(&inst))
		to_json_2a03(j, *p2A03);
	else if (auto pVRC7 = dynamic_cast<const CInstrumentVRC7*>(&inst))
		to_json_vrc7(j, *pVRC7);
	else if (auto pFDS = dynamic_cast<const CInstrumentFDS*>(&inst))
		to_json_fds(j, *pFDS);
	else if (auto pN163 = dynamic_cast<const CInstrumentN163*>(&inst))
		to_json_n163(j, *pN163);
	else if (auto pSeq = dynamic_cast<const CSeqInstrument*>(&inst))
		to_json_seq(j, *pSeq);
}

void to_json(json& j, const CGroove& groove) {
	j = json{
		{"values", json::array()},
	};
	for (unsigned int i = 0; i < groove.GetSize(); ++i)
		j["values"].push_back(groove.GetEntry(i));
}

void to_json(json& j, const CFamiTrackerDoc& modfile) {

	auto channels = json::array();
	for (int i = 0; i < modfile.GetChannelCount(); ++i)
	{
		CTrackerChannel* ch = modfile.GetChannel(i);
		channels.push_back({
			{ "chip", GetChannelChipName(ch->GetChip())},
			//{ "subindex", ???},
			});
	}

	j = json{
		{"metadata", {
			{"title", std::string {modfile.GetSongName()}},
			{"artist", std::string {modfile.GetSongArtist()}},
			{"copyright", std::string {modfile.GetSongCopyright()}},
			{"comment", std::string {modfile.GetComment()}},
			{"show_comment_on_open", modfile.ShowCommentOnOpen()},
		}},
		{"global", {
			{"machine", modfile.GetMachine() == machine_t::PAL ? "pal" : "ntsc"},
			{"engine_speed", modfile.GetEngineSpeed()},
			{"vibrato_style", modfile.GetVibratoStyle() == vibrato_t::VIBRATO_OLD ? "old" : "new"},
			{"linear_pitch", modfile.GetLinearPitch()},
			{"fxx_split_point", modfile.GetSpeedSplitPoint()},
			{"detune", {
				{"semitones", modfile.GetTuningSemitone()},
				{"cents", modfile.GetTuningCent()},
			}},
		}},

		{"channels", channels},
		{"songs", json::array()},
		{"instruments", json::array()},
		{"sequences", json::array()},
		{"dpcm_samples", json(*modfile.GetInstrumentManager()->GetDSampleManager())},
		{"detunes", json::array()},
		{"grooves", json::array()},
	};

	for (unsigned int Track = 0; Track < modfile.GetTrackCount(); ++Track)
	{
		auto sj = json{
			{"speed", modfile.GetSongSpeed(Track)},
			{"tempo", modfile.GetSongTempo(Track)},
			{"frames", modfile.GetFrameCount(Track)},
			{"rows", modfile.GetPatternLength(Track)},
			{"title", std::string {modfile.GetTrackTitle(Track)}},
			{"uses_groove", modfile.GetSongGroove(Track)},
			{"tracks", json::array()},
			// Note: 0CC exports only a two-value array with no Offset field.
			{"highlight", {modfile.GetHighlight().First, modfile.GetHighlight().Second, modfile.GetHighlight().Offset}},
		};

		// TODO
		for (int Channel = 0; Channel < modfile.GetChannelCount(); ++Channel)
		{
			json cj = json{
				{ "frame_list", json::array() },
				{ "patterns", json::array() },
				{ "effect_columns", MAX_EFFECT_COLUMNS },
				{ "chip", GetChannelChipName((inst_type_t)modfile.GetChipType(Channel))},
				//{ "subindex", ???}
			};

			for (unsigned int Frame = 0; Frame < modfile.GetFrameCount(Track); ++Frame)
			{
				cj["frame_list"].push_back(modfile.GetPatternAtFrame(Track, Frame, Channel));
			}

			// TODO
			for (unsigned int Pattern = 0; Pattern < MAX_PATTERN; ++Pattern)
			{
				// TODO
				json pj = json {
					{"index", Pattern},
					{"notes", json::array()}
				};
				int notes = 0;
				for (unsigned int row = 0; row < modfile.GetPatternLength(Track); ++row)
				{
					stChanNote note;
					modfile.GetDataAtPattern(Track, Pattern, Channel, row, &note);
					if (note != stChanNote{}) {
						notes++;
						json jn;
						to_json(jn, note);
						pj["notes"].push_back(json{
							{"row", row},
							{"note", jn},
						});
					}
				}

				if (notes > 0) {
					cj["patterns"].push_back(std::move(pj));
				}
			}
			sj["tracks"].push_back(std::move(cj));
		}
		j["songs"].push_back(std::move(sj));
	}

	for (unsigned i = 0; i < MAX_INSTRUMENTS; ++i)
		if (auto pInst = modfile.GetInstrumentManager()->GetInstrument(i)) {
			auto ij = json(*pInst);
			ij["index"] = i;
			j["instruments"].push_back(std::move(ij));
		}

	for (int i = 0; i < 6; ++i)
		for (int n = 0; n < NOTE_COUNT; ++n)
			if (auto offs = modfile.GetDetuneOffset(i, n))
				j["detunes"].push_back(json{
					{"table_id", i},
					{"note", n},
					{"offset", offs},
					});

	for (unsigned i = 0; i < MAX_GROOVE; ++i)
		if (auto pGroove = modfile.GetGroove(i)) {
			auto gj = json(*pGroove);
			gj["index"] = i;
			j["grooves"].push_back(std::move(gj));
		}

	const auto InsertSequences = [&](inst_type_t inst_type) {
		const CSequenceManager& smanager = *modfile.GetInstrumentManager()->GetSequenceManager(inst_type);
		auto name = std::string{ GetInstrumentChipName(inst_type) };
		for (sequence_t t = sequence_t::SEQ_VOLUME; t < sequence_t::SEQ_COUNT; t = (sequence_t)(unsigned int)(t+1))
			if (const CSequenceCollection* seqcol = smanager.GetCollection(t))
				for (unsigned i = 0; i < MAX_SEQUENCES; ++i)
					if (auto pSeq = seqcol->GetSequence(i)) {
						auto sj = json(*pSeq);
						sj["chip"] = name;
						sj["macro_id"] = (unsigned int)(t);
						sj["index"] = i;
						j["sequences"].push_back(std::move(sj));
					}
	};

	InsertSequences(INST_2A03);
	InsertSequences(INST_VRC6);
	//	InsertSequences(INST_FDS);
	InsertSequences(INST_N163);
	InsertSequences(INST_S5B);
}

const CString& CJsonExport::ExportFile(LPCTSTR FileName, CFamiTrackerDoc* Doc)
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

	std::string s = nlohmann::json(*Doc).dump();
	f.WriteString(s.c_str());
	return sResult;
}