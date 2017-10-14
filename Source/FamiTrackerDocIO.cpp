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
#include "PatternData.h"
#include "PatternNote.h"

#include "DSampleManager.h"
#include "DSample.h"

// // // save/load functionality

CFamiTrackerDocIO::CFamiTrackerDocIO(CDocumentFile &file) :
	file_(file)
{
}

void CFamiTrackerDocIO::Load(CFamiTrackerDoc &doc) {
}

void CFamiTrackerDocIO::Save(const CFamiTrackerDoc &doc) {
}

void CFamiTrackerDocIO::LoadSamples(CFamiTrackerDoc &doc, int ver) {
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

void CFamiTrackerDocIO::SaveSamples(const CFamiTrackerDoc &doc) {
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

void CFamiTrackerDocIO::SaveComments(const CFamiTrackerDoc &doc) {
	if (const auto &str = doc.GetComment(); !str.IsEmpty()) {
		file_.WriteBlockInt(doc.ShowCommentOnOpen() ? 1 : 0);
		file_.WriteString(str);
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
