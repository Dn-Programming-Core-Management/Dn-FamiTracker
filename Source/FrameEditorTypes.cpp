/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2023 D.P.C.M.
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

#include "FrameEditorTypes.h"
#include <algorithm>
#include "FamiTrackerDoc.h"

// CFrameClipData //////////////////////////////////////////////////////////////


// Constructor/desctructor

CFrameClipData::CFrameClipData()
{
	memset(&ClipInfo, 0, sizeof(ClipInfo));
}

CFrameClipData::CFrameClipData(int Channels, int Frames) :
	pFrames(new int[Channels * Frames]), iSize(Channels * Frames)
{
	memset(&ClipInfo, 0, sizeof(ClipInfo));
	ClipInfo.Channels = Channels;		// // //
	ClipInfo.Frames = Frames;
}

CFrameClipData::~CFrameClipData()
{
	SAFE_RELEASE_ARRAY(pFrames);
}

SIZE_T CFrameClipData::GetAllocSize() const
{
	return sizeof(ClipInfo) + sizeof(int) * iSize;
}

void CFrameClipData::ToMem(HGLOBAL hMem) 
{
	// From CPatternClipData to memory
	ASSERT(hMem != NULL);

	BYTE *pByte = (BYTE*)::GlobalLock(hMem);

	if (pByte != NULL) {
		memcpy(pByte, &ClipInfo, sizeof(ClipInfo));
		memcpy(pByte + sizeof(ClipInfo), pFrames, sizeof(int) * iSize);

		::GlobalUnlock(hMem);
	}
}

void CFrameClipData::FromMem(HGLOBAL hMem)
{
	// From memory to CPatternClipData
	ASSERT(hMem != NULL);
	ASSERT(pFrames == NULL);

	BYTE *pByte = (BYTE*)::GlobalLock(hMem);

	if (pByte != NULL) {
		memcpy(&ClipInfo, pByte, sizeof(ClipInfo));

		iSize = ClipInfo.Channels * ClipInfo.Frames;
		pFrames = new int[iSize];

		memcpy(pFrames, pByte + sizeof(ClipInfo), sizeof(int) * iSize);

		::GlobalUnlock(hMem);
	}
}

int CFrameClipData::GetFrame(int Frame, int Channel) const
{
	ASSERT(Frame >= 0 && Frame < ClipInfo.Frames);
	ASSERT(Channel >= 0 && Channel < ClipInfo.Channels);

	return *(pFrames + (Frame * ClipInfo.Channels + Channel));
}

void CFrameClipData::SetFrame(int Frame, int Channel, int Pattern)
{
	ASSERT(Frame >= 0 && Frame < ClipInfo.Frames);
	ASSERT(Channel >= 0 && Channel < ClipInfo.Channels);

	*(pFrames + (Frame * ClipInfo.Channels + Channel)) = Pattern;
}

// // // CFrameSelection class

int CFrameSelection::GetFrameStart() const
{
	return (m_cpEnd.m_iFrame > m_cpStart.m_iFrame) ? m_cpStart.m_iFrame : m_cpEnd.m_iFrame;
}

int CFrameSelection::GetFrameEnd() const
{
	return (m_cpEnd.m_iFrame > m_cpStart.m_iFrame) ? m_cpEnd.m_iFrame : m_cpStart.m_iFrame;
}

int CFrameSelection::GetChanStart() const
{
	return (m_cpEnd.m_iChannel > m_cpStart.m_iChannel) ? m_cpStart.m_iChannel : m_cpEnd.m_iChannel;
}

int CFrameSelection::GetChanEnd() const
{
	return (m_cpEnd.m_iChannel > m_cpStart.m_iChannel) ? m_cpEnd.m_iChannel : m_cpStart.m_iChannel;
}

void CFrameSelection::Normalize(CFrameCursorPos &Begin, CFrameCursorPos &End) const
{
	CFrameCursorPos Temp {GetFrameStart(), GetChanStart()};
	std::swap(End, CFrameCursorPos {GetFrameEnd(), GetChanEnd()});
	std::swap(Begin, Temp);
}

CFrameSelection CFrameSelection::GetNormalized() const
{
	CFrameSelection Sel;
	Normalize(Sel.m_cpStart, Sel.m_cpEnd);
	return Sel;
}

// // // CFrameIterator class

CFrameIterator::CFrameIterator(const CFrameIterator &it) :
	m_iTrack(it.m_iTrack), m_pDocument(it.m_pDocument), CFrameCursorPos(static_cast<const CFrameCursorPos &>(it))
{
}

CFrameIterator::CFrameIterator(CFamiTrackerDoc *pDoc, int Track, const CFrameCursorPos &Pos) :
	m_iTrack(Track), m_pDocument(pDoc), CFrameCursorPos(Pos)
{
	m_iFrame = NormalizeFrame(m_iFrame);
}

std::pair<CFrameIterator, CFrameIterator> CFrameIterator::FromCursor(const CFrameCursorPos &Pos, CFamiTrackerDoc *const pDoc, int Track)
{
	return std::make_pair(
		CFrameIterator {pDoc, Track, Pos},
		CFrameIterator {pDoc, Track, Pos}
	);
}

std::pair<CFrameIterator, CFrameIterator> CFrameIterator::FromSelection(const CFrameSelection &Sel, CFamiTrackerDoc *const pDoc, int Track)
{
	CFrameCursorPos it, end;
	Sel.Normalize(it, end);
	return std::make_pair(
		CFrameIterator {pDoc, Track, it},
		CFrameIterator {pDoc, Track, end}
	);
}

int CFrameIterator::Get(int Channel) const
{
	return m_pDocument->GetPatternAtFrame(m_iTrack, NormalizeFrame(m_iFrame), Channel);
}

void CFrameIterator::Set(int Channel, int Frame)
{
	m_pDocument->SetPatternAtFrame(m_iTrack, NormalizeFrame(m_iFrame), Channel, Frame);
}

CFrameIterator& CFrameIterator::operator+=(const int Frames)
{
	m_iFrame = NormalizeFrame(m_iFrame + Frames);
	return *this;
}

CFrameIterator& CFrameIterator::operator-=(const int Frames)
{
	return operator+=(-Frames);
}

CFrameIterator& CFrameIterator::operator++()
{
	return operator+=(1);
}

CFrameIterator CFrameIterator::operator++(int)
{
	CFrameIterator tmp(*this);
	operator+=(1);
	return tmp;
}

CFrameIterator& CFrameIterator::operator--()
{
	return operator+=(-1);
}

CFrameIterator CFrameIterator::operator--(int)
{
	CFrameIterator tmp(*this);
	operator+=(-1);
	return tmp;
}

bool CFrameIterator::operator==(const CFrameIterator &other) const
{
	return m_iFrame == other.m_iFrame;
}

int CFrameIterator::NormalizeFrame(int Frame) const
{
	int Frames = m_pDocument->GetFrameCount(m_iTrack);
	Frame %= Frames;
	if (Frame < 0) Frame += Frames;
	return Frame;
}
