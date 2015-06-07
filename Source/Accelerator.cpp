/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2015 HertzDevil
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
#include "FamiTracker.h"
#include "accelerator.h"
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
const stAccelEntry CAccelerator::DEFAULT_TABLE[] = {
	{_T("Decrease octave"),				MOD_NONE,		VK_DIVIDE,		ID_CMD_OCTAVE_PREVIOUS},
	{_T("Increase octave"),				MOD_NONE,		VK_MULTIPLY,	ID_CMD_OCTAVE_NEXT},
	{_T("Play / Stop"),					MOD_NONE,		VK_RETURN,		ID_TRACKER_TOGGLE_PLAY},
	{_T("Play"),						MOD_NONE,		0,				ID_TRACKER_PLAY},
	{_T("Play from start"),				MOD_NONE,		VK_F5,			ID_TRACKER_PLAY_START},
	{_T("Play from cursor"),			MOD_NONE,		VK_F7,			ID_TRACKER_PLAY_CURSOR},
	{_T("Play and loop pattern"),		MOD_NONE,		VK_F6,			ID_TRACKER_PLAYPATTERN},
	{_T("Play row"),					MOD_CONTROL,	VK_RETURN,		ID_TRACKER_PLAYROW},
	{_T("Stop"),						MOD_NONE,		VK_F8,			ID_TRACKER_STOP},
	{_T("Edit enable/disable"),			MOD_NONE,		VK_SPACE,		ID_TRACKER_EDIT},
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
	{_T("Go to row"),					MOD_ALT,		'G',			ID_EDIT_GOTO},						// // //
	{_T("Toggle channel"),				MOD_ALT,		VK_F9,			ID_TRACKER_TOGGLECHANNEL},
	{_T("Solo channel"),				MOD_ALT,		VK_F10,			ID_TRACKER_SOLOCHANNEL},
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
	{_T("Duplicate patterns"),			MOD_NONE,		0,				ID_MODULE_DUPLICATEFRAMEPATTERNS},
	{_T("Duplicate current pattern"),	MOD_ALT,		'D',			ID_MODULE_DUPLICATECURRENTPATTERN},	// // //
	{_T("Decrease pattern values"),		MOD_SHIFT,		VK_F1,			ID_DECREASEVALUES},
	{_T("Increase pattern values"),		MOD_SHIFT,		VK_F2,			ID_INCREASEVALUES},
	{_T("Coarse decrease values"),		MOD_SHIFT,		VK_F3,			ID_DECREASEVALUESCOARSE},			// // //
	{_T("Coarse increase values"),		MOD_SHIFT,		VK_F4,			ID_INCREASEVALUESCOARSE},			// // //
	{_T("Toggle find / replace tab"),	MOD_CONTROL,	'F',			ID_EDIT_FIND_TOGGLE},				// // //
	{_T("Find next"),					MOD_NONE,		0,				ID_FIND_NEXT},						// // //
	{_T("Recall channel state"),		MOD_NONE,		0,				ID_RECALL_CHANNEL_STATE},			// // //
	{_T("Compact View"),				MOD_NONE,		0,				IDC_COMPACT_TOGGLE},				// // //
};

const int CAccelerator::ACCEL_COUNT = sizeof(CAccelerator::DEFAULT_TABLE) / sizeof(stAccelEntry);

// Registry key
LPCTSTR CAccelerator::SHORTCUTS_SECTION = _T("Shortcuts");

// Translate internal modifier -> windows modifier
static BYTE GetMod(int Mod) 
{
	return ((Mod & MOD_CONTROL) ? FCONTROL : 0) | ((Mod & MOD_SHIFT) ? FSHIFT : 0) | ((Mod & MOD_ALT) ? FALT : 0);
}

// Class instance functions

