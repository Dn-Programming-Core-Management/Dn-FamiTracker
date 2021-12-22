/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2015 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2021 D.P.C.M.
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

// The instrument file tree, used in the instrument toolbar to quickly load an instrument

#include "stdafx.h"
#include "InstrumentFileTree.h"

CInstrumentFileTree::CInstrumentFileTree() : m_pRootMenu(NULL), m_iFileIndex(0), m_bShouldRebuild(true)
{
}

CInstrumentFileTree::~CInstrumentFileTree()
{
	DeleteMenuObjects();
}

void CInstrumentFileTree::DeleteMenuObjects()
{
	SAFE_RELEASE(m_pRootMenu);

	for (int i = 0; i < m_menuArray.GetCount(); ++i) {
		SAFE_RELEASE(m_menuArray[i]);
	}

	m_menuArray.RemoveAll();
	m_iTotalMenusAdded = 0;

	TRACE("Cleared instrument file tree\n");
}

CString CInstrumentFileTree::GetFile(int Index) const
{
	ASSERT(Index >= MENU_BASE + 2);
	return m_fileList[Index - MENU_BASE - 2];
}

void CInstrumentFileTree::Changed()
{
	m_bShouldRebuild = true;
}

bool CInstrumentFileTree::ShouldRebuild() const
{
	// Check if tree expired, to allow changes in the file system to be visible
	return (GetTickCount() > m_iTimeout) || m_bShouldRebuild;
}

bool CInstrumentFileTree::BuildMenuTree(CString instrumentPath)
{
	CWaitCursor wait;

	DeleteMenuObjects();
	m_fileList.RemoveAll();

	TRACE("Building instrument file tree...\n");

	m_pRootMenu = new CMenu();

	m_pRootMenu->CreatePopupMenu();
	m_pRootMenu->AppendMenu(MF_STRING, MENU_BASE + 0, _T("Open file..."));
	m_pRootMenu->AppendMenu(MF_STRING, MENU_BASE + 1, _T("Select directory..."));
	m_pRootMenu->AppendMenu(MF_SEPARATOR);
	m_pRootMenu->SetDefaultItem(0, TRUE);

	if (instrumentPath.GetLength() == 0) {
		m_bShouldRebuild = true;
		m_pRootMenu->AppendMenu(MF_STRING | MF_DISABLED, MENU_BASE + 2, _T("(select a directory)"));
	}
	else {
		m_iFileIndex = 2;

		if (!ScanDirectory(instrumentPath, m_pRootMenu, 0)) {
			// No files found
			m_pRootMenu->AppendMenu(MF_STRING | MF_DISABLED, MENU_BASE + 2, _T("(no files found)"));
			m_bShouldRebuild = true;
		}
		else {
			m_fileList.FreeExtra();
			m_menuArray.FreeExtra();

			m_iTimeout = GetTickCount() + CACHE_TIMEOUT;
			m_bShouldRebuild = false;
		}
	}

	TRACE("Done\n");

	return true;
}

bool CInstrumentFileTree::ScanDirectory(CString path, CMenu *pMenu, int level)
{
	CFileFind fileFinder;
	bool bNoFile = true;

	if (level > RECURSION_LIMIT)
		return false;

	BOOL working = fileFinder.FindFile(path + _T("\\*.*"));

	// First scan directories
	while (working) {
		working = fileFinder.FindNextFile();

		if (fileFinder.IsDirectory() && !fileFinder.IsHidden() && !fileFinder.IsDots() && m_iTotalMenusAdded++ < MAX_MENUS) {
			CMenu *pSubMenu = new CMenu();
			m_menuArray.Add(pSubMenu);
			pSubMenu->CreatePopupMenu();
			// Recursive scan
			bool bDisabled = false;
			if (!ScanDirectory(path + _T("\\") + fileFinder.GetFileName(), pSubMenu, level + 1))
				bDisabled = true;
			pMenu->AppendMenu(MF_STRING | MF_POPUP | (bDisabled ? MF_DISABLED : MF_ENABLED), (UINT)pSubMenu->m_hMenu, fileFinder.GetFileName());		
			bNoFile = false;
		}
	}

	working = fileFinder.FindFile(path + _T("\\*.fti"));

	// Then files
	while (working) {
		working = fileFinder.FindNextFile();
		pMenu->AppendMenu(MF_STRING | MF_ENABLED, MENU_BASE + m_iFileIndex++, fileFinder.GetFileTitle());
		m_fileList.Add(path + _T("\\") + fileFinder.GetFileName());
		bNoFile = false;
	}

	return !bNoFile;
}

CMenu *CInstrumentFileTree::GetMenu() const
{
	return m_pRootMenu;
}
