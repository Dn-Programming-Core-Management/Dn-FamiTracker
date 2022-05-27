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

#include "json/json.hpp"		// // //
#include "version.h"		// // //
#include "stdafx.h"
#include "Exception.h"
#include "FamiTracker.h"
#include "version.h"		// // //
#include "Exception.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "TrackerChannel.h"
#include "MIDI.h"
#include "SoundGen.h"
#include "Accelerator.h"
#include "Settings.h"
#include "ChannelMap.h"
#include "CustomExporters.h"
#include "CommandLineExport.h"
#include "WinSDK/VersionHelpers.h"		// // //
#include "VisualizerWnd.h"		// // //
#include "htmlhelp.h"		// // !!
#include "VersionChecker.h"		// // //
#include "VersionCheckerDlg.h"		// // !!
#include <iostream>		// // //
#include <utility>  // std::move
#include "str_conv/str_conv.hpp"		// // //

// 0CC uses AfxRegSetValue() and AfxGetModuleShortFileName(),
// found in the undocumented header afxpriv.h.
// These functions shouldn't have been used... but here we are.
#include <afxpriv.h>

// Single instance-stuff
const TCHAR FT_SHARED_MUTEX_NAME[]	= _T("FamiTrackerMutex");	// Name of global mutex
const TCHAR FT_SHARED_MEM_NAME[]	= _T("FamiTrackerWnd");		// Name of global memory area
const DWORD	SHARED_MEM_SIZE			= 256;

// CFamiTrackerApp

BEGIN_MESSAGE_MAP(CFamiTrackerApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_RECENTFILES_CLEAR, OnRecentFilesClear)		// // //
	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFiles)		// // //
	ON_COMMAND(ID_HELP_VERSION_CHECK, OnHelpVersionCheck)		// // !!
END_MESSAGE_MAP()

// CFamiTrackerApp construction

CFamiTrackerApp::CFamiTrackerApp() :
	m_CoInitialized(false),
	m_bThemeActive(false),
	m_pMIDI(NULL),
	m_pAccel(NULL),
	m_pSettings(NULL),
	m_pSoundGenerator(NULL),
	m_pChannelMap(NULL),
	m_customExporters(NULL),
	m_hWndMapFile(NULL),
#ifdef SUPPORT_TRANSLATIONS
	m_hInstResDLL(NULL),
#endif
	m_pInstanceMutex(NULL)
{
	// Place all significant initialization in InitInstance
	EnableHtmlHelp();

#ifdef ENABLE_CRASH_HANDLER
	// This will cover the whole process
	InstallExceptionHandler();
#endif /* ENABLE_CRASH_HANDLER */
}


// The one and only CFamiTrackerApp object
CFamiTrackerApp	theApp;

// CFamiTrackerApp initialization

BOOL CFamiTrackerApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();
#ifdef SUPPORT_TRANSLATIONS
	LoadLocalization();
