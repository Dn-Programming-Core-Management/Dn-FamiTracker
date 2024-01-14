/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2020 Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** Dn-FamiTracker is (C) 2020-2024 D.P.C.M.
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

#include "stdafx.h"
#include "FamiTracker.h"
#include "Accelerator.h"
#include "Settings.h"

/*
 * This class is used to translate key shortcuts -> command messages
 *
 * Translation is now done using win32 functions
 */

// List of modifier strings
LPCTSTR CAccelerator::MOD_NAMES[] = {
	_T("None"), 
	_T("Alt"), 
	_T("Ctrl"), 
	_T("Ctrl+Alt"), 
	_T("Shift"), 
	_T("Shift+Alt"), 
	_T("Shift+Ctrl"), 
	_T("Shift+Ctrl+Alt")
};

// Default shortcut table
const std::vector<stAccelEntry> CAccelerator::DEFAULT_TABLE {
	{_T("Decrease octave"),				MOD_NONE,		VK_DIVIDE,		ID_CMD_OCTAVE_PREVIOUS},
	{_T("Increase octave"),				MOD_NONE,		VK_MULTIPLY,	ID_CMD_OCTAVE_NEXT},
	{_T("Play / Stop"),					MOD_NONE,		VK_RETURN,		ID_TRACKER_TOGGLE_PLAY},
	{_T("Play"),						MOD_NONE,		0,				ID_TRACKER_PLAY},
	{_T("Play from start"),				MOD_NONE,		VK_F5,			ID_TRACKER_PLAY_START},
	{_T("Play from cursor"),			MOD_NONE,		VK_F7,			ID_TRACKER_PLAY_CURSOR},
	{_T("Play from row marker"),		MOD_CONTROL,	VK_F7,			ID_TRACKER_PLAY_MARKER, _T("Play from bookmark")},		// // // 050B
	{_T("Play and loop pattern"),		MOD_NONE,		VK_F6,			ID_TRACKER_PLAYPATTERN},
	{_T("Play row"),					MOD_CONTROL,	VK_RETURN,		ID_TRACKER_PLAYROW},
	{_T("Stop"),						MOD_NONE,		VK_F8,			ID_TRACKER_STOP},
	{_T("Edit enable/disable"),			MOD_NONE,		VK_SPACE,		ID_TRACKER_EDIT},
	{_T("Set row marker"),				MOD_CONTROL,	'B',			ID_TRACKER_SET_MARKER, _T("Set row marker")},		// // // 050B
	{_T("Paste and mix"),				MOD_CONTROL,	'M',			ID_EDIT_PASTEMIX},
	{_T("Paste and overwrite"),			MOD_NONE,		0,				ID_EDIT_PASTEOVERWRITE},			// // //
	{_T("Paste and insert"),			MOD_NONE,		0,				ID_EDIT_PASTEINSERT},				// // //
	{_T("Select all"),					MOD_CONTROL,	'A',			ID_EDIT_SELECTALL},
	{_T("Deselect"),					MOD_NONE,		VK_ESCAPE,		ID_SELECT_NONE},					// // //
	{_T("Select row"),					MOD_NONE,		0,				ID_SELECT_ROW},						// // //
	{_T("Select column"),				MOD_NONE,		0,				ID_SELECT_COLUMN},					// // //
	{_T("Select pattern"),				MOD_NONE,		0,				ID_SELECT_PATTERN},					// // //
	{_T("Select frame"),				MOD_NONE,		0,				ID_SELECT_FRAME},					// // //
	{_T("Select channel"),				MOD_NONE,		0,				ID_SELECT_CHANNEL},					// // //
	{_T("Select track"),				MOD_NONE,		0,				ID_SELECT_TRACK},					// // //
	{_T("Select in other editor"),		MOD_NONE,		0,				ID_SELECT_OTHER},					// // //
	{_T("Go to row"),					MOD_ALT,		'G',			ID_EDIT_GOTO},						// // //
	{_T("Toggle channel"),				MOD_ALT,		VK_F9,			ID_TRACKER_TOGGLECHANNEL},
	{_T("Solo channel"),				MOD_ALT,		VK_F10,			ID_TRACKER_SOLOCHANNEL},
	{_T("Toggle chip"),					MOD_CONTROL|MOD_ALT, VK_F9,		ID_TRACKER_TOGGLECHIP},				// // //
	{_T("Solo chip"),					MOD_CONTROL|MOD_ALT, VK_F10,	ID_TRACKER_SOLOCHIP},				// // //
	{_T("Record to instrument"),		MOD_NONE,		0,				ID_TRACKER_RECORDTOINST},			// // //
	{_T("Interpolate"),					MOD_CONTROL,	'G',			ID_EDIT_INTERPOLATE},
	{_T("Go to next frame"),			MOD_CONTROL,	VK_RIGHT,		ID_NEXT_FRAME},
	{_T("Go to previous frame"),		MOD_CONTROL,	VK_LEFT,		ID_PREV_FRAME},
	{_T("Toggle bookmark"),				MOD_CONTROL,	'K',			ID_BOOKMARKS_TOGGLE},				// // //
	{_T("Next bookmark"),				MOD_CONTROL,	VK_NEXT,		ID_BOOKMARKS_NEXT},					// // //
	{_T("Previous bookmark"),			MOD_CONTROL,	VK_PRIOR,		ID_BOOKMARKS_PREVIOUS},				// // //
	{_T("Transpose, decrease notes"),	MOD_CONTROL,	VK_F1,			ID_TRANSPOSE_DECREASENOTE},
	{_T("Transpose, increase notes"),	MOD_CONTROL,	VK_F2,			ID_TRANSPOSE_INCREASENOTE},
	{_T("Transpose, decrease octaves"),	MOD_CONTROL,	VK_F3,			ID_TRANSPOSE_DECREASEOCTAVE},
	{_T("Transpose, increase octaves"),	MOD_CONTROL,	VK_F4,			ID_TRANSPOSE_INCREASEOCTAVE},
	{_T("Increase pattern"),			MOD_NONE,		VK_ADD,			IDC_FRAME_INC},
	{_T("Decrease pattern"),			MOD_NONE,		VK_SUBTRACT,	IDC_FRAME_DEC},
	{_T("Next instrument"),				MOD_CONTROL,	VK_DOWN,		ID_CMD_NEXT_INSTRUMENT},
	{_T("Previous instrument"),			MOD_CONTROL,	VK_UP,			ID_CMD_PREV_INSTRUMENT},
	{_T("Type instrument number"),		MOD_NONE,		0,				ID_CMD_INST_NUM},					// // //
	{_T("Mask instruments"),			MOD_ALT,		'T',			ID_EDIT_INSTRUMENTMASK},
	{_T("Mask volume"),					MOD_ALT,		'V',			ID_EDIT_VOLUMEMASK},				// // //
	{_T("Edit instrument"),				MOD_CONTROL,	'I',			ID_INSTRUMENT_EDIT},
	{_T("Increase step size"),			MOD_CONTROL,	VK_ADD,			ID_CMD_INCREASESTEPSIZE},
	{_T("Decrease step size"),			MOD_CONTROL,	VK_SUBTRACT,	ID_CMD_DECREASESTEPSIZE},
	{_T("Follow mode"),					MOD_NONE,		VK_SCROLL,		IDC_FOLLOW_TOGGLE},
	{_T("Duplicate frame"),				MOD_CONTROL,	'D',			ID_MODULE_DUPLICATEFRAME},
	{_T("Insert frame"),				MOD_NONE,		0,				ID_MODULE_INSERTFRAME},
	{_T("Remove frame"),				MOD_NONE,		0,				ID_MODULE_REMOVEFRAME},
	{_T("Reverse"),						MOD_CONTROL,	'R',			ID_EDIT_REVERSE},
	{_T("Select frame editor"),			MOD_NONE,		VK_F3,			ID_FOCUS_FRAME_EDITOR},
	{_T("Select pattern editor"),		MOD_NONE,		VK_F2,			ID_FOCUS_PATTERN_EDITOR},
	{_T("Move one step up"),			MOD_ALT,		VK_UP,			ID_CMD_STEP_UP},
	{_T("Move one step down"),			MOD_ALT,		VK_DOWN,		ID_CMD_STEP_DOWN},
	{_T("Replace instrument"),			MOD_ALT,		'S',			ID_EDIT_REPLACEINSTRUMENT},
	{_T("Toggle control panel"),		MOD_NONE,		0,				ID_VIEW_CONTROLPANEL},
	{_T("Display effect list"),			MOD_NONE,		0,				ID_HELP_EFFECTTABLE},
	{_T("Select block start"),			MOD_ALT,		'B',			ID_BLOCK_START},
	{_T("Select block end"),			MOD_ALT,		'E',			ID_BLOCK_END},
	{_T("Pick up row settings"),		MOD_NONE,		0,				ID_POPUP_PICKUPROW},
	{_T("Next song"),					MOD_NONE,		0,				ID_NEXT_SONG},
	{_T("Previous song"),				MOD_NONE,		0,				ID_PREV_SONG},
	{_T("Expand patterns"),				MOD_NONE,		0,				ID_EDIT_EXPANDPATTERNS},
	{_T("Shrink patterns"),				MOD_NONE,		0,				ID_EDIT_SHRINKPATTERNS},
	{_T("Stretch patterns"),			MOD_NONE,		0,				ID_EDIT_STRETCHPATTERNS},			// // //
	{_T("Clone frame"),					MOD_NONE,		0,				ID_MODULE_DUPLICATEFRAMEPATTERNS, _T("Duplicate patterns")},		// // //
	{_T("Clone pattern"),				MOD_ALT,		'D',			ID_MODULE_DUPLICATECURRENTPATTERN, _T("Duplicate current pattern")},	// // //
	{_T("Decrease pattern values"),		MOD_SHIFT,		VK_F1,			ID_DECREASEVALUES},
	{_T("Increase pattern values"),		MOD_SHIFT,		VK_F2,			ID_INCREASEVALUES},
	{_T("Coarse decrease values"),		MOD_SHIFT,		VK_F3,			ID_DECREASEVALUESCOARSE},			// // //
	{_T("Coarse increase values"),		MOD_SHIFT,		VK_F4,			ID_INCREASEVALUESCOARSE},			// // //
	{_T("Toggle find / replace tab"),	MOD_CONTROL,	'F',			ID_EDIT_FIND_TOGGLE},				// // //
	{_T("Find next"),					MOD_NONE,		0,				ID_FIND_NEXT},						// // //
	{_T("Find previous"),				MOD_NONE,		0,				ID_FIND_PREVIOUS},					// // //
	{_T("Recall channel state"),		MOD_NONE,		0,				ID_RECALL_CHANNEL_STATE},			// // //
	{_T("Compact View"),				MOD_NONE,		0,				IDC_COMPACT_TOGGLE},				// // //
};

