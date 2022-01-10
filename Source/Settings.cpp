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

/*
 *  Add new program settings to the SetupSettings function,
 *  three macros are provided for the type of setting you want to add.
 *  (SETTING_INT, SETTING_BOOL, SETTING_STRING)
 *
 */

#include "stdafx.h"
#include "FamiTracker.h"
#include "Settings.h"
#include "ColorScheme.h"

// // // registry context

stOldSettingContext::stOldSettingContext()
{
	free((void*)theApp.m_pszProfileName);
	theApp.m_pszProfileName = _T("FamiTracker");
}

stOldSettingContext::~stOldSettingContext()
{
	CString s;
	s.LoadString(AFX_IDS_APP_TITLE);
	theApp.m_pszProfileName = _tcsdup(s);
}

// CSettings

CSettings* CSettings::GetObject()
{
	static CSettings Object;
	return &Object;
}

CSettings::CSettings() : m_iAddedSettings(0)
{
	memset(m_pSettings, 0, sizeof(CSettingBase*) * MAX_SETTINGS);
	SetupSettings();
	TRACE(_T("Settings: Added %i settings\n"), m_iAddedSettings);
}

CSettings::~CSettings()
{
	// Release all settings
	for (int i = 0; i < m_iAddedSettings; ++i) {
		SAFE_RELEASE(m_pSettings[i]);
	}
}