#endif
	CWinApp::InitInstance();

	TRACE("App: InitInstance\n");

	if (!AfxOleInit()) {
		TRACE("OLE initialization failed\n");
	}

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		TRACE("CFamiTrackerApp: Failed to CoInitializeEx COM!\n");
	}
	if (!FAILED(hr)) {
		// Call CoUninitialize() on shutdown.
		m_CoInitialized = true;
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T(""));
	LoadStdProfileSettings(MAX_RECENT_FILES);  // Load standard INI file options (including MRU)

	// Load program settings
	m_pSettings = CSettings::GetObject();
	m_pSettings->LoadSettings();

	// Parse command line for standard shell commands, DDE, file open + some custom ones
	CFTCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (CheckSingleInstance(cmdInfo))
		return FALSE;

	//who: added by Derek Andrews <derek.george.andrews@gmail.com>
	//why: Load all custom exporter plugins on startup.

	TCHAR pathToPlugins[MAX_PATH];
	GetModuleFileName(NULL, pathToPlugins, MAX_PATH);
	PathRemoveFileSpec(pathToPlugins);
	PathAppend(pathToPlugins, _T("\\Plugins"));
	m_customExporters = new CCustomExporters( pathToPlugins );

	// Load custom accelerator
	m_pAccel = new CAccelerator();
	m_pAccel->LoadShortcuts(m_pSettings);
	m_pAccel->Setup();

	// Create the MIDI interface
	m_pMIDI = new CMIDI();

	// Create sound generator
	m_pSoundGenerator = new CSoundGen();

	// Create channel map
	m_pChannelMap = new CChannelMap();

	// Check if the application is themed
	CheckAppThemed();

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CDocTemplate0CC* pDocTemplate = new CDocTemplate0CC(		// // //
		IDR_MAINFRAME,
		RUNTIME_CLASS(CFamiTrackerDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(CFamiTrackerView));

	if (!pDocTemplate)
		return FALSE;

	if (m_pDocManager == NULL)		// // //
		m_pDocManager = new CDocManager0CC { };
	m_pDocManager->AddDocTemplate(pDocTemplate);

	// Enable file type registration in Windows Vista and up
	AfxSetPerUserRegistration(TRUE);

	// Enable DDE Execute open
	EnableShellOpen();

	// Skip this if in wip/beta mode
#if !defined(WIP) && !defined(_DEBUG)
	// Add shell options
	RegisterShellFileTypes();		// // //
	static const LPCTSTR FILE_ASSOC_NAME = _T(APP_NAME " Module");
	AfxRegSetValue(HKEY_CLASSES_ROOT, "0CCFamiTracker.Document", REG_SZ, FILE_ASSOC_NAME, lstrlen(FILE_ASSOC_NAME) * sizeof(TCHAR));
	// Add an option to play files
	CString strPathName, strTemp, strFileTypeId;
	AfxGetModuleShortFileName(AfxGetInstanceHandle(), strPathName);
	CString strOpenCommandLine = strPathName;
	strOpenCommandLine += _T(" /play \"%1\"");
	if (pDocTemplate->GetDocString(strFileTypeId, CDocTemplate::regFileTypeId) && !strFileTypeId.IsEmpty()) {
		strTemp.Format(_T("%s\\shell\\play\\%s"), (LPCTSTR)strFileTypeId, _T("command"));
		AfxRegSetValue(HKEY_CLASSES_ROOT, strTemp, REG_SZ, strOpenCommandLine, lstrlen(strOpenCommandLine) * sizeof(TCHAR));
	}
#endif

	AfxSetPerUserRegistration(FALSE);

	// Handle command line export
	if (cmdInfo.m_bExport) {
		CCommandLineExport exporter;
		exporter.CommandLineExport(cmdInfo.m_strFileName, cmdInfo.m_strExportFile, cmdInfo.m_strExportLogFile, cmdInfo.m_strExportDPCMFile);
		ExitProcess(0);
	}
	if (cmdInfo.m_bHelp) {		// // !!
		ExitProcess(0);
	}

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo)) {
		if (cmdInfo.m_nShellCommand == CCommandLineInfo::AppUnregister) {
			// Also clear settings from registry when unregistering
			m_pSettings->DeleteSettings();
		}
		return FALSE;
	}

	// Move root key back to default
	if (IsWindowsVistaOrGreater()) {		// // //
		::RegOverridePredefKey(HKEY_CLASSES_ROOT, NULL);
	}

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(m_nCmdShow);
	m_pMainWnd->UpdateWindow();
	// call DragAcceptFiles only if there's a suffix
	//  In an SDI app, this should occur after ProcessShellCommand
	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Initialize the sound interface, also starts the thread
	if (!m_pSoundGenerator->BeginThread()) {
		// If failed, restore and save default settings
		m_pSettings->DefaultSettings();
		m_pSettings->SaveSettings();
		// Quit program
		AfxMessageBox(IDS_START_ERROR, MB_ICONERROR);
		return FALSE;
	}

	// Initialize midi unit
	m_pMIDI->Init();

	if (cmdInfo.m_bPlay)
		theApp.StartPlayer(MODE_PLAY);

	// Save the main window handle
	RegisterSingleInstance();

#ifndef _DEBUG
	m_pMainWnd->GetMenu()->GetSubMenu(4)->RemoveMenu(ID_MODULE_CHANNELS, MF_BYCOMMAND);		// // //
#endif

	if (m_pSettings->General.bCheckVersion)		// // //
		CheckNewVersion(true);

	// Initialization is done
	TRACE("App: InitInstance done\n");
	m_bRunning = true;		// // //
	return TRUE;
}

