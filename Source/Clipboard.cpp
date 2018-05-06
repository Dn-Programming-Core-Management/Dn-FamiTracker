/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
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

#include "stdafx.h"
#include "resource.h"		// // //
#include "Clipboard.h"

// CClipboard //////////////////////////////////////////////////////////////////

CClipboard::CClipboard(CWnd *pWnd, UINT clipboardFormat) : m_bOpened(pWnd->OpenClipboard() == TRUE), mClipboardFormat(clipboardFormat), m_hMemory(NULL)
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

HGLOBAL CClipboard::AllocMem(UINT Size) const
{
	return ::GlobalAlloc(GMEM_MOVEABLE, Size);
}

void CClipboard::SetData(HGLOBAL hMemory) const
{
	ASSERT(m_bOpened);

	::EmptyClipboard();
	::SetClipboardData(mClipboardFormat, hMemory);
}

bool CClipboard::SetDataPointer(LPVOID pData, UINT Size) const
{
	ASSERT(m_bOpened);

	HGLOBAL hMemory = AllocMem(Size);
	if (hMemory == NULL)
		return false;

	LPVOID pClipData = ::GlobalLock(hMemory);
	if (pClipData == NULL)
		return false;

	memcpy(pClipData, pData, Size);

	::GlobalUnlock(hMemory);
	SetData(hMemory);

	return true;
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
	hMemory = ::GetClipboardData(mClipboardFormat);
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
	return ::IsClipboardFormatAvailable(mClipboardFormat) == TRUE;
}
