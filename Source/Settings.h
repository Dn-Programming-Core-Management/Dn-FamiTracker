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


// CSettings command target

enum EDIT_STYLES {		// // // renamed
	EDIT_STYLE_FT2 = 0,		// FT2
	EDIT_STYLE_MPT = 1,		// ModPlug
	EDIT_STYLE_IT = 2,		// IT
	EDIT_STYLE_FT2_JP = 3,	// FT2-JP106
};

enum module_error_level_t {		// // //
	MODULE_ERROR_NONE,		/*!< No error checking at all (warning) */
	MODULE_ERROR_DEFAULT,	/*!< Usual error checking */
	MODULE_ERROR_STRICT,	/*!< Extra validation for some values */
	MODULE_ERROR_MAX = MODULE_ERROR_STRICT,
};

enum WIN_STATES {
	STATE_NORMAL,
	STATE_MAXIMIZED
};

enum PATHS {
	PATH_FTM,
	PATH_FTI,
	PATH_DMC,
	PATH_WAV_IMPORT,
	PATH_COUNT,

	PATH_NSF = PATH_FTM,
	PATH_EXPORT = PATH_FTM
};

// // !! VRC7 hardware patches
enum VRC7_PATCH {
	PATCH_NUKE = 0,		// OPLL_VRC7_TONE
	PATCH_RW = 1,		// OPLL_VRC7_RW_TONE
	PATCH_FT36 = 2,		// OPLL_VRC7_FT36_TONE
	PATCH_FT35 = 3,		// OPLL_VRC7_FT35_TONE
	PATCH_MO = 4,		// OPLL_VRC7_MO_TONE
	PATCH_KT2 = 5,		// OPLL_VRC7_KT2_TONE
	PATCH_KT1 = 6,		// OPLL_VRC7_KT1_TONE
	PATCH_2413 = 7,		// OPLL_2413_TONE
	PATCH_281B = 8		// OPLL_281B_TONE
};

// // // helper class for loading settings from official famitracker
struct stOldSettingContext
{
	stOldSettingContext();
	~stOldSettingContext();
};

// Base class for settings, pure virtual
class CSettingBase {
public:
	CSettingBase(LPCTSTR pSection, LPCTSTR pEntry) : m_pSection(pSection), m_pEntry(pEntry) {};
	virtual ~CSettingBase() {}
	virtual void Load() = 0;
	virtual void Save() = 0;
	virtual void Default() = 0;
	virtual void UpdateDefault(LPCTSTR pSection, LPCTSTR pEntry);		// // /
	LPCTSTR GetSection() const { return m_pSection; };
protected:
	LPCTSTR m_pSection;
	LPCTSTR m_pEntry;
	LPCTSTR m_pSectionSecond = nullptr;		// // //
	LPCTSTR m_pEntrySecond = nullptr;		// // //
};

// Templated setting class
template <class T>
class CSettingType : public CSettingBase {
public:
	CSettingType(LPCTSTR pSection, LPCTSTR pEntry, T defaultVal, T *pVar) : CSettingBase(pSection, pEntry), m_tDefaultValue(defaultVal), m_pVariable(pVar) {};
	virtual void Load();
	virtual void Save();
	virtual void Default();
protected:
	T *m_pVariable;
	T m_tDefaultValue;
};

// Settings collection
class CSettings : public CObject
{
private:
	CSettings();

public:
	virtual ~CSettings();

	void	LoadSettings();
	void	SaveSettings();
	void	DefaultSettings();
	void	DeleteSettings();
	void	SetWindowPos(int Left, int Top, int Right, int Bottom, int State);

	CString GetPath(unsigned int PathType) const;
	void	SetPath(CString PathName, unsigned int PathType);

public:
	static CSettings* GetObject();

public:
	// Local cache of all settings (all public)