int CFamiTrackerApp::ExitInstance()
{
	// Close program
	// The document is already closed at this point (and detached from sound player)

	TRACE("App: Begin ExitInstance\n");

	UnregisterSingleInstance();

	ShutDownSynth();

	if (m_pMIDI) {
		m_pMIDI->Shutdown();
		delete m_pMIDI;
		m_pMIDI = NULL;
	}

	if (m_pAccel) {
		m_pAccel->SaveShortcuts(m_pSettings);
		m_pAccel->Shutdown();
		delete m_pAccel;
		m_pAccel = NULL;
	}

	if (m_pSettings) {
		m_pSettings->SaveSettings();
		m_pSettings = NULL;
	}

	if (m_customExporters) {
		delete m_customExporters;
		m_customExporters = NULL;
	}

	if (m_pChannelMap) {
		delete m_pChannelMap;
		m_pChannelMap = NULL;
	}

#ifdef SUPPORT_TRANSLATIONS
	if (m_hInstResDLL) {
		// Revert back to internal resources
		AfxSetResourceHandle(m_hInstance);
		// Unload DLL
		::FreeLibrary(m_hInstResDLL);
		m_hInstResDLL = NULL;
	}
#endif

	m_pVersionChecker.reset();		// // //

	if (m_CoInitialized) {
		CoUninitialize();
	}

	TRACE("App: End ExitInstance\n");

	return CWinApp::ExitInstance();
}

BOOL CFamiTrackerApp::PreTranslateMessage(MSG* pMsg)
{
	if (CWinApp::PreTranslateMessage(pMsg)) {
		return TRUE;
	}
	else if (m_pMainWnd != NULL && m_pAccel != NULL) {
		if (m_pAccel->Translate(m_pMainWnd->m_hWnd, pMsg)) {
			return TRUE;
		}
	}

	return FALSE;
}

void CFamiTrackerApp::CheckAppThemed()
{
	HMODULE hinstDll = ::LoadLibrary(_T("UxTheme.dll"));

	if (hinstDll) {
		typedef BOOL (*ISAPPTHEMEDPROC)();
		ISAPPTHEMEDPROC pIsAppThemed;
		pIsAppThemed = (ISAPPTHEMEDPROC) ::GetProcAddress(hinstDll, "IsAppThemed");

		if(pIsAppThemed)
			m_bThemeActive = (pIsAppThemed() == TRUE);

		::FreeLibrary(hinstDll);
	}
}

bool CFamiTrackerApp::IsThemeActive() const
{
	return m_bThemeActive;
}

bool GetFileVersion(LPCTSTR Filename, WORD &Major, WORD &Minor, WORD &Revision, WORD &Build)
{
	DWORD Handle;
	DWORD Size = GetFileVersionInfoSize(Filename, &Handle);
	bool Success = true;

	Major = 0;
	Minor = 0;
	Revision = 0;
	Build = 0;

	if (Size > 0) {
		TCHAR *pData = new TCHAR[Size];
		if (GetFileVersionInfo(Filename, NULL, Size, pData) != 0) {
			UINT size;
			VS_FIXEDFILEINFO *pFileinfo;
			if (VerQueryValue(pData, _T("\\"), (LPVOID*)&pFileinfo, &size) != 0) {
				Major = pFileinfo->dwProductVersionMS >> 16;
				Minor = pFileinfo->dwProductVersionMS & 0xFFFF;
				Revision = pFileinfo->dwProductVersionLS >> 16;
				Build = pFileinfo->dwProductVersionLS & 0xFFFF;
			}
			else
				Success = false;
		}
		else
			Success = false;

		SAFE_RELEASE_ARRAY(pData);
	}
	else
		Success = false;

	return Success;
}

#ifdef SUPPORT_TRANSLATIONS
void CFamiTrackerApp::LoadLocalization()
{
	LPCTSTR DLL_NAME = _T("language.dll");
	WORD Major, Minor, Build, Revision;

	if (GetFileVersion(DLL_NAME, Major, Minor, Revision, Build)) {
		if (Major != VERSION_API || Minor != VERSION_MAJ || Revision != VERSION_MIN || Build != VERSION_REV)		// // //
			return;

		m_hInstResDLL = ::LoadLibrary(DLL_NAME);

		if (m_hInstResDLL != NULL) {
			TRACE("App: Loaded localization DLL\n");
			AfxSetResourceHandle(m_hInstResDLL);
		}
	}
}
#endif

