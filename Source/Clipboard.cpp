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

#include "Clipboard.h"
#include "resource.h"		// // //
#include "ClipboardResource.h"		// // //

// CClipboard //////////////////////////////////////////////////////////////////

CClipboard::CClipboard(CWnd *pWnd, UINT Clipboard) : m_bOpened(pWnd->OpenClipboard() == TRUE), m_iClipboard(Clipboard), m_hMemory(NULL)
{
}

CClipboard::~CClipboard()
{
	if (m_hMemory != NULL)
		::GlobalUnlock(m_hMemory);

	if (m_bOpened)
		::CloseClipboard();
}

bool CClipboard::IsOpened() const
{
	return m_bOpened;
}

void CClipboard::SetData(HGLOBAL hMemory) const
{
	ASSERT(m_bOpened);

	::EmptyClipboard();
	::SetClipboardData(m_iClipboard, hMemory);
}

bool CClipboard::SetDataPointer(LPVOID pData, UINT Size) const
{
	ASSERT(m_bOpened);

	if (HGLOBAL hMemory = ::GlobalAlloc(GMEM_MOVEABLE, Size)) {
		if (LPVOID pClipData = ::GlobalLock(hMemory)) {
			memcpy(pClipData, pData, Size);
			SetData(hMemory);
			::GlobalUnlock(hMemory);
			return true;
		}
	}
	return false;
}

bool CClipboard::GetData(HGLOBAL &hMemory) const		// // //
{
	ASSERT(m_bOpened);
	if (!IsOpened()) {
		AfxMessageBox(IDS_CLIPBOARD_OPEN_ERROR);
		return false;
	}
	if (!IsDataAvailable()) {
		AfxMessageBox(IDS_CLIPBOARD_NOT_AVALIABLE);
		::CloseClipboard();
		return false;
	}
	hMemory = ::GetClipboardData(m_iClipboard);
	if (hMemory == nullptr) {
		AfxMessageBox(IDS_CLIPBOARD_PASTE_ERROR);
		return false;
	}
	return true;
}

LPVOID CClipboard::GetDataPointer()
{
	if (!GetData(m_hMemory))		// // //
		return NULL;

	return ::GlobalLock(m_hMemory);
}

bool CClipboard::IsDataAvailable() const
{
	return ::IsClipboardFormatAvailable(m_iClipboard) == TRUE;
}

bool CClipboard::TryCopy(const CClipboardResource &res) {		// // //
	if (auto hMem = res.AllocateGlobalMemory()) {
		if (res.WriteGlobalMemory(hMem)) {
			SetData(hMem);
			return true;
		}
	}
	return false;
}

bool CClipboard::TryRestore(CClipboardResource &res) const {		// // //
	HGLOBAL hMem;		// // //
	if (!GetData(hMem))
		return false;
	return res.ReadGlobalMemory(hMem);
}
