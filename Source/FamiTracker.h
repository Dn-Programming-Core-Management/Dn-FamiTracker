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

#pragma once


// FamiTracker.h : main header file for the FamiTracker application

#include <thread>		// // //
#include <string>

// Support DLL translations
#define SUPPORT_TRANSLATIONS

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "../resource.h"       // main symbols

// Inter-process commands
enum {
	IPC_LOAD = 1,
	IPC_LOAD_PLAY
};

// Custom command line reader
class CFTCommandLineInfo : public CCommandLineInfo
{
public:
	CFTCommandLineInfo();
	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast);
public:
	bool m_bHelp;		// // !!
	bool m_bLog;
	bool m_bExport;
	bool m_bPlay;
	CString m_strExportFile;
	CString m_strExportLogFile;
	CString m_strExportDPCMFile;
};

class CMainFrame;		// // //
class CMIDI;
class CSoundGen;
class CSettings;
class CAccelerator;
class CChannelMap;
class CCustomExporters;
class CVersionChecker;		// // //

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

/*!
	\brief A MFC document manager allowing both .0cc and .ftm file extensions to be displayed while
	opening or saving documents.
	\details This class also saves the FTM path of the last load/save operation to the registry.
*/
class CDocManager0CC : public CDocManager
{
public:
	virtual BOOL DoPromptFileName(CString& fileName, UINT nIDSTitle,
								  DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate* pTemplate);
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
	void			CheckNewVersion(bool StartUp);		// // //
	void			LoadSoundConfig();
	void			UpdateMenuShortcuts();		// // //
	void			ReloadColorScheme();
	int				GetCPUUsage() const;
	bool			IsThemeActive() const;
	void			RemoveSoundGenerator();
	void			RefreshFrameEditor();
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
	CMainFrame		*GetMainFrame() const;		// // //
	CAccelerator	*GetAccelerator() const		{ ASSERT(m_pAccel); return m_pAccel; }
	CSoundGen		*GetSoundGenerator() const	{ ASSERT(m_pSoundGenerator); return m_pSoundGenerator; }
	CMIDI			*GetMIDI() const			{ ASSERT(m_pMIDI); return m_pMIDI; }
	CSettings		*GetSettings() const		{ ASSERT(m_pSettings); return m_pSettings; }
	CChannelMap		*GetChannelMap() const		{ ASSERT(m_pChannelMap); return m_pChannelMap; }

	CCustomExporters *GetCustomExporters() const;

	//
	// Private functions
	//
private:
	void CheckAppThemed();
	void ShutDownSynth();
	bool CheckSingleInstance(CFTCommandLineInfo &cmdInfo);
	void RegisterSingleInstance();
	void UnregisterSingleInstance();
	void LoadLocalization();

	// Private variables and objects
private:
	static const int MAX_RECENT_FILES = 8;		// // //

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

	bool m_CoInitialized;
	bool			m_bRunning = false;		// // //
	bool			m_bThemeActive;

	bool			m_bVersionReady;
public:
	std::unique_ptr<CVersionChecker> m_pVersionChecker;		// // //
	std::string		m_pVersionURL, m_pVerInfo, m_pVerDesc;
	bool m_bStartUp;

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
	afx_msg BOOL OnIdle(LONG lCount);		// // //
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	afx_msg void OnVersionCheck();			// // !!
	afx_msg void OnHelpVersionCheck();		// // !!
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnTestExport();
	void OnRecentFilesClear();		// // //
	void OnUpdateRecentFiles(CCmdUI *pCmdUI);		// // //
};

extern CFamiTrackerApp theApp;

// Global helper functions
CString LoadDefaultFilter(LPCTSTR Name, LPCTSTR Ext);
CString LoadDefaultFilter(UINT nID, LPCTSTR Ext);
void AfxFormatString3(CString &rString, UINT nIDS, LPCTSTR lpsz1, LPCTSTR lpsz2, LPCTSTR lpsz3);
CString MakeIntString(int val, LPCTSTR format = _T("%i"));
CString MakeFloatString(float val, LPCTSTR format = _T("%g"));
