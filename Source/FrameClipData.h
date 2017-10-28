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


#pragma once

#include <memory>
#include "ClipboardResource.h"		// // //

class CFrameClipData : public CClipboardResource {		// // //
public:
	CFrameClipData() = default;
	CFrameClipData(int Channels, int Frames);

	int  GetFrame(int Frame, int Channel) const;
	void SetFrame(int Frame, int Channel, int Pattern);

private:
	SIZE_T GetAllocSize() const override;
	bool ContainsData() const override;		// // //
	bool ToBytes(unsigned char *pBuf) const override;
	bool FromBytes(const unsigned char *pBuf) override;

public:
	// Clip info
	struct {
		int Channels = 0;
		int Frames = 0;
		int FirstChannel = 0;
		struct {
			int SourceRowStart = 0;
			int SourceRowEnd = 0;
		} OleInfo;
	} ClipInfo;
	
	// Clip data
	std::unique_ptr<int[]> pFrames;
	int iSize = 0;
};
