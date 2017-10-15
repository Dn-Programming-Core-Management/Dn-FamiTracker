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

#include "DSampleManager.h"
#include "DSample.h"

#include "BookmarkManager.h"
#include "BookmarkCollection.h"

// // // save/load functionality

CFamiTrackerDocIO::CFamiTrackerDocIO(CDocumentFile &file) :
	file_(file)
{
}

void CFamiTrackerDocIO::Load(CFamiTrackerDoc &doc) {
	char buf[32];
	file_.GetBlock(buf, std::size(buf));
	doc.SetSongName(buf);
	file_.GetBlock(buf, std::size(buf));
	doc.SetSongArtist(buf);
	file_.GetBlock(buf, std::size(buf));
	doc.SetSongCopyright(buf);
}

void CFamiTrackerDocIO::Save(const CFamiTrackerDoc &doc) {
	file_.WriteBlock(doc.GetSongName(), 32);
	file_.WriteBlock(doc.GetSongArtist(), 32);
	file_.WriteBlock(doc.GetSongCopyright(), 32);
}

void CFamiTrackerDocIO::LoadSongInfo(CFamiTrackerDoc &doc, int ver) {
}

void CFamiTrackerDocIO::SaveSongInfo(const CFamiTrackerDoc &doc, int ver) {
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
	doc.SetComment(file_.ReadString(), disp);
}

void CFamiTrackerDocIO::SaveComments(const CFamiTrackerDoc &doc, int ver) {
	if (const auto &str = doc.GetComment(); !str.IsEmpty()) {
		file_.WriteBlockInt(doc.ShowCommentOnOpen() ? 1 : 0);
		file_.WriteString(str);
	}
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
	for (int i = 0; i < std::size(NoteUsed); ++i) {
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
			doc.SetGroove(Index, pGroove.get()); // TODO: fix
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
	for (unsigned i = 0; i < doc.GetTrackCount(); ++i)
		file_.WriteBlockChar(doc.GetSongGroove(i));
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