	struct {
		bool	bWrapCursor;
		bool	bWrapFrames;
		bool	bFreeCursorEdit;
		bool	bWavePreview;
		bool	bKeyRepeat;
		bool	bRowInHex;
		bool	bFramePreview;
		int		iEditStyle;
		bool	bNoDPCMReset;
		bool	bNoStepMove;
		int		iPageStepSize;
		bool	bPullUpDelete;
		bool	bBackups;
		bool	bSingleInstance;
		bool	bPreviewFullRow;
		bool	bDblClickSelect;
		bool	bWrapPatternValue;		// // //
		bool	bCutVolume;
		bool	bFDSOldVolume;
		bool	bRetrieveChanState;
		bool	bOverflowPaste;
		bool	bShowSkippedRows;
		bool	bHexKeypad;
		bool	bMultiFrameSel;
		bool	bCheckVersion;		// // //
	} General;

	struct {
		int		iLowRefreshRate;	// // !!
		int		iMaxChannelView;	// // !!
		bool	bPreciseRegPitch;
	} GUI;

	struct {
		int		iErrorLevel;
	} Version;		// // //

	struct {
		int		iDevice;
		int		iSampleRate;
		int		iBufferLength;
		int		iBassFilter;
		int		iTrebleFilter;
		int		iTrebleDamping;
		int		iMixVolume;
	} Sound;

	struct {
		int		iMidiDevice;
		int		iMidiOutDevice;
		bool	bMidiMasterSync;
		bool	bMidiKeyRelease;
		bool	bMidiChannelMap;
		bool	bMidiVelocity;
		bool	bMidiArpeggio;
	} Midi;

	struct {
		int		iColBackground;
		int		iColBackgroundHilite;
		int		iColBackgroundHilite2;
		int		iColPatternText;
		int		iColPatternTextHilite;
		int		iColPatternTextHilite2;
		int		iColPatternInstrument;
		int		iColPatternVolume;
		int		iColPatternEffect;
		int		iColSelection;
		int		iColCursor;
		int		iColCurrentRowNormal;		// // //
		int		iColCurrentRowEdit;
		int		iColCurrentRowPlaying;

		CString	strFont;		// // //
		CString	strFrameFont;		// // // 050B
		int		rowHeight;
		int		fontPercent;	// Font height (pixels), as a percentage of row height

		bool	bPatternColor;
		bool	bDisplayFlats;
	} Appearance;

	struct {
		int		iLeft;
		int		iTop;
		int		iRight;
		int		iBottom;
		int		iState;
	} WindowPos;

	struct {
		int		iKeyNoteCut;
		int		iKeyNoteRelease;
		int		iKeyClear;
		int		iKeyRepeat;
		int		iKeyEchoBuffer;		// // //
	} Keys;

	struct {
		bool	bAverageBPM;
		bool	bRegisterState;
	} Display;		// // // 050B

	// Other
	int SampleWinState;
	int FrameEditPos;
	int ControlPanelPos;		// // // 050B
	int ChannelViewCount;		// // !!
	bool FollowMode;
	bool MeterDecayRate;		// // // 050B

	struct {
		int		iLevelAPU1;
		int		iLevelAPU2;
		int		iLevelVRC6;
		int		iLevelVRC7;
		int		iLevelFDS;
		int		iLevelMMC5;
		int		iLevelN163;
		int		iLevelS5B;
		int		iSurveyMixAPU1;
		int		iSurveyMixAPU2;
		int		iSurveyMixVRC6;
		int		iSurveyMixVRC7;
		int		iSurveyMixFDS;
		int		iSurveyMixMMC5;
		int		iSurveyMixN163;
		int		iSurveyMixS5B;
	} ChipLevels;

	struct {
		// FDS
		int		iFDSLowpass;
		// N163
		bool	bNamcoMixing;		// // //
		int		iN163Lowpass;
		// VRC7
		int		iVRC7Patch;
	} Emulation;

	CString InstrumentMenuPath;

private:
	template<class T>
	CSettingBase *AddSetting(LPCTSTR pSection, LPCTSTR pEntry, T tDefault, T *pVariable);		// // //
	void SetupSettings();

private:
	static const int MAX_SETTINGS = 128;

private:
	CSettingBase *m_pSettings[MAX_SETTINGS];
	int m_iAddedSettings;

private:
	// Paths
	CString Paths[PATH_COUNT];
};