void CFamiTrackerApp::OnRecentFilesClear()		// // //
{
	int confirm = AfxMessageBox(IDS_CLEAR_RECENTS, MB_YESNO | MB_ICONEXCLAMATION | MB_DEFBUTTON2);
	if (confirm == IDNO) {
		return;
	} else if (confirm != IDYES) {
		throw std::runtime_error("Recent Files Clear message box returned invalid value");
	}

	SAFE_RELEASE(m_pRecentFileList);
	m_pRecentFileList = new CRecentFileList(0, _T("Recent File List"), _T("File%d"), MAX_RECENT_FILES);

	auto pMenu = m_pMainWnd->GetMenu()->GetSubMenu(0)->GetSubMenu(14);
	for (int i = 0; i < MAX_RECENT_FILES; ++i)
		pMenu->RemoveMenu(ID_FILE_MRU_FILE1 + i, MF_BYCOMMAND);
	pMenu->AppendMenu(MF_STRING, ID_FILE_MRU_FILE1, _T("(File)"));
}

void CFamiTrackerApp::OnUpdateRecentFiles(CCmdUI *pCmdUI)		// // //
{
	// https://www.codeguru.com/cpp/controls/menu/miscellaneous/article.php/c167
	// updating a submenu?
	if (pCmdUI->m_pSubMenu != NULL) return;

	m_pRecentFileList->UpdateMenu(pCmdUI);
}

void CFamiTrackerApp::ShutDownSynth()
{
	// Shut down sound generator
	if (m_pSoundGenerator == NULL) {
		TRACE("App: Sound generator object was not available\n");
		return;
	}

	auto stdThread = std::move(m_pSoundGenerator->m_audioThread);

	if (!stdThread.joinable()) {
		// Object was found but thread not created
		delete m_pSoundGenerator;
		m_pSoundGenerator = NULL;
		TRACE("App: Sound generator object was found but no thread created\n");
		return;
	}

	TRACE("App: Waiting for sound player thread to close\n");

	// Send quit message. Note that this object may be deleted now!
	m_pSoundGenerator->PostGuiMessage(AM_QUIT, 0, 0);

	// If audio thread is waiting on stuck WASAPI, interrupt the wait. On the next loop
	// iteration, it will see the quit message.
	// TODO fix data race on closing m_hInterruptEvent or deleting m_pSoundGenerator
	m_pSoundGenerator->Interrupt();

	// Wait for thread to exit
	DWORD dwResult = ::WaitForSingleObject(
		stdThread.native_handle(), CSoundGen::AUDIO_TIMEOUT + 1000
	);

	if (dwResult != WAIT_OBJECT_0 && m_pSoundGenerator != NULL) {
		TRACE("App: Closing the sound generator thread failed\n");
#ifdef _DEBUG
		AfxMessageBox(_T("Error: Could not close sound generator thread"));
#endif
		stdThread.detach();
		// Unclean exit
		return;
	}
	stdThread.join();

	// Object should be auto-deleted
	ASSERT(m_pSoundGenerator == NULL);

	TRACE("App: Sound generator has closed\n");
}

void CFamiTrackerApp::RemoveSoundGenerator()
{
	// Sound generator object has been deleted, remove reference
	m_pSoundGenerator = NULL;
}

CCustomExporters* CFamiTrackerApp::GetCustomExporters(void) const
{
	return m_customExporters;
}

void CFamiTrackerApp::RegisterSingleInstance()
{
	// Create a memory area with this app's window handle
	if (!GetSettings()->General.bSingleInstance)
		return;

	m_hWndMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHARED_MEM_SIZE, FT_SHARED_MEM_NAME);

	if (m_hWndMapFile != NULL) {
		LPTSTR pBuf = (LPTSTR) MapViewOfFile(m_hWndMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEM_SIZE);
		if (pBuf != NULL) {
			// Create a string of main window handle
			_itot_s((int)GetMainWnd()->m_hWnd, pBuf, SHARED_MEM_SIZE, 10);
			UnmapViewOfFile(pBuf);
		}
	}
}

void CFamiTrackerApp::UnregisterSingleInstance()
{
	// Close shared memory area
	if (m_hWndMapFile) {
		CloseHandle(m_hWndMapFile);
		m_hWndMapFile = NULL;
	}

	SAFE_RELEASE(m_pInstanceMutex);
}

void CFamiTrackerApp::CheckNewVersion(bool StartUp)		// // //
{
	//return;
	m_pVersionChecker = std::make_unique<CVersionChecker>(StartUp);		// // //
}