void CSettings::SetupSettings()
{
	//
	// This function defines all settings in the program that are stored in registry
	// All settings are loaded on program start and saved when closing the program
	//

	// The SETTING macros takes four arguments: 
	//
	//  1. Registry section
	//  2. Registry key name
	//  3. Default value
	//  4. A variable that contains the setting, loaded on program startup and saved on shutdown
	//

	const auto SETTING_INT = [&] (LPCTSTR Section, LPCTSTR Entry, int Default, int *Variable) {		// // //
		return AddSetting<int>(_T(Section), _T(Entry), Default, Variable);
	};
	const auto SETTING_BOOL = [&] (LPCTSTR Section, LPCTSTR Entry, bool Default, bool *Variable) {
		return AddSetting<bool>(_T(Section), _T(Entry), Default, Variable);
	};
	const auto SETTING_STRING = [&] (LPCTSTR Section, LPCTSTR Entry, CString Default, CString *Variable) {
		return AddSetting<CString>(_T(Section), _T(Entry), Default, Variable);
	};

	stOldSettingContext s;		// // //
	/*
		location priorities:
		1. HKCU/SOFTWARE/0CC-FamiTracker
		2. HKCU/SOFTWARE/0CC-FamiTracker, original key (using UpdateDefault)
		3. HKCU/SOFTWARE/FamiTracker
		4. HKCU/SOFTWARE/FamiTracker, original key (using UpdateDefault)
		5. Default value
	*/

	// General
	SETTING_INT("General", "Edit style", EDIT_STYLE_FT2, &General.iEditStyle);
	SETTING_INT("General", "Page step size", 4, &General.iPageStepSize);
	SETTING_BOOL("General", "Wrap cursor", true, &General.bWrapCursor);
	SETTING_BOOL("General", "Wrap across frames", true, &General.bWrapFrames);
	SETTING_BOOL("General", "Free cursor edit",	false, &General.bFreeCursorEdit);
	SETTING_BOOL("General", "Wave preview", true, &General.bWavePreview);
	SETTING_BOOL("General", "Key repeat", true, &General.bKeyRepeat);
	SETTING_BOOL("General", "Hex row display", true, &General.bRowInHex);
	SETTING_BOOL("General", "Frame preview", true, &General.bFramePreview);
	SETTING_BOOL("General", "No DPCM reset", false, &General.bNoDPCMReset);
	SETTING_BOOL("General", "No Step moving", false, &General.bNoStepMove);
	SETTING_BOOL("General", "Delete pull up", false, &General.bPullUpDelete);
	SETTING_BOOL("General", "Backups", false, &General.bBackups);
	SETTING_BOOL("General", "Single instance", false, &General.bSingleInstance);
	SETTING_BOOL("General", "Preview full row", false, &General.bPreviewFullRow);
	SETTING_BOOL("General", "Double click selection", false, &General.bDblClickSelect);
	// // //
	SETTING_BOOL("General", "Wrap pattern values", false, &General.bWrapPatternValue);
	SETTING_BOOL("General", "Cut sub-volume", false, &General.bCutVolume);
	SETTING_BOOL("General", "Use old FDS volume table", false, &General.bFDSOldVolume);
	SETTING_BOOL("General", "Retrieve channel state (on by default in Dn-FT)", true, &General.bRetrieveChanState);
	SETTING_BOOL("General", "Overflow paste mode", false, &General.bOverflowPaste);
	SETTING_BOOL("General", "Show skipped rows", false, &General.bShowSkippedRows);
	SETTING_BOOL("General", "Hexadecimal keypad", false, &General.bHexKeypad);
	SETTING_BOOL("General", "Multi-frame selection", false, &General.bMultiFrameSel);
	SETTING_BOOL("General", "Check for new versions", true, &General.bCheckVersion);

	// GUI
	SETTING_INT("GUI", "Idle refresh rate", 100, &GUI.iLowRefreshRate);
	SETTING_INT("GUI", "Maximum frame editor channel view", 28, &GUI.iMaxChannelView);		// // !!
	SETTING_BOOL("GUI", "Smooth register frequency view", true, &GUI.bSmoothRegFreq);

	// // // Version / Compatibility info
	SETTING_INT("Version", "Module error level", MODULE_ERROR_DEFAULT, &Version.iErrorLevel);

	// Keys
	SETTING_INT("Keys", "Note cut",		0x31, &Keys.iKeyNoteCut);
	SETTING_INT("Keys", "Note release", 0xDC, &Keys.iKeyNoteRelease);
	SETTING_INT("Keys", "Clear field",	0xBD, &Keys.iKeyClear);
	SETTING_INT("Keys", "Repeat",		0x00, &Keys.iKeyRepeat);
	SETTING_INT("Keys", "Echo buffer",	0x00, &Keys.iKeyEchoBuffer);		// // //

	// Sound
	SETTING_INT("Sound", "Audio Device", 0, &Sound.iDevice);
	SETTING_INT("Sound", "Sample rate",	44100, &Sound.iSampleRate);
	SETTING_INT("Sound", "Sample size", 16, &Sound.iSampleSize);
	SETTING_INT("Sound", "Buffer length", 40, &Sound.iBufferLength);
	SETTING_INT("Sound", "Bass filter freq", 30, &Sound.iBassFilter);
	SETTING_INT("Sound", "Treble filter freq", 12000, &Sound.iTrebleFilter);
	SETTING_INT("Sound", "Treble filter damping", 24, &Sound.iTrebleDamping);
	SETTING_INT("Sound", "Volume", 100, &Sound.iMixVolume);

	// Midi
	SETTING_INT("MIDI", "Device", 0, &Midi.iMidiDevice);
	SETTING_INT("MIDI", "Out Device", 0, &Midi.iMidiOutDevice);
	SETTING_BOOL("MIDI", "Master sync", false, &Midi.bMidiMasterSync);
	SETTING_BOOL("MIDI", "Key release", false, &Midi.bMidiKeyRelease);
	SETTING_BOOL("MIDI", "Channel map", false, &Midi.bMidiChannelMap);
	SETTING_BOOL("MIDI", "Velocity control", false,	&Midi.bMidiVelocity);
	SETTING_BOOL("MIDI", "Auto Arpeggio", false, &Midi.bMidiArpeggio);

	// Appearance
	SETTING_INT("Appearance", "Background", DEFAULT_COLOR_SCHEME.BACKGROUND, &Appearance.iColBackground);
	SETTING_INT("Appearance", "Background highlighted", DEFAULT_COLOR_SCHEME.BACKGROUND_HILITE, &Appearance.iColBackgroundHilite);
	SETTING_INT("Appearance", "Background highlighted 2", DEFAULT_COLOR_SCHEME.BACKGROUND_HILITE2, &Appearance.iColBackgroundHilite2);
	SETTING_INT("Appearance", "Pattern text", DEFAULT_COLOR_SCHEME.TEXT_NORMAL, &Appearance.iColPatternText);
	SETTING_INT("Appearance", "Pattern text highlighted", DEFAULT_COLOR_SCHEME.TEXT_HILITE, &Appearance.iColPatternTextHilite);
	SETTING_INT("Appearance", "Pattern text highlighted 2", DEFAULT_COLOR_SCHEME.TEXT_HILITE2, &Appearance.iColPatternTextHilite2);
	SETTING_INT("Appearance", "Pattern instrument", DEFAULT_COLOR_SCHEME.TEXT_INSTRUMENT, &Appearance.iColPatternInstrument);
	SETTING_INT("Appearance", "Pattern volume", DEFAULT_COLOR_SCHEME.TEXT_VOLUME, &Appearance.iColPatternVolume);
	SETTING_INT("Appearance", "Pattern effect", DEFAULT_COLOR_SCHEME.TEXT_EFFECT, &Appearance.iColPatternEffect);
	SETTING_INT("Appearance", "Selection", DEFAULT_COLOR_SCHEME.SELECTION, &Appearance.iColSelection);
	SETTING_INT("Appearance", "Cursor", DEFAULT_COLOR_SCHEME.CURSOR, &Appearance.iColCursor);

	SETTING_INT("Appearance", "Current row (normal mode)", DEFAULT_COLOR_SCHEME.ROW_NORMAL, &Appearance.iColCurrentRowNormal);
	SETTING_INT("Appearance", "Current row (edit mode)", DEFAULT_COLOR_SCHEME.ROW_EDIT, &Appearance.iColCurrentRowEdit);
	SETTING_INT("Appearance", "Current row (playing)", DEFAULT_COLOR_SCHEME.ROW_PLAYING, &Appearance.iColCurrentRowPlaying);

	SETTING_STRING("Appearance", "Pattern font", FONT_FACE, &Appearance.strFont)
		->UpdateDefault("General", "Pattern font");
	SETTING_INT("Appearance", "Pattern font size", FONT_SIZE, &Appearance.rowHeight)
		->UpdateDefault("General", "Pattern font size");
	SETTING_INT("Appearance", "Pattern font size percentage", FONT_RATIO, &Appearance.fontPercent);

	SETTING_STRING("Appearance", "Frame font", FONT_FACE, &Appearance.strFrameFont)
		->UpdateDefault("General", "Frame font");		// // // 050B
	SETTING_BOOL("Appearance", "Pattern colors", true, &Appearance.bPatternColor)
		->UpdateDefault("Appearance", "Pattern colors");
	SETTING_BOOL("Appearance", "Display flats", false, &Appearance.bDisplayFlats)
		->UpdateDefault("Appearance", "Display flats");
	
	// Window position
	SETTING_INT("Window position", "Left", 100, &WindowPos.iLeft);
	SETTING_INT("Window position", "Top", 100, &WindowPos.iTop);
	SETTING_INT("Window position", "Right",	950, &WindowPos.iRight);
	SETTING_INT("Window position", "Bottom", 920, &WindowPos.iBottom);
	SETTING_INT("Window position", "State",	STATE_NORMAL, &WindowPos.iState);

	// Display
	SETTING_BOOL("Display", "Average BPM", false, &Display.bAverageBPM);		// // // 050B
	SETTING_BOOL("Display", "Register state", false, &Display.bRegisterState);		// // // 050B

	// Other
	SETTING_INT("Other", "Sample window state", 0, &SampleWinState);
	SETTING_INT("Other", "Frame editor position", 0, &FrameEditPos);
	SETTING_INT("Other", "Control panel position", 0, &ControlPanelPos);		// // // 050B todo
	SETTING_INT("Other", "Frame editor channel count", 5, &ChannelViewCount);		// // !!
	SETTING_BOOL("Other", "Follow mode", true, &FollowMode);
	SETTING_BOOL("Other", "Meter decay rate", false, &MeterDecayRate);		// // // 050B

	// Paths
	SETTING_STRING("Paths", "FTM path", "", &Paths[PATH_FTM]);
	SETTING_STRING("Paths", "FTI path", "", &Paths[PATH_FTI]);
	SETTING_STRING("Paths", "DMC path", "", &Paths[PATH_DMC]);
	SETTING_STRING("Paths", "WAV path", "", &Paths[PATH_WAV_IMPORT]);
	
	//SETTING_STRING("Paths", "NSF path", "", &Paths[PATH_NSF]);
	//SETTING_STRING("Paths", "TXT Export path", "", &Paths[PATH_EXPORT]);

	SETTING_STRING("Paths", "Instrument menu", "", &InstrumentMenuPath);

	// Mixing
	SETTING_INT("Mixer", "APU1", 0, &ChipLevels.iLevelAPU1);
	SETTING_INT("Mixer", "APU2", 0, &ChipLevels.iLevelAPU2);
	SETTING_INT("Mixer", "VRC6", 0, &ChipLevels.iLevelVRC6);
	SETTING_INT("Mixer", "VRC7", 0, &ChipLevels.iLevelVRC7);
	SETTING_INT("Mixer", "MMC5", 0, &ChipLevels.iLevelMMC5);
	SETTING_INT("Mixer", "FDS", 0, &ChipLevels.iLevelFDS);
	SETTING_INT("Mixer", "N163", 0, &ChipLevels.iLevelN163);
	SETTING_INT("Mixer", "S5B", 0, &ChipLevels.iLevelS5B);

	// Emulation
		// FDS
	SETTING_INT("Emulation", "FDS lowpass filter cutoff", 2000, &Emulation.iFDSLowpass);
		// N163
	SETTING_BOOL("Emulation", "N163 multiplexing", true, &Emulation.bNamcoMixing);
		// VRC7
	SETTING_INT("Emulation", "VRC7 hardware patch", PATCH_NUKE, &Emulation.iVRC7Patch);
}

