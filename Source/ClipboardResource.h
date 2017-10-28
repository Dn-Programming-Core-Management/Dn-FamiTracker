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

#include "stdafx.h"

// // // interface for objects that can be transferred to/from clipboard

class CClipboardResource {
public:
	HGLOBAL AllocateGlobalMemory() const;
	bool WriteGlobalMemory(HGLOBAL hMem) const;
	bool ReadGlobalMemory(HGLOBAL hMem);
	DROPEFFECT DragDropTransfer(UINT clipboardID, DWORD effects) const;

	// Check whether object contains clipboard data
	virtual bool ContainsData() const = 0;		// // //

private:
	// Get memory size in bytes
	virtual SIZE_T GetAllocSize() const = 0;

	// Copy structures to memory, return success
	virtual bool ToBytes(unsigned char *pBuf) const = 0;		// // //

	// Copy structures from memory, return success
	virtual bool FromBytes(const unsigned char *pBuf) = 0;		// // //
};