const int CAccelerator::ACCEL_COUNT = static_cast<int>(DEFAULT_TABLE.size());

// Registry key
LPCTSTR CAccelerator::SHORTCUTS_SECTION = _T("Shortcuts");

// Translate internal modifier -> windows modifier
static BYTE GetMod(int Mod) 
{
	return ((Mod & MOD_CONTROL) ? FCONTROL : 0) | ((Mod & MOD_SHIFT) ? FSHIFT : 0) | ((Mod & MOD_ALT) ? FALT : 0);
}

// Class instance functions

CAccelerator::CAccelerator() : 
	m_pEntriesTable(ACCEL_COUNT),		// // //
	m_pAccelTable(ACCEL_COUNT)		// // //
{
	TRACE(_T("Accelerator: Accelerator table contains %d items\n"), ACCEL_COUNT);
}

CAccelerator::~CAccelerator()
{
	ASSERT(m_hAccel == nullptr);
}

LPCTSTR CAccelerator::GetItemName(int Item) const
{
	return m_pEntriesTable[Item].name;
}

int CAccelerator::GetItemKey(int Item) const
{
	return m_pEntriesTable[Item].key;
}

int CAccelerator::GetItemMod(int Item) const
{
	return m_pEntriesTable[Item].mod;
}

int CAccelerator::GetDefaultKey(int Item) const
{
	return DEFAULT_TABLE[Item].key;
}

