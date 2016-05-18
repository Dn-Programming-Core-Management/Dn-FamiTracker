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


// Key accelerator class

#include <unordered_set>		// // //

#define MOD_NONE 0

struct stAccelEntry {
	LPCTSTR name;
	int	mod;
	int	key;
	int	id;
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
	static const stAccelEntry DEFAULT_TABLE[];						// List of default shortcuts
	static const int		  ACCEL_COUNT;							// Number of shortcuts
	static LPCTSTR			  SHORTCUTS_SECTION;					// Registry section

private:
	HACCEL	m_hAccel;
	HACCEL	m_hAdditionalAccel;

	// Shortcut table
	stAccelEntry *m_pEntriesTable;

	// Accelerator table
	ACCEL	*m_pAccelTable;

	// // // Used keys
	std::unordered_set<int> m_iUsedKeys;
};
