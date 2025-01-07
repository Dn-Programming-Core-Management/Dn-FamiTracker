/*
** Dn-FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2020-2025 D.P.C.M.
** FamiTracker Copyright (C) 2005-2020 Jonathan Liss
** 0CC-FamiTracker Copyright (C) 2014-2018 HertzDevil
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program. If not, see https://www.gnu.org/licenses/.
*/

#pragma once


// CInstrumentFileTree

class CInstrumentFileTree
{
public:
	CInstrumentFileTree();
	~CInstrumentFileTree();

	bool BuildMenuTree(CString instrumentPath);
	CMenu *GetMenu() const;
	CString GetFile(int Index) const;
	bool ShouldRebuild() const;
	void Changed();

public:
	// Limits, to avoid very deep recursions
	static const int RECURSION_LIMIT = 6;
	static const int MAX_MENUS = 200;

	static const int MENU_BASE = 0x9000;	// Choose a range where no strings are located

	static const int CACHE_TIMEOUT = 60000;	// 1 minute

protected:
	bool ScanDirectory(CString path, CMenu *pMenu, int level);
	void DeleteMenuObjects();

private:
	CMenu *m_pRootMenu;
	int m_iFileIndex;
	CArray<CString, CString> m_fileList;
	CArray<CMenu*, CMenu*> m_menuArray;
	DWORD m_iTimeout;
	bool m_bShouldRebuild;
	int m_iTotalMenusAdded;
};
