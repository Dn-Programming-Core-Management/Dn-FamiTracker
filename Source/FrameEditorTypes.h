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


#pragma once

#include "stdafx.h"

// // // common classes for frame editor

struct stSelectInfo {
	bool bSelecting;
	int iRowStart;
	int iRowEnd;
};

class CFrameClipData {
public:
	// Constructor/desctructor
	CFrameClipData() : pFrames(NULL), iSize(0) {
		memset(&ClipInfo, 0, sizeof(ClipInfo));
	}

	CFrameClipData(int Channels, int Frames) {
		memset(&ClipInfo, 0, sizeof(ClipInfo));
		Alloc(Channels * Frames);
	}

	virtual ~CFrameClipData() {
		SAFE_RELEASE_ARRAY(pFrames);
	}

	void Alloc(int Size);

	SIZE_T GetAllocSize() const;	// Get memory size in bytes
	void ToMem(HGLOBAL hMem);		// Copy structures to memory
	void FromMem(HGLOBAL hMem);		// Copy structures from memory
	
	int  GetFrame(int Frame, int Channel) const;
	void SetFrame(int Frame, int Channel, int Pattern);

public:
	// Clip info
	struct {
		int Channels;
		int Rows;
		int FirstChannel;
		struct {
			int SourceRowStart;
			int SourceRowEnd;
		} OleInfo;
	} ClipInfo;
	
	// Clip data
	int *pFrames;
	int iSize;
};