template<class T>
CSettingBase *CSettings::AddSetting(LPCTSTR pSection, LPCTSTR pEntry, T tDefault, T *pVariable)
{
	ASSERT(m_iAddedSettings < MAX_SETTINGS);
	return m_pSettings[m_iAddedSettings++] = new CSettingType<T>(pSection, pEntry, tDefault, pVariable);		// // //
}

// CSettings member functions

void CSettings::LoadSettings()
{
	for (int i = 0; i < m_iAddedSettings; ++i) {
		m_pSettings[i]->Load();
	}
}

void CSettings::SaveSettings()
{
	for (int i = 0; i < m_iAddedSettings; ++i) {
		m_pSettings[i]->Save();
	}
}

void CSettings::DefaultSettings()
{
	for (int i = 0; i < m_iAddedSettings; ++i) {
		m_pSettings[i]->Default();
	}
}

void CSettings::DeleteSettings()
{
	// Delete all settings from registry
	HKEY hKey = theApp.GetAppRegistryKey();
	theApp.DelRegTree(hKey, _T(""));
}

void CSettings::SetWindowPos(int Left, int Top, int Right, int Bottom, int State)
{
	WindowPos.iLeft = Left;
	WindowPos.iTop = Top;
	WindowPos.iRight = Right;
	WindowPos.iBottom = Bottom;
	WindowPos.iState = State;
}