CAccelerator::CAccelerator() : 
	m_hAccel(NULL), 
	m_hAdditionalAccel(NULL), 
	m_pEntriesTable(new stAccelEntry[ACCEL_COUNT]), 
	m_pAccelTable(new ACCEL[ACCEL_COUNT])
{
	ATLTRACE2(atlTraceGeneral, 0, "Accelerator: Accelerator table contains %d items\n", ACCEL_COUNT);
}

CAccelerator::~CAccelerator()
{
	ASSERT(m_hAccel == NULL);
	SAFE_RELEASE_ARRAY(m_pEntriesTable);
	SAFE_RELEASE_ARRAY(m_pAccelTable);
}

LPCTSTR CAccelerator::GetItemName(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return m_pEntriesTable[Item].name;
}

int CAccelerator::GetItemKey(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return m_pEntriesTable[Item].key;
}

int CAccelerator::GetItemMod(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return m_pEntriesTable[Item].mod;
}

int CAccelerator::GetDefaultKey(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return DEFAULT_TABLE[Item].key;
}

int CAccelerator::GetDefaultMod(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
	return DEFAULT_TABLE[Item].mod;
}

LPCTSTR CAccelerator::GetItemModName(int Item) const
{
	ASSERT(Item < ACCEL_COUNT);
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
	ASSERT(Item < ACCEL_COUNT);
	m_pEntriesTable[Item].key = Key;
	m_pEntriesTable[Item].mod = Mod;
}

bool CAccelerator::GetShortcutString(int id, CString &str) const
{
	for (int i = 0; i < ACCEL_COUNT; ++i) {
		if (m_pEntriesTable[i].id == id) {
			CString KeyName = GetVKeyName(m_pEntriesTable[i].key);
			if (KeyName.GetLength() > 1)
				KeyName = KeyName.Mid(0, 1).MakeUpper() + KeyName.Mid(1, KeyName.GetLength() - 1).MakeLower();
			if (m_pEntriesTable[i].mod > 0)
				str.Format(_T("\t%s+%s"), MOD_NAMES[m_pEntriesTable[i].mod], KeyName);
			else
				str.Format(_T("\t%s"), KeyName);
			return true;
		}
	}

	return false;
}

// Registry storage/loading

void CAccelerator::SaveShortcuts(CSettings *pSettings) const
{
	// Save values
	for (int i = 0; i < ACCEL_COUNT; ++i) {
		pSettings->StoreSetting(SHORTCUTS_SECTION, m_pEntriesTable[i].name, (m_pEntriesTable[i].mod << 8) | m_pEntriesTable[i].key);
	}
}

void CAccelerator::LoadShortcuts(CSettings *pSettings)
{
	// Set up names and default values
	LoadDefaults();

	// Load custom values, if exists
	for (int i = 0; i < ACCEL_COUNT; ++i) {
		int Default = (m_pEntriesTable[i].mod << 8) | m_pEntriesTable[i].key;
		int Setting = pSettings->LoadSetting(SHORTCUTS_SECTION, m_pEntriesTable[i].name, Default);
		m_pEntriesTable[i].key = Setting & 0xFF;
		m_pEntriesTable[i].mod = Setting >> 8;
	}
}

void CAccelerator::LoadDefaults()
{
	memcpy(m_pEntriesTable, DEFAULT_TABLE, sizeof(stAccelEntry) * ACCEL_COUNT);
}

void CAccelerator::Setup()
{
	// Translate key table -> windows accelerator table

	if (m_hAccel) {
		DestroyAcceleratorTable(m_hAccel);
		m_hAccel = NULL;
	}

	memset(m_pAccelTable, 0, sizeof(ACCEL) * ACCEL_COUNT);

	for (int i = 0; i < ACCEL_COUNT; ++i) {
		m_pAccelTable[i].cmd = m_pEntriesTable[i].id;
		m_pAccelTable[i].fVirt = FVIRTKEY | GetMod(m_pEntriesTable[i].mod);
		m_pAccelTable[i].key = m_pEntriesTable[i].key;
	}

	m_hAccel = CreateAcceleratorTable(m_pAccelTable, ACCEL_COUNT);
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
