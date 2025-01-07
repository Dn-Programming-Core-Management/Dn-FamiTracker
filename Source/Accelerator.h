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


// Key accelerator class

#include <vector>		// // //
#include <unordered_set>		// // //

#define MOD_NONE 0

struct stAccelEntry {
	LPCTSTR name;
	int	mod;
	int	key;
	int	id;
	LPCTSTR orig_name;		// // //
};

class CAccelerator {
public:
	CAccelerator();
	~CAccelerator();

	LPCTSTR			GetItemName(int Item) const;					// Name of shortcut
	int				GetItemKey(int Item) const;						// Key for shortcut
	int				GetItemMod(int Item) const;						// Modifier for shortcut
	int				GetDefaultKey(int Item) const;					// Default key for shortcut
	int				GetDefaultMod(int Item) const;					// Default modifier for shortcut
	LPCTSTR			GetItemModName(int Item) const;					// Key string for shortcut
	LPCTSTR			GetItemKeyName(int Item) const;					// Modifier string for shortcut
	LPCTSTR			GetVKeyName(int virtualKey) const;				// Translates virtual key to a string
	void			StoreShortcut(int Item, int Key, int Mod);		// Store key and modifier for shortcut

	void			SaveShortcuts(CSettings *pSettings) const;		// Save to registry
	void			LoadShortcuts(CSettings *pSettings);			// Load from registry
	void			LoadDefaults();									// Load defaults

	void			Setup();
	void			Shutdown();
	BOOL			Translate(HWND hWnd, MSG *pMsg);
	void			SetAccelerator(HACCEL hAccel);

	bool			GetShortcutString(int id, CString &str) const;

	// // // check if key is used as a modifier-less shortcut
	bool			IsKeyUsed(int nChar) const;

public:
	// Class member constants
	static LPCTSTR			  MOD_NAMES[];							// Strings for modifiers
	static const std::vector<stAccelEntry> DEFAULT_TABLE;			// // // List of default shortcuts
	static const int		  ACCEL_COUNT;							// Number of shortcuts
	static LPCTSTR			  SHORTCUTS_SECTION;					// Registry section

private:
	HACCEL	m_hAccel = nullptr;
	HACCEL	m_hAdditionalAccel = nullptr;

	// Shortcut table
	std::vector<stAccelEntry> m_pEntriesTable;		// // //

	// Accelerator table
	std::vector<ACCEL> m_pAccelTable;		// // //

	// // // Used keys
	std::unordered_set<int> m_iUsedKeys;
};