bool CFamiTrackerApp::CheckSingleInstance(CFTCommandLineInfo &cmdInfo)
{
	// Returns true if program should close

	if (!GetSettings()->General.bSingleInstance)
		return false;

	if (cmdInfo.m_bExport)
		return false;

	m_pInstanceMutex = new CMutex(FALSE, FT_SHARED_MUTEX_NAME);

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// Another instance detected, get window handle
		HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, FT_SHARED_MEM_NAME);
		if (hMapFile != NULL) {
			LPCTSTR pBuf = (LPTSTR) MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEM_SIZE);
			if (pBuf != NULL) {
				// Get window handle
				HWND hWnd = (HWND)_ttoi(pBuf);
				if (hWnd != NULL) {
					// Get file name
					LPTSTR pFilePath = cmdInfo.m_strFileName.GetBuffer();
					// We have the window handle & file, send a message to open the file
					COPYDATASTRUCT data;
					data.dwData = cmdInfo.m_bPlay ? IPC_LOAD_PLAY : IPC_LOAD;
					data.cbData = (DWORD)((_tcslen(pFilePath) + 1) * sizeof(TCHAR));
					data.lpData = pFilePath;
					ULONG_PTR result;
					SendMessageTimeout(hWnd, WM_COPYDATA, NULL, (LPARAM)&data, SMTO_NORMAL, 100, &result);
					UnmapViewOfFile(pBuf);
					CloseHandle(hMapFile);
					TRACE("App: Found another instance, shutting down\n");
					// Then close the program
					return true;
				}

				UnmapViewOfFile(pBuf);
			}
			CloseHandle(hMapFile);
		}
	}

	return false;
}

////////////////////////////////////////////////////////
//  Things that belongs to the synth are kept below!  //
////////////////////////////////////////////////////////

// Load sound configuration
void CFamiTrackerApp::LoadSoundConfig()
{
	GetSoundGenerator()->LoadSettings();
	GetSoundGenerator()->Interrupt();
	static_cast<CFrameWnd*>(GetMainWnd())->SetMessageText(IDS_NEW_SOUND_CONFIG);
}

void CFamiTrackerApp::UpdateMenuShortcuts()		// // //
{
	CMainFrame *pMainFrm = dynamic_cast<CMainFrame*>(GetMainWnd());
	if (pMainFrm != nullptr)
		pMainFrm->UpdateMenus();
}
CMainFrame *CFamiTrackerApp::GetMainFrame() const {
	auto* pMainFrm = static_cast<CMainFrame*>(const_cast<CFamiTrackerApp*>(this)->GetMainWnd());
	return ::IsWindow(pMainFrm->m_hWnd) && m_bRunning ? pMainFrm : nullptr;
}

// Silences everything
void CFamiTrackerApp::SilentEverything()
{
	GetSoundGenerator()->SilentAll();
	CFamiTrackerView::GetView()->MakeSilent();
}

int CFamiTrackerApp::GetCPUUsage() const
{
	// Calculate CPU usage
	const HANDLE hThreads[] = {
		m_hThread,
		m_pSoundGenerator->m_audioThread.native_handle(),
		GetMainFrame()->GetVisualizerWnd()->GetThreadHandle(),
	};

	static FILETIME KernelLastTime[std::size(hThreads)] = { };
	static FILETIME UserLastTime[std::size(hThreads)] = { };
	uint64_t TotalTime = 0u;

	// // // get processor count
	SYSTEM_INFO si;
	::GetNativeSystemInfo(&si);

	for (std::size_t i = 0; i < std::size(hThreads); ++i) {
		FILETIME CreationTime, ExitTime, KernelTime, UserTime;
		GetThreadTimes(hThreads[i], &CreationTime, &ExitTime, &KernelTime, &UserTime);
		TotalTime += (KernelTime.dwLowDateTime - KernelLastTime[i].dwLowDateTime) / 1000;
		TotalTime += (UserTime.dwLowDateTime - UserLastTime[i].dwLowDateTime) / 1000;
		KernelLastTime[i] = KernelTime;
		UserLastTime[i]	= UserTime;
	}

	return static_cast<int>(TotalTime / si.dwNumberOfProcessors);
}

void CFamiTrackerApp::ReloadColorScheme()
{
	// Notify all views
	POSITION TemplatePos = GetFirstDocTemplatePosition();
	CDocTemplate *pDocTemplate = GetNextDocTemplate(TemplatePos);
	POSITION DocPos = pDocTemplate->GetFirstDocPosition();

	while (CDocument* pDoc = pDocTemplate->GetNextDoc(DocPos)) {
		POSITION ViewPos = pDoc->GetFirstViewPosition();
		while (CView *pView = pDoc->GetNextView(ViewPos)) {
			if (pView->IsKindOf(RUNTIME_CLASS(CFamiTrackerView)))
				static_cast<CFamiTrackerView*>(pView)->SetupColors();
		}
	}

	// Main window
	CMainFrame *pMainFrm = dynamic_cast<CMainFrame*>(GetMainWnd());

	if (pMainFrm != NULL) {
		pMainFrm->SetupColors();
		pMainFrm->RedrawWindow();
	}
}

