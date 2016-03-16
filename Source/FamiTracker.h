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

#pragma once


// FamiTracker.h : main header file for the FamiTracker application

#include "version.h"

// Support DLL translations
#define SUPPORT_TRANSLATIONS

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

// Inter-process commands
enum {
	IPC_LOAD = 1,	
	IPC_LOAD_PLAY
};

#ifdef RELEASE_BUILD
// Always disable export test for release builds
#undef EXPORT_TEST
#endif /* RELEASE_BUILD */

// Custom command line reader
class CFTCommandLineInfo : public CCommandLineInfo
{
public:
	CFTCommandLineInfo();
	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);
public:
	bool m_bLog;
	bool m_bExport;
	bool m_bPlay;
#ifdef EXPORT_TEST
	bool m_bVerifyExport;
	CString m_strVerifyFile;
#endif
	CString m_strExportFile;
	CString m_strExportLogFile;
	CString m_strExportDPCMFile;
};


class CMIDI;
class CSoundGen;
class CSettings;
class CAccelerator;
class CChannelMap;
class CCustomExporters;

class CMutex;

enum play_mode_t;	// Defined in soundgen.h

/*!
	\brief A MFC document template supporting both .0cc and .ftm file extensions.
*/
class CDocTemplate0CC : public CSingleDocTemplate
{
public:
	CDocTemplate0CC(UINT nIDResource, CRuntimeClass* pDocClass, CRuntimeClass* pFrameClass, CRuntimeClass* pViewClass);
	BOOL GetDocString(CString& rString, enum DocStringIndex i) const;
	CDocTemplate::Confidence MatchDocType(const char* pszPathName, CDocument*& rpDocMatch);
};

// CFamiTrackerApp:
// See FamiTracker.cpp for the implementation of this class
//

class CFamiTrackerApp : public CWinApp
{
public:
	// Constructor
	CFamiTrackerApp();

	//
	// Public functions
	//
public:
	void			LoadSoundConfig();
	void			ReloadColorScheme();
	int				GetCPUUsage() const;
	bool			IsThemeActive() const;
	void			RemoveSoundGenerator();
	void			ThreadDisplayMessage(LPCTSTR lpszText, UINT nType = 0, UINT nIDHelp = 0);
	void			ThreadDisplayMessage(UINT nIDPrompt, UINT nType = 0, UINT nIDHelp = 0);

	// Tracker player functions
	void			StartPlayer(play_mode_t Mode);
	void			StopPlayer();
	void			StopPlayerAndWait();
	void			TogglePlayer();
	bool			IsPlaying() const;
	void			ResetPlayer();
	void			SilentEverything();
	void			WaitUntilStopped() const;

	// Get-functions
	CAccelerator	*GetAccelerator() const		{ ASSERT(m_pAccel); return m_pAccel; }
	CSoundGen		*GetSoundGenerator() const	{ ASSERT(m_pSoundGenerator); return m_pSoundGenerator; }
	CMIDI			*GetMIDI() const			{ ASSERT(m_pMIDI); return m_pMIDI; }
	CSettings		*GetSettings() const		{ ASSERT(m_pSettings); return m_pSettings; }
	CChannelMap		*GetChannelMap() const		{ ASSERT(m_pChannelMap); return m_pChannelMap; }
	
	CCustomExporters *GetCustomExporters() const;

#ifdef EXPORT_TEST
	void			VerifyExport() const;
	void			VerifyExport(LPCTSTR File) const;
	bool			IsExportTest() const;
#endif /* EXPORT_TEST */

	//
	// Private functions
	//
private:
	void CheckAppThemed();
	void ShutDownSynth();
	bool CheckSingleInstance(CFTCommandLineInfo &cmdInfo);
	void RegisterSingleInstance();
	void UnregisterSingleInstance();
	void CheckNewVersion();
	void LoadLocalization();

public: // as it is
	BOOL DoPromptFileName(CString& fileName, CString& filePath, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);

	// Private variables and objects
private:
	// Objects
	CMIDI			*m_pMIDI;
	CAccelerator	*m_pAccel;					// Keyboard accelerator
	CSoundGen		*m_pSoundGenerator;			// Sound synth & player
	CSettings		*m_pSettings;				// Program settings
	CChannelMap		*m_pChannelMap;

	CCustomExporters *m_customExporters;

	// Single instance stuff
	CMutex			*m_pInstanceMutex;
	HANDLE			m_hWndMapFile;

	bool			m_bThemeActive;

#ifdef EXPORT_TEST
	bool			m_bExportTesting;
#endif

#ifdef SUPPORT_TRANSLATIONS
	HINSTANCE		m_hInstResDLL;
#endif

	// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();	

	// Implementation
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnTestExport();
};

extern CFamiTrackerApp theApp;

// Global helper functions
CString LoadDefaultFilter(LPCTSTR Name, LPCTSTR Ext);
CString LoadDefaultFilter(UINT nID, LPCTSTR Ext);
void AfxFormatString3(CString &rString, UINT nIDS, LPCTSTR lpsz1, LPCTSTR lpsz2, LPCTSTR lpsz3);
CString MakeIntString(int val, LPCTSTR format = _T("%i"));
CString MakeFloatString(float val, LPCTSTR format = _T("%g"));