int CAccelerator::GetDefaultMod(int Item) const
{
	return DEFAULT_TABLE[Item].mod;
}

LPCTSTR CAccelerator::GetItemModName(int Item) const
{
	return MOD_NAMES[m_pEntriesTable[Item].mod];
}

LPCTSTR CAccelerator::GetItemKeyName(int Item) const
{
	if (m_pEntriesTable[Item].key > 0) {
		return GetVKeyName(m_pEntriesTable[Item].key);
	}

	return _T("None");
}

LPCTSTR CAccelerator::GetVKeyName(int virtualKey) const
{
	unsigned int scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);

	// because MapVirtualKey strips the extended bit for some keys
	switch (virtualKey) {
		case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN: // arrow keys
		case VK_PRIOR: case VK_NEXT: // page up and page down
		case VK_END: case VK_HOME:
		case VK_INSERT: case VK_DELETE:
		case VK_DIVIDE:	 // numpad slash
		case VK_NUMLOCK:
			scanCode |= 0x100; // set extended bit
			break;
	}

	static TCHAR keyName[50];

	if (GetKeyNameText(scanCode << 16, keyName, sizeof(keyName)) != 0)
		return keyName;

	return _T("");
}

void CAccelerator::StoreShortcut(int Item, int Key, int Mod)
{
	m_pEntriesTable[Item].key = Key;
	m_pEntriesTable[Item].mod = Mod;
}