// refresh the frame editor to reflect new changes from the configuration settings
void CFamiTrackerApp::RefreshFrameEditor()
{
	// The frame editor UI will update late without this
	// function call in this particular spot
	GetMainFrame()->ResizeFrameWindow();

	CFamiTrackerView::GetView()->RefreshFrameEditor();
}

BOOL CFamiTrackerApp::OnIdle(LONG lCount)		// // //
{
	if (CWinApp::OnIdle(lCount))
		return TRUE;

	if (m_pVersionChecker && m_pVersionChecker->IsReady())
		if (auto pChecker = std::move(m_pVersionChecker); auto result = pChecker->GetVersionCheckResult())
		{
			m_pVersionURL = result->URL;
			m_pVerInfo = result->VerInfo;
			m_pVerDesc = result->VerDesc;
			m_bStartUp = result->StartUp;
			OnVersionCheck();
		}
	return FALSE;
}

void CFamiTrackerApp::OnVersionCheck()
{
	CVersionCheckerDlg VCDlg;
	VCDlg.DoModal();
}

void CFamiTrackerApp::OnHelpVersionCheck() {
	CheckNewVersion(false);
}

// App command to run the about dialog
void CFamiTrackerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CFamiTrackerApp message handlers

void CFamiTrackerApp::StartPlayer(play_mode_t Mode)
{
	int Track = static_cast<CMainFrame*>(GetMainWnd())->GetSelectedTrack();
	if (m_pSoundGenerator)
		m_pSoundGenerator->StartPlayer(Mode, Track);
}

void CFamiTrackerApp::StopPlayer()
{
	if (m_pSoundGenerator)
		m_pSoundGenerator->StopPlayer();

	m_pMIDI->ResetOutput();
}

void CFamiTrackerApp::StopPlayerAndWait()
{
	// Synchronized stop
	if (m_pSoundGenerator) {
		m_pSoundGenerator->StopPlayer();
		m_pSoundGenerator->WaitForStop();
	}
	m_pMIDI->ResetOutput();
}

void CFamiTrackerApp::TogglePlayer()
{
	if (m_pSoundGenerator) {
		if (m_pSoundGenerator->IsPlaying())
			StopPlayer();
		else
			StartPlayer(MODE_PLAY);
	}
}

// Player interface

bool CFamiTrackerApp::IsPlaying() const
{
	if (m_pSoundGenerator)
		return m_pSoundGenerator->IsPlaying();

	return false;
}

void CFamiTrackerApp::ResetPlayer()
{
	// Called when changing track
	int Track = static_cast<CMainFrame*>(GetMainWnd())->GetSelectedTrack();
	if (m_pSoundGenerator)
		m_pSoundGenerator->ResetPlayer(Track);
}

// File load/save

void CFamiTrackerApp::OnFileOpen()
{
	CString newName = _T("");		// // //

	if (!AfxGetApp()->DoPromptFileName(newName, AFX_IDS_OPENFILE, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, TRUE, NULL))
		return; // open cancelled

	CFrameWnd *pFrameWnd = (CFrameWnd*)GetMainWnd();

	if (pFrameWnd)
		pFrameWnd->SetMessageText(IDS_LOADING_FILE);

	AfxGetApp()->OpenDocumentFile(newName);

	if (pFrameWnd)
		pFrameWnd->SetMessageText(IDS_LOADING_DONE);
}

// Used to display a messagebox on the main thread
void CFamiTrackerApp::ThreadDisplayMessage(LPCTSTR lpszText, UINT nType, UINT nIDHelp)
{
	m_pMainWnd->SendMessage(WM_USER_DISPLAY_MESSAGE_STRING, (WPARAM)lpszText, (LPARAM)nType);
}

void CFamiTrackerApp::ThreadDisplayMessage(UINT nIDPrompt, UINT nType, UINT nIDHelp)
{
	m_pMainWnd->SendMessage(WM_USER_DISPLAY_MESSAGE_ID, (WPARAM)nIDPrompt, (LPARAM)nType);
}

// Various global helper functions

