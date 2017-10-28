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

#include "ClipboardResource.h"
#include <memory>

HGLOBAL CClipboardResource::AllocateGlobalMemory() const {
	return ::GlobalAlloc(GMEM_MOVEABLE, GetAllocSize());
}

bool CClipboardResource::WriteGlobalMemory(HGLOBAL hMem) const {
	if (ContainsData())
		if (auto pByte = (BYTE *)::GlobalLock(hMem)) {
			bool result = ToBytes(pByte);
			::GlobalUnlock(hMem);
			return result;
		}
	return false;
}

bool CClipboardResource::ReadGlobalMemory(HGLOBAL hMem) {
//	if (!ContainsData())
		if (auto pByte = (BYTE *)::GlobalLock(hMem)) {
			bool result = FromBytes(pByte);
			::GlobalUnlock(hMem);
			return result;
		}
	return false;
}

DROPEFFECT CClipboardResource::DragDropTransfer(UINT clipboardID, DWORD effects) const {
	DROPEFFECT res = DROPEFFECT_NONE;
	if (HGLOBAL hMem = AllocateGlobalMemory()) {
		if (WriteGlobalMemory(hMem)) {
			// Setup OLE
			auto pSrc = std::make_unique<COleDataSource>();
			pSrc->CacheGlobalData(clipboardID, hMem);
			res = pSrc->DoDragDrop(effects); // calls DropData
		}
		::GlobalFree(hMem);
	}
	return res;
}