bool CAccelerator::GetShortcutString(int id, CString &str) const
{
	for (const auto &x : m_pEntriesTable) {		// // //
		if (x.id == id) {
			CString KeyName = GetVKeyName(x.key);
			if (KeyName.GetLength() > 1)
				KeyName = KeyName.Mid(0, 1).MakeUpper() + KeyName.Mid(1, KeyName.GetLength() - 1).MakeLower();
			if (x.mod > 0)
				str.Format(_T("\t%s+%s"), MOD_NAMES[x.mod], KeyName);
			else
				str.Format(_T("\t%s"), KeyName);
			return true;
		}
	}

	return false;
}

bool CAccelerator::IsKeyUsed(int nChar) const		// // //
{
	return m_iUsedKeys.count(nChar & 0xFF) > 0;
}

// Registry storage/loading

void CAccelerator::SaveShortcuts(CSettings *pSettings) const
{
	// Save values
	for (const auto &x : m_pEntriesTable) {		// // //
		theApp.WriteProfileInt(SHORTCUTS_SECTION, x.name, (x.mod << 8) | x.key);
	}
}

void CAccelerator::LoadShortcuts(CSettings *pSettings)
{
	// Set up names and default values
	LoadDefaults();

	m_iUsedKeys.clear();		// // //

	// Load custom values, if exists
	/*
		location priorities:
		1. HKCU/SOFTWARE/0CC-FamiTracker
		1. HKCU/SOFTWARE/0CC-FamiTracker, original key
		2. HKCU/SOFTWARE/FamiTracker
		3. HKCU/SOFTWARE/FamiTracker, original key (using stAccelEntry::orig_name)
		4. default value
	*/
	for (auto &x : m_pEntriesTable) {		// // //
		int Setting = (x.mod << 8) | x.key;
		{		// // //
			stOldSettingContext s;
			if (x.orig_name != nullptr)		// // //
				Setting = theApp.GetProfileInt(SHORTCUTS_SECTION, x.orig_name, Setting);
			Setting = theApp.GetProfileInt(SHORTCUTS_SECTION, x.name, Setting);
		}
		if (x.orig_name != nullptr)		// // //
			Setting = theApp.GetProfileInt(SHORTCUTS_SECTION, x.orig_name, Setting);
		Setting = theApp.GetProfileInt(SHORTCUTS_SECTION, x.name, Setting);

		x.key = Setting & 0xFF;
		x.mod = Setting >> 8;
		if (x.mod == MOD_NONE && x.key)		// // //
			m_iUsedKeys.insert(x.key);
	}
}

void CAccelerator::LoadDefaults()
{
	m_pEntriesTable = DEFAULT_TABLE;		// // //
}

void CAccelerator::Setup()
{
	// Translate key table -> windows accelerator table

	if (m_hAccel) {
		DestroyAcceleratorTable(m_hAccel);
		m_hAccel = NULL;
	}

	m_pAccelTable.clear();		// // //
	for (const auto &x : m_pEntriesTable) {
		ACCEL a;
		a.fVirt = FVIRTKEY | GetMod(x.mod);
		a.key = x.key;
		a.cmd = x.id;
		m_pAccelTable.push_back(a);
	}
	m_hAccel = CreateAcceleratorTable(m_pAccelTable.data(), ACCEL_COUNT);
}

BOOL CAccelerator::Translate(HWND hWnd, MSG *pMsg)
{
	if (m_hAdditionalAccel) {
		if (TranslateAccelerator(hWnd, m_hAdditionalAccel, pMsg)) {
			return TRUE;
		}
	}

	if (m_hAccel) {
		if (TranslateAccelerator(hWnd, m_hAccel, pMsg)) {
			return TRUE;
		}
	}

	return FALSE;
}

void CAccelerator::Shutdown()
{
	if (m_hAccel) {
		DestroyAcceleratorTable(m_hAccel);
		m_hAccel = NULL;
	}
}

void CAccelerator::SetAccelerator(HACCEL hAccel)
{
	m_hAdditionalAccel = hAccel;
}