CString LoadDefaultFilter(LPCTSTR Name, LPCTSTR Ext)
{
	// Loads a single filter string including the all files option
	CString filter;
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));

	filter = Name;
	filter += _T("|*");
	filter += Ext;
	filter += _T("|");
	filter += allFilter;
	filter += _T("|*.*||");

	return filter;
}

CString LoadDefaultFilter(UINT nID, LPCTSTR Ext)
{
	// Loads a single filter string including the all files option
	CString filter;
	CString allFilter;
	VERIFY(allFilter.LoadString(AFX_IDS_ALLFILTER));

	filter.LoadString(nID);
	filter += _T("|*");
	filter += Ext;
	filter += _T("|");
	filter += allFilter;
	filter += _T("|*.*||");

	return filter;
}

void AfxFormatString3(CString &rString, UINT nIDS, LPCTSTR lpsz1, LPCTSTR lpsz2, LPCTSTR lpsz3)
{
	// AfxFormatString with three arguments
	LPCTSTR arr[] = {lpsz1, lpsz2, lpsz3};
	AfxFormatStrings(rString, nIDS, arr, 3);
}

CString MakeIntString(int val, LPCTSTR format)
{
	// Turns an int into a string
	CString str;
	str.Format(format, val);
	return str;
}

CString MakeFloatString(float val, LPCTSTR format)
{
	// Turns a float into a string
	CString str;
	str.Format(format, val);
	return str;
}

/**
 * CFTCommandLineInfo, a custom command line parser
 *
 */

CFTCommandLineInfo::CFTCommandLineInfo() : CCommandLineInfo(),
	m_bLog(false),
	m_bExport(false),
	m_bPlay(false),
	m_bHelp(false),		// // !!
	m_strExportFile(_T("")),
	m_strExportLogFile(_T("")),
	m_strExportDPCMFile(_T(""))
{
}

void CFTCommandLineInfo::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
	// Call default implementation
	CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);

	if (bFlag) {
		// Export file (/export or /e)
		if (!_tcsicmp(pszParam, _T("export")) || !_tcsicmp(pszParam, _T("e"))) {
			m_bExport = true;
			return;
		}
		// Auto play (/play or /p)
		else if (!_tcsicmp(pszParam, _T("play")) || !_tcsicmp(pszParam, _T("p"))) {
			m_bPlay = true;
			return;
		}
		// Disable crash dumps (/nodump)
		else if (!_tcsicmp(pszParam, _T("nodump"))) {
#ifdef ENABLE_CRASH_HANDLER
			UninstallExceptionHandler();
#endif
			return;
		}
		// Enable register logger (/log), available in debug mode only
		else if (!_tcsicmp(pszParam, _T("log"))) {
#ifdef _DEBUG
			m_bLog = true;
			return;
#endif
		}
		// Enable console output (TODO)
		// This is intended for a small helper program that avoids the problem with console on win32 programs,
		// and should remain undocumented. I'm using it for testing.
		else if (!_tcsicmp(pszParam, _T("console"))) {
			FILE *f;
			AttachConsole(ATTACH_PARENT_PROCESS);
			errno_t err = freopen_s(&f, "CON", "w", stdout);
			errno_t err2 = freopen_s(&f, "CON", "w", stderr);
			fprintf(stderr, "%s\n", APP_NAME_VERSION);		// // //
			return;
		}
		// // !! help (/help)
		else if (!_tcsicmp(pszParam, _T("help")) || !_tcsicmp(pszParam, _T("h"))) {
			m_bHelp = true;
			FILE* cout;
			AttachConsole(ATTACH_PARENT_PROCESS);
			errno_t err = freopen_s(&cout, "CON", "w", stdout);
			// TODO: format this better
			std::string helpmessage = "Dn-FamiTracker commandline help";
;			helpmessage += "\nusage: Dn-FamiTracker [module file] [-play | -export | -nodump | -log]\n";
			helpmessage += "options:\n";
			helpmessage += "play\t: automatically plays when the program starts\n";
			helpmessage += "export\t: exports the module to a specified format. the format is determined by the filetype of the output.\n";
			helpmessage += "\t-export [output file] [optional log file] [DPCM file for BIN export]\n";
			helpmessage += "\tthe following formats are available:\n";
			helpmessage += "\t\t.nsf\n\t\t.nes\n\t\t.bin\n\t\t.prg\n\t\t.asm\n\t\t.nsfe\n\t\t.txt\n";
			helpmessage += "nodump\t: disables the crash dump generation, for cases where these are undesirable\n";
			helpmessage += "log\t: enables the register logger, available in debug builds only\n";
			helpmessage += "Press enter to continue . . .";
			fprintf(stdout, "%s\n", helpmessage.c_str());
			fclose(cout);
			return;
		}
	}
	else {
		// Store NSF name, then log filename
		if (m_bExport == true) {
			if (m_strExportFile.GetLength() == 0)
			{
				m_strExportFile = CString(pszParam);
				return;
			}
			else if(m_strExportLogFile.GetLength() == 0)
			{
				m_strExportLogFile = CString(pszParam);
				return;
			}
			else if(m_strExportDPCMFile.GetLength() == 0)
			{
				// BIN export takes another file paramter for DPCM
				m_strExportDPCMFile = CString(pszParam);
				return;
			}
		}
	}
}

