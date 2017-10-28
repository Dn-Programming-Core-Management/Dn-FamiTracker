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

#include "FrameClipData.h"

CFrameClipData::CFrameClipData(int Channels, int Frames) :
	pFrames(std::make_unique<int[]>(Channels * Frames)), iSize(Channels * Frames),
	ClipInfo({Channels, Frames})		// // //
{
}

SIZE_T CFrameClipData::GetAllocSize() const
{
	return sizeof(ClipInfo) + sizeof(int) * iSize;
}

bool CFrameClipData::ContainsData() const {		// // //
	return pFrames != nullptr;
}

bool CFrameClipData::ToBytes(unsigned char *pBuf) const		// // //
{
	memcpy(pBuf, &ClipInfo, sizeof(ClipInfo));
	memcpy(pBuf + sizeof(ClipInfo), pFrames.get(), sizeof(int) * iSize);
	return true;
}

bool CFrameClipData::FromBytes(const unsigned char *pBuf)		// // //
{
	memcpy(&ClipInfo, pBuf, sizeof(ClipInfo));
	iSize = ClipInfo.Channels * ClipInfo.Frames;
	pFrames = std::make_unique<int[]>(iSize);
	memcpy(pFrames.get(), pBuf + sizeof(ClipInfo), sizeof(int) * iSize);
	return true;
}

int CFrameClipData::GetFrame(int Frame, int Channel) const
{
	ASSERT(Frame >= 0 && Frame < ClipInfo.Frames);
	ASSERT(Channel >= 0 && Channel < ClipInfo.Channels);

	return pFrames[Frame * ClipInfo.Channels + Channel];		// // //
}

void CFrameClipData::SetFrame(int Frame, int Channel, int Pattern)
{
	ASSERT(Frame >= 0 && Frame < ClipInfo.Frames);
	ASSERT(Channel >= 0 && Channel < ClipInfo.Channels);

	pFrames[Frame * ClipInfo.Channels + Channel] = Pattern;		// // //
}
