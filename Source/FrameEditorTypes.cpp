/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2016 HertzDevil
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

#include "FrameEditorTypes.h"
#include <algorithm>

// CFrameClipData //////////////////////////////////////////////////////////////

void CFrameClipData::Alloc(int Size)
{
	iSize = Size;
	pFrames = new int[Size];
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

		iSize = ClipInfo.Channels * ClipInfo.Rows;
		pFrames = new int[iSize];

		memcpy(pFrames, pByte + sizeof(ClipInfo), sizeof(int) * iSize);

		::GlobalUnlock(hMem);
	}
}

int CFrameClipData::GetFrame(int Frame, int Channel) const
{
	ASSERT(Frame >= 0 && Frame < ClipInfo.Rows);
	ASSERT(Channel >= 0 && Channel < ClipInfo.Channels);

	return *(pFrames + (Frame * ClipInfo.Channels + Channel));
}

void CFrameClipData::SetFrame(int Frame, int Channel, int Pattern)
{
	ASSERT(Frame >= 0 && Frame < ClipInfo.Rows);
	ASSERT(Channel >= 0 && Channel < ClipInfo.Channels);

	*(pFrames + (Frame * ClipInfo.Channels + Channel)) = Pattern;
}

// // // CFrameSelection class

inline int CFrameSelection::GetFrameStart() const
{
	return (m_cpEnd.m_iFrame > m_cpStart.m_iFrame) ? m_cpStart.m_iFrame : m_cpEnd.m_iFrame;
}

inline int CFrameSelection::GetFrameEnd() const
{
	return (m_cpEnd.m_iFrame > m_cpStart.m_iFrame) ? m_cpEnd.m_iFrame : m_cpStart.m_iFrame;
}

inline int CFrameSelection::GetChanStart() const
{
	return (m_cpEnd.m_iChannel > m_cpStart.m_iChannel) ? m_cpStart.m_iChannel : m_cpEnd.m_iChannel;
}

inline int CFrameSelection::GetChanEnd() const
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