//
// CDocTemplate0CC class
//

// copied from http://support.microsoft.com/en-us/kb/141921

CDocTemplate0CC::CDocTemplate0CC(UINT nIDResource, CRuntimeClass *pDocClass, CRuntimeClass *pFrameClass, CRuntimeClass *pViewClass) :
	CSingleDocTemplate(nIDResource, pDocClass, pFrameClass, pViewClass)
{
}

BOOL CDocTemplate0CC::GetDocString(CString &rString, DocStringIndex i) const
{
	CString strTemp, strLeft, strRight;
	int nFindPos;
	AfxExtractSubString(strTemp, m_strDocStrings, (int)i);
	if (i == CDocTemplate::filterExt) {
		nFindPos = strTemp.Find(';');
		if (-1 != nFindPos) {
			//string contains two extensions
			strLeft = strTemp.Left(nFindPos + 1);
			strRight = strTemp.Right(lstrlen((const char*)strTemp) - nFindPos - 1);
			strTemp = strLeft + strRight;
		}
	}
	rString = strTemp;
	return TRUE;
}

CDocTemplate::Confidence CDocTemplate0CC::MatchDocType(const char *pszPathName, CDocument *&rpDocMatch)
{
	ASSERT(pszPathName != NULL);
	rpDocMatch = NULL;

	// go through all documents
	POSITION pos = GetFirstDocPosition();
	while (pos != NULL) {
		CDocument* pDoc = GetNextDoc(pos);
		if (pDoc->GetPathName() == pszPathName) {
		   // already open
			rpDocMatch = pDoc;
			return yesAlreadyOpen;
		}
	}  // end while

	// // // see if it matches one of the suffixes
	CString strFilterExt;
	if (GetDocString(strFilterExt, CDocTemplate::filterExt) && !strFilterExt.IsEmpty()) {
		 // see if extension matches
		int nDot = CString(pszPathName).ReverseFind('.');
		int curPos = 0;
		CString tok = strFilterExt.Tokenize(_T(";"), curPos);
		while (!tok.IsEmpty()) {
			ASSERT(tok[0] == '.');
			if (nDot >= 0 && lstrcmpi(pszPathName + nDot, tok) == 0)
				return yesAttemptNative; // extension matches
			tok = strFilterExt.Tokenize(_T(";"), curPos);
		}
	}
	return yesAttemptForeign; //unknown document type
}

//
// CDocManager0CC class
//

BOOL CDocManager0CC::DoPromptFileName(CString &fileName, UINT nIDSTitle, DWORD lFlags, BOOL bOpenFileDialog, CDocTemplate *pTemplate)
{
	// Copied from MFC
	// // // disregard doc template
	CString path = theApp.GetSettings()->GetPath(PATH_FTM) + _T("\\");

	CFileDialog OpenFileDlg(bOpenFileDialog, _T("0cc"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							_T(APP_NAME " modules (*.dnm;*.0cc;*.ftm)|*.dnm; *.0cc; *.ftm|All files (*.*)|*.*||"),
							AfxGetMainWnd(), 0);
	OpenFileDlg.m_ofn.Flags |= lFlags;
	OpenFileDlg.m_ofn.lpstrFile = fileName.GetBuffer(_MAX_PATH);
	OpenFileDlg.m_ofn.lpstrInitialDir = path.GetBuffer(_MAX_PATH);
	CString title;
	ENSURE(title.LoadString(nIDSTitle));
	OpenFileDlg.m_ofn.lpstrTitle = title;
	INT_PTR nResult = OpenFileDlg.DoModal();
	fileName.ReleaseBuffer();
	path.ReleaseBuffer();

	if (nResult == IDOK) {
		theApp.GetSettings()->SetPath(fileName, PATH_FTM);
		return true;
	}
	return false;
}