CString CSettings::GetPath(unsigned int PathType) const
{
	ASSERT(PathType < PATH_COUNT);
	return Paths[PathType];
}

void CSettings::SetPath(CString PathName, unsigned int PathType)
{
	ASSERT(PathType < PATH_COUNT);

	// Remove file name if there is a
	if (PathName.Right(1) == _T("\\") || PathName.Find(_T('\\')) == -1)
		Paths[PathType] = PathName;
	else
		Paths[PathType] = PathName.Left(PathName.ReverseFind(_T('\\')));
}

// Settings types

template<class T>
void CSettingType<T>::Load()
{
	T Value = m_tDefaultValue;		// // //
	{
		stOldSettingContext s;
		if (m_pSectionSecond)
			Value = theApp.GetProfileInt(m_pSectionSecond, m_pEntrySecond, Value);
		Value = theApp.GetProfileInt(m_pSection, m_pEntry, Value);
	}
	if (m_pSectionSecond)
		Value = theApp.GetProfileInt(m_pSectionSecond, m_pEntrySecond, Value);
	*m_pVariable = theApp.GetProfileInt(m_pSection, m_pEntry, Value);
}

template<>
void CSettingType<bool>::Load()
{
	bool Value = m_tDefaultValue;		// // //
	{
		stOldSettingContext s;
		if (m_pSectionSecond)
			Value = theApp.GetProfileInt(m_pSectionSecond, m_pEntrySecond, Value) != 0;
		Value = theApp.GetProfileInt(m_pSection, m_pEntry, Value) != 0;
	}
	if (m_pSectionSecond)
		Value = theApp.GetProfileInt(m_pSectionSecond, m_pEntrySecond, Value) != 0;
	*m_pVariable = theApp.GetProfileInt(m_pSection, m_pEntry, Value) != 0;
}

template<>
void CSettingType<CString>::Load()
{
	CString Value = m_tDefaultValue;		// // //
	{
		stOldSettingContext s;
		if (m_pSectionSecond)
			Value = theApp.GetProfileString(m_pSectionSecond, m_pEntrySecond, Value);
		Value = theApp.GetProfileString(m_pSection, m_pEntry, Value);
	}
	if (m_pSectionSecond)
		Value = theApp.GetProfileString(m_pSectionSecond, m_pEntrySecond, Value);
	*m_pVariable = theApp.GetProfileString(m_pSection, m_pEntry, Value);
}

template<class T>
void CSettingType<T>::Save()
{
	theApp.WriteProfileInt(m_pSection, m_pEntry, *m_pVariable);
}

template<>
void CSettingType<CString>::Save()
{
	theApp.WriteProfileString(m_pSection, m_pEntry, *m_pVariable);
}

template<class T>
void CSettingType<T>::Default()
{
	*m_pVariable = m_tDefaultValue;
}

void CSettingBase::UpdateDefault(LPCTSTR pSection, LPCTSTR pEntry)		// // //
{
	m_pSectionSecond = pSection;
	m_pEntrySecond = pEntry;
}
