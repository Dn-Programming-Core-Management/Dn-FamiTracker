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

/*

 Document file version changes

 Ver 4.0
  - Header block, added song names

 Ver 3.0
  - Sequences are stored in the way they are represented in the instrument editor
  - Added separate speed and tempo settings
  - Changed automatic portamento to 3xx and added 1xx & 2xx portamento

 Ver 2.1
  - Made some additions to support multiple effect columns and prepared for more channels
  - Made some speed adjustments, increase speed effects by one if it's below 20

 Ver 2.0
  - Files are small

*/

#include <memory>		// // //
#include "stdafx.h"
#include <algorithm>
#include <vector>		// // //
#include <string>		// // //
#include <unordered_map>		// // //
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "ModuleException.h"		// // //
#include "TrackerChannel.h"
#include "MainFrm.h"
#include "DocumentFile.h"
#include "Settings.h"
#include "SoundGen.h"
#include "ChannelMap.h"
#include "SequenceCollection.h"		// // //
#include "SequenceManager.h"		// // //
#include "DSampleManager.h"			// // //
#include "InstrumentManager.h"		// // //
#include "APU/APU.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Defaults when creating new modules
const char* CFamiTrackerDoc::DEFAULT_TRACK_NAME = "New song";
const int	CFamiTrackerDoc::DEFAULT_ROW_COUNT	= 64;

const char* CFamiTrackerDoc::NEW_INST_NAME = "New instrument";

// Make 1 channel default since 8 sounds bad
const int	CFamiTrackerDoc::DEFAULT_NAMCO_CHANS = 1;

const int	CFamiTrackerDoc::DEFAULT_FIRST_HIGHLIGHT = 4;
const int	CFamiTrackerDoc::DEFAULT_SECOND_HIGHLIGHT = 16;
const stHighlight CFamiTrackerDoc::DEFAULT_HIGHLIGHT = {DEFAULT_FIRST_HIGHLIGHT, DEFAULT_SECOND_HIGHLIGHT, 0};		// // //

const bool	CFamiTrackerDoc::DEFAULT_LINEAR_PITCH = false;

// File I/O constants
static const char *FILE_HEADER				= "FamiTracker Module";
static const char *FILE_BLOCK_PARAMS		= "PARAMS";
static const char *FILE_BLOCK_INFO			= "INFO";
static const char *FILE_BLOCK_INSTRUMENTS	= "INSTRUMENTS";
static const char *FILE_BLOCK_SEQUENCES		= "SEQUENCES";
static const char *FILE_BLOCK_FRAMES		= "FRAMES";
static const char *FILE_BLOCK_PATTERNS		= "PATTERNS";
static const char *FILE_BLOCK_DSAMPLES		= "DPCM SAMPLES";
static const char *FILE_BLOCK_HEADER		= "HEADER";
static const char *FILE_BLOCK_COMMENTS		= "COMMENTS";

// VRC6
static const char *FILE_BLOCK_SEQUENCES_VRC6 = "SEQUENCES_VRC6";

// N163
static const char *FILE_BLOCK_SEQUENCES_N163 = "SEQUENCES_N163";
static const char *FILE_BLOCK_SEQUENCES_N106 = "SEQUENCES_N106";

// Sunsoft
static const char *FILE_BLOCK_SEQUENCES_S5B = "SEQUENCES_S5B";

// // // 0CC-FamiTracker specific
const char *FILE_BLOCK_DETUNETABLES			= "DETUNETABLES";
const char *FILE_BLOCK_GROOVES				= "GROOVES";
const char *FILE_BLOCK_BOOKMARKS			= "BOOKMARKS";

// FTI instruments files
static const char INST_HEADER[] = "FTI";
static const char INST_VERSION[] = "2.4";

/* 
	Instrument version history
	 * 2.1 - Release points for sequences in 2A03 & VRC6
	 * 2.2 - FDS volume sequences goes from 0-31 instead of 0-15
	 * 2.3 - Support for release points & extra setting in sequences, 2A03 & VRC6
	 * 2.4 - DPCM delta counter setting
*/

// Structures for loading older versions of files

struct stInstrumentImport {
	char Name[256];
	bool Free;
	int	 ModEnable[SEQ_COUNT];
	int	 ModIndex[SEQ_COUNT];
	int	 AssignedSample;				// For DPCM
};

struct stSequenceImport {
	signed char Length[64];	// locked to 64
	signed char Value[64];
	int	Count;
};

struct stDSampleImport {
	char *SampleData;
	int	 SampleSize;
	char Name[256];
};

struct stChanNoteImport {
	int	Note;
	int	Octave;
	int	Vol;
	int	Instrument;
	int	ExtraStuff1;
	int	ExtraStuff2;
};

// File blocks

enum {
	FB_INSTRUMENTS,
	FB_SEQUENCES,
	FB_PATTERN_ROWS,
	FB_PATTERNS,
	FB_SPEED,
	FB_CHANNELS,
	FB_DSAMPLES,
	FB_EOF,
	FB_MACHINE,
	FB_ENGINESPEED,
	FB_SONGNAME,
	FB_SONGARTIST,
	FB_SONGCOPYRIGHT
};

//
// CFamiTrackerDoc
//

IMPLEMENT_DYNCREATE(CFamiTrackerDoc, CDocument)

BEGIN_MESSAGE_MAP(CFamiTrackerDoc, CDocument)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
END_MESSAGE_MAP()

// CFamiTrackerDoc construction/destruction

CFamiTrackerDoc::CFamiTrackerDoc() : 
	m_bFileLoaded(false), 
	m_bFileLoadFailed(false), 
	m_iRegisteredChannels(0), 
	m_iNamcoChannels(0),		// // //
	m_bDisplayComment(false)
{
	// Initialize document object

	ResetDetuneTables();		// // //

	// Clear pointer arrays
	memset(m_pTracks, 0, sizeof(CPatternData*) * MAX_TRACKS);
	memset(m_pGrooveTable, 0, sizeof(CGroove*) * MAX_GROOVE);		// // //
	memset(m_pBookmarkList, 0, sizeof(std::vector<stBookmark>*) * MAX_TRACKS);		// // //
	m_pInstrumentManager = new CInstrumentManager();

	// Register this object to the sound generator
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen)
		pSoundGen->AssignDocument(this);
}

CFamiTrackerDoc::~CFamiTrackerDoc()
{
	// Clean up

	// Patterns
	for (int i = 0; i < MAX_TRACKS; ++i) {
		SAFE_RELEASE(m_pTracks[i]);
	}

	// // // Grooves
	for (int i = 0; i < MAX_GROOVE; ++i)
		SAFE_RELEASE(m_pGrooveTable[i]);

	// // // Bookmarks
	for (int i = 0; i < MAX_TRACKS; ++i)
		ClearBookmarkList(i);
	
	m_pInstrumentManager->ClearAll();		// // //
	SAFE_RELEASE(m_pInstrumentManager);
}

//
// Static functions
//

CFamiTrackerDoc *CFamiTrackerDoc::GetDoc()
{
	CFrameWnd *pFrame = static_cast<CFrameWnd*>(AfxGetApp()->m_pMainWnd);
	ASSERT_VALID(pFrame);

	return static_cast<CFamiTrackerDoc*>(pFrame->GetActiveDocument());
}

// Synchronization
BOOL CFamiTrackerDoc::LockDocument() const
{
	return m_csDocumentLock.Lock();
}

BOOL CFamiTrackerDoc::LockDocument(DWORD dwTimeout) const
{
	return m_csDocumentLock.Lock(dwTimeout);
}

BOOL CFamiTrackerDoc::UnlockDocument() const
{
	return m_csDocumentLock.Unlock();
}

//
// Overrides
//

BOOL CFamiTrackerDoc::OnNewDocument()
{
	// Called by the GUI to create a new file

	// This calls DeleteContents
	if (!CDocument::OnNewDocument())
		return FALSE;

	CreateEmpty();

	return TRUE;
}

BOOL CFamiTrackerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	// This function is called by the GUI to load a file

	//DeleteContents();
	theApp.GetSoundGenerator()->ResetDumpInstrument();
	theApp.GetSoundGenerator()->SetRecordChannel(-1);		// // //

	m_csDocumentLock.Lock();

	// Load file
	if (!OpenDocument(lpszPathName)) {
		// Loading failed, create empty document
		m_csDocumentLock.Unlock();
		/*
		DeleteContents();
		CreateEmpty();
		for (int i = UPDATE_TRACK; i <= UPDATE_COLUMNS; ++i)		// // // test
			UpdateAllViews(NULL, i);
		*/
		// and tell doctemplate that loading failed
		return FALSE;
	}

	m_csDocumentLock.Unlock();

	// Update main frame
	ApplyExpansionChip();

#ifdef AUTOSAVE
	SetupAutoSave();
#endif

	// Remove modified flag
	SetModifiedFlag(FALSE);

	SetExceededFlag(FALSE);		// // //

	return TRUE;
}

BOOL CFamiTrackerDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
#ifdef DISABLE_SAVE		// // //
	static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_DISABLE_SAVE);
	return FALSE;
#endif

	// This function is called by the GUI to save the file

	if (!m_bFileLoaded)
		return FALSE;

	// File backup, now performed on save instead of open
	if ((m_bForceBackup || theApp.GetSettings()->General.bBackups) && !m_bBackupDone) {
		CString BakName;
		BakName.Format(_T("%s.bak"), lpszPathName);
		CopyFile(lpszPathName, BakName.GetBuffer(), FALSE);
		m_bBackupDone = true;
	}

	if (!SaveDocument(lpszPathName))
		return FALSE;

	// Reset modified flag
	SetModifiedFlag(FALSE);

	SetExceededFlag(FALSE);		// // //

	return TRUE;
}

void CFamiTrackerDoc::OnCloseDocument()
{	
	// Document object is about to be deleted

	// Remove itself from sound generator
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen)
		pSoundGen->RemoveDocument();

	CDocument::OnCloseDocument();
}

void CFamiTrackerDoc::DeleteContents()
{
	// Current document is being unloaded, clear and reset variables and memory
	// Delete everything because the current object is being reused in SDI

	// Make sure player is stopped
	theApp.StopPlayerAndWait();

	m_csDocumentLock.Lock();

	// Mark file as unloaded
	m_bFileLoaded = false;
	m_bForceBackup = false;
	m_bBackupDone = true;	// No backup on new modules

	UpdateAllViews(NULL, UPDATE_CLOSE);	// TODO remove

	m_pInstrumentManager->ClearAll();		// // //

	// // // Grooves
	for (int i = 0; i < MAX_GROOVE; ++i)
		SAFE_RELEASE(m_pGrooveTable[i]);

	// // // Bookmarks
	for (int i = 0; i < MAX_TRACKS; ++i)
		ClearBookmarkList(i);

	// Clear number of tracks
	m_iTrackCount = 1;

	// Delete all patterns
	for (int i = 0; i < MAX_TRACKS; ++i) {
		SAFE_RELEASE(m_pTracks[i]);
		m_sTrackNames[i].Empty();
	}

	// Clear song info
	memset(m_strName, 0, 32);
	memset(m_strArtist, 0, 32);
	memset(m_strCopyright, 0, 32);

	// Reset variables to default
	m_iMachine			 = DEFAULT_MACHINE_TYPE;
	m_iEngineSpeed		 = 0;
	m_iExpansionChip	 = SNDCHIP_NONE;
	m_iVibratoStyle		 = VIBRATO_OLD;
	m_bLinearPitch		 = DEFAULT_LINEAR_PITCH;
	m_iChannelsAvailable = CHANNELS_DEFAULT;
	m_iSpeedSplitPoint	 = DEFAULT_SPEED_SPLIT_POINT;

	m_vHighlight = DEFAULT_HIGHLIGHT;		// // //

	ResetDetuneTables();		// // //

	// Used for loading older files
	m_vSequences.RemoveAll();
	m_vTmpSequences.RemoveAll();

	// Auto save
#ifdef AUTOSAVE
	ClearAutoSave();
#endif

	m_strComment.Empty();
	m_bDisplayComment = false;

	// Remove modified flag
	SetModifiedFlag(FALSE);
	SetExceededFlag(FALSE);		// // //

	m_csDocumentLock.Unlock();

	CDocument::DeleteContents();
}

void CFamiTrackerDoc::SetModifiedFlag(BOOL bModified)
{
	// Trigger auto-save in 10 seconds
#ifdef AUTOSAVE
	if (bModified)
		m_iAutoSaveCounter = 10;
#endif

	BOOL bWasModified = IsModified();
	CDocument::SetModifiedFlag(bModified);
	
	CFrameWnd *pFrameWnd = dynamic_cast<CFrameWnd*>(theApp.m_pMainWnd);
	if (pFrameWnd != NULL) {
		if (pFrameWnd->GetActiveDocument() == this && bWasModified != bModified) {
			pFrameWnd->OnUpdateFrameTitle(TRUE);
		}
	}
}

void CFamiTrackerDoc::CreateEmpty()
{
	m_csDocumentLock.Lock();

	// Allocate first song
	AllocateTrack(0);

	// Auto-select new style vibrato for new modules
	m_iVibratoStyle = VIBRATO_NEW;
	m_bLinearPitch = DEFAULT_LINEAR_PITCH;

	m_iNamcoChannels = 0;		// // //

	// and select 2A03 only
	SelectExpansionChip(SNDCHIP_NONE);

#ifdef AUTOSAVE
	SetupAutoSave();
#endif

	SetModifiedFlag(FALSE);

	// Document is avaliable
	m_bFileLoaded = true;

	m_csDocumentLock.Unlock();

	theApp.GetSoundGenerator()->DocumentPropertiesChanged(this);
}

//
// Messages
//

void CFamiTrackerDoc::OnFileSave()
{
#ifdef DISABLE_SAVE		// // //
	static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	if (GetPathName().GetLength() == 0)
		OnFileSaveAs();
	else
		CDocument::OnFileSave();
}

void CFamiTrackerDoc::OnFileSaveAs()
{
#ifdef DISABLE_SAVE		// // //
	static_cast<CFrameWnd*>(AfxGetMainWnd())->SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	// Overloaded in order to save the ftm-path
	CString newName = GetPathName();

	if (!theApp.DoPromptFileName(newName, theApp.GetSettings()->GetPath(PATH_FTM) + _T("\\"), AFX_IDS_SAVEFILE,
		OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, FALSE, NULL))		// // // overridden
		return;

	theApp.GetSettings()->SetPath(newName, PATH_FTM);
	
	DoSave(newName);
}

// CFamiTrackerDoc serialization (never used)

void CFamiTrackerDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CFamiTrackerDoc diagnostics

#ifdef _DEBUG
void CFamiTrackerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFamiTrackerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CFamiTrackerDoc commands

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File load / save routines
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Functions for compability with older file versions

void CFamiTrackerDoc::ReorderSequences()
{
	int Slots[SEQ_COUNT] = {0, 0, 0, 0, 0};
	int Indices[MAX_SEQUENCES][SEQ_COUNT];

	memset(Indices, 0xFF, MAX_SEQUENCES * SEQ_COUNT * sizeof(int));

	m_vSequences.SetSize(MAX_SEQUENCES);

	// Organize sequences
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (auto pInst = std::dynamic_pointer_cast<CInstrument2A03>(m_pInstrumentManager->GetInstrument(i))) {		// // //
			for (int j = 0; j < SEQ_COUNT; ++j) {
				if (pInst->GetSeqEnable(j)) {
					int Index = pInst->GetSeqIndex(j);
					if (Indices[Index][j] >= 0 && Indices[Index][j] != -1) {
						pInst->SetSeqIndex(j, Indices[Index][j]);
					}
					else {
						memcpy(&m_vSequences[Slots[j]][j], &m_vTmpSequences[Index], sizeof(stSequence));
						for (unsigned int k = 0; k < m_vSequences[Slots[j]][j].Count; ++k) {
							switch (j) {
								case SEQ_VOLUME: 
									m_vSequences[Slots[j]][j].Value[k] = std::max<int>(m_vSequences[Slots[j]][j].Value[k], 0);
									m_vSequences[Slots[j]][j].Value[k] = std::min<int>(m_vSequences[Slots[j]][j].Value[k], 15);
									break;
								case SEQ_DUTYCYCLE: 
									m_vSequences[Slots[j]][j].Value[k] = std::max<int>(m_vSequences[Slots[j]][j].Value[k], 0);
									m_vSequences[Slots[j]][j].Value[k] = std::min<int>(m_vSequences[Slots[j]][j].Value[k], 3);
									break;
							}
						}
						Indices[Index][j] = Slots[j];
						pInst->SetSeqIndex(j, Slots[j]);
						Slots[j]++;
					}
				}
				else
					pInst->SetSeqIndex(j, 0);
			}
		}
	}

	// De-allocate memory
	m_vTmpSequences.RemoveAll();
}

void CFamiTrackerDoc::ConvertSequences()
{
	// This function is used to convert the old type sequences to new type

	for (int  i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			stSequence *pOldSeq = &m_vSequences[i][j];
			CSequence *pNewSeq = GetSequence(INST_2A03, i, j);
			ConvertSequence(pOldSeq, pNewSeq, j);
		}
	}

	// De-allocate memory
	m_vSequences.RemoveAll();
}

void CFamiTrackerDoc::AssertFileData(bool Cond, std::string Msg) const
{
	if (!Cond) {
		CModuleException *e = m_pCurrentDocument ? m_pCurrentDocument->GetException() : new CModuleException();
		e->AppendError(Msg);
		e->Raise();
	}
}

/*** File format description ***

0000: "FamiTracker Module"					id string
000x: Version								int, version number
000x: Start of blocks

 {FILE_BLOCK_PARAMS, 2}
  Expansion chip							char
  Channels									int
  Machine type								int
  Engine speed								int

 {FILE_BLOCK_INFO, 1}
  Song name									string, 32 bytes
  Artist name								string, 32 bytes
  Copyright									string, 32 bytes

000x: End of blocks
000x: "END"						End of file

********************************/


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document store functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerDoc::SaveDocument(LPCTSTR lpszPathName) const
{
	CDocumentFile DocumentFile;
	m_pCurrentDocument = &DocumentFile;		// // //
	CFileException ex;
	TCHAR TempPath[MAX_PATH], TempFile[MAX_PATH];
	ULONGLONG FileSize;

	// First write to a temp file (if saving fails, the original is not destroyed)
	GetTempPath(MAX_PATH, TempPath);
	GetTempFileName(TempPath, _T("FTM"), 0, TempFile);

	if (!DocumentFile.Open(TempFile, CFile::modeWrite | CFile::modeCreate, &ex)) {
		// Could not open file
		TCHAR szCause[255];
		CString strFormatted;
		ex.GetErrorMessage(szCause, 255);
		AfxFormatString1(strFormatted, IDS_SAVE_FILE_ERROR, szCause);
		AfxMessageBox(strFormatted, MB_OK | MB_ICONERROR);
		m_pCurrentDocument = nullptr;		// // //
		return FALSE;
	}

	DocumentFile.BeginDocument();

	if (!WriteBlocks(&DocumentFile)) {
		// The save process failed, delete temp file
		DocumentFile.Close();
		DeleteFile(TempFile);
		// Display error
		CString	ErrorMsg;
		ErrorMsg.LoadString(IDS_SAVE_ERROR);
		AfxMessageBox(ErrorMsg, MB_OK | MB_ICONERROR);
		m_pCurrentDocument = nullptr;		// // //
		return FALSE;
	}

	DocumentFile.EndDocument();

	FileSize = DocumentFile.GetLength();

	DocumentFile.Close();
	m_pCurrentDocument = nullptr;		// // //

	// Save old creation date
	HANDLE hOldFile;
	FILETIME creationTime;

	hOldFile = CreateFile(lpszPathName, FILE_READ_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	GetFileTime(hOldFile, &creationTime, NULL, NULL);
	CloseHandle(hOldFile);

	// Everything is done and the program cannot crash at this point
	// Replace the original
	if (!MoveFileEx(TempFile, lpszPathName, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
		// Display message if saving failed
		TCHAR *lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
		CString	strFormatted;
		AfxFormatString1(strFormatted, IDS_SAVE_FILE_ERROR, lpMsgBuf);
		AfxMessageBox(strFormatted, MB_OK | MB_ICONERROR);
		LocalFree(lpMsgBuf);
		// Remove temp file
		DeleteFile(TempFile);
		return FALSE;
	}

	// Restore creation date
	hOldFile = CreateFile(lpszPathName, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	SetFileTime(hOldFile, &creationTime, NULL, NULL);
	CloseHandle(hOldFile);

	// Todo: avoid calling the main window from document class
	CMainFrame *pMainFrame = static_cast<CMainFrame*>(AfxGetMainWnd());
	if (pMainFrame != NULL) {
		CString text;
		AfxFormatString1(text, IDS_FILE_SAVED, MakeIntString(static_cast<int>(FileSize)));
		pMainFrame->SetMessageText(text);
	}

	return TRUE;
}

bool CFamiTrackerDoc::WriteBlocks(CDocumentFile *pDocFile) const
{
	static const int DEFAULT_BLOCK_VERSION[] = {		// // // TODO: use version info
#ifdef TRANSPOSE_FDS
		6, 1, 3, 6, 6, 3, 5, 1, 1,	// internal
#else
		6, 1, 3, 6, 6, 3, 4, 1, 1,
#endif
		6, 1, 1,					// expansion
		1, 1, 1						// 0cc-ft
	};

	static bool (CFamiTrackerDoc::*FTM_WRITE_FUNC[])(CDocumentFile*, const int) const = {		// // //
		&CFamiTrackerDoc::WriteBlock_Parameters,
		&CFamiTrackerDoc::WriteBlock_SongInfo,
		&CFamiTrackerDoc::WriteBlock_Header,
		&CFamiTrackerDoc::WriteBlock_Instruments,
		&CFamiTrackerDoc::WriteBlock_Sequences,
		&CFamiTrackerDoc::WriteBlock_Frames,
		&CFamiTrackerDoc::WriteBlock_Patterns,
		&CFamiTrackerDoc::WriteBlock_DSamples,
		&CFamiTrackerDoc::WriteBlock_Comments,
		&CFamiTrackerDoc::WriteBlock_SequencesVRC6,		// // //
		&CFamiTrackerDoc::WriteBlock_SequencesN163,
		&CFamiTrackerDoc::WriteBlock_SequencesS5B,
		&CFamiTrackerDoc::WriteBlock_DetuneTables,		// // //
		&CFamiTrackerDoc::WriteBlock_Grooves,			// // //
		&CFamiTrackerDoc::WriteBlock_Bookmarks,			// // //
	};

	for (size_t i = 0; i < sizeof(FTM_WRITE_FUNC) / sizeof(*FTM_WRITE_FUNC); ++i) {
		if (!CALL_MEMBER_FN(this, FTM_WRITE_FUNC[i])(pDocFile, DEFAULT_BLOCK_VERSION[i]))
			return false;
	}
	return true;
}

bool CFamiTrackerDoc::WriteBlock_Parameters(CDocumentFile *pDocFile, const int Version) const
{
	// Module parameters
	pDocFile->CreateBlock(FILE_BLOCK_PARAMS, Version);
	
	pDocFile->WriteBlockChar(m_iExpansionChip);		// ver 2 change
	pDocFile->WriteBlockInt(m_iChannelsAvailable);
	pDocFile->WriteBlockInt(static_cast<int>(m_iMachine));
	pDocFile->WriteBlockInt(m_iEngineSpeed);
	pDocFile->WriteBlockInt(m_iVibratoStyle);		// ver 3 change
	// TODO write m_bLinearPitch
	pDocFile->WriteBlockInt(m_vHighlight.First);		// // // ver 4 change
	pDocFile->WriteBlockInt(m_vHighlight.Second);

	if (ExpansionEnabled(SNDCHIP_N163))
		pDocFile->WriteBlockInt(m_iNamcoChannels);	// ver 5 change

	pDocFile->WriteBlockInt(m_iSpeedSplitPoint);	// ver 6 change

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_SongInfo(CDocumentFile *pDocFile, const int Version) const
{
	// Song info
	pDocFile->CreateBlock(FILE_BLOCK_INFO, Version);
	
	pDocFile->WriteBlock(m_strName, 32);
	pDocFile->WriteBlock(m_strArtist, 32);
	pDocFile->WriteBlock(m_strCopyright, 32);

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Header(CDocumentFile *pDocFile, const int Version) const
{
	/* 
	 *  Header data
 	 *
	 *  Store song count and then for each channel: 
	 *  channel type and number of effect columns
	 *
	 */

	// Version 3 adds song names

	// Header data
	pDocFile->CreateBlock(FILE_BLOCK_HEADER, Version);

	// Write number of tracks
	pDocFile->WriteBlockChar(m_iTrackCount - 1);

	// Ver 3, store track names
	for (unsigned int i = 0; i < m_iTrackCount; ++i) {
		pDocFile->WriteString(m_sTrackNames[i]);
	}

	for (unsigned int i = 0; i < m_iChannelsAvailable; ++i) {
		// Channel type
		pDocFile->WriteBlockChar(m_iChannelTypes[i]);
		for (unsigned int j = 0; j < m_iTrackCount; ++j) {
			ASSERT(m_pTracks[j] != NULL);
			// Effect columns
			pDocFile->WriteBlockChar(m_pTracks[j]->GetEffectColumnCount(i));
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Instruments(CDocumentFile *pDocFile, const int Version) const
{
	// A bug in v0.3.0 causes a crash if this is not 2, so change only when that ver is obsolete!
	//
	// Log:
	// - v6: adds DPCM delta settings
	//

	// If FDS is used then version must be at least 4 or recent files won't load

	// Fix for FDS instruments
/*	if (m_iExpansionChip & SNDCHIP_FDS)
		Version = 4;

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (m_pInstrumentManager->GetInstrument(i)->GetType() == INST_FDS)
			Version = 4;
	}
*/
	char Name[CInstrument::INST_NAME_MAX];

	// Instruments block
	const int Count = m_pInstrumentManager->GetInstrumentCount();
	if (!Count) return true;		// // //
	pDocFile->CreateBlock(FILE_BLOCK_INSTRUMENTS, Version);
	pDocFile->WriteBlockInt(Count);

	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		// Only write instrument if it's used
		if (auto pInst = m_pInstrumentManager->GetInstrument(i)) {
			// Write index and type
			pDocFile->WriteBlockInt(i);
			pDocFile->WriteBlockChar(static_cast<char>(pInst->GetType()));

			// Store the instrument
			pInst->Store(pDocFile);

			// Store the name
			pInst->GetName(Name);
			pDocFile->WriteBlockInt((int)strlen(Name));
			pDocFile->WriteBlock(Name, (int)strlen(Name));			
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Sequences(CDocumentFile *pDocFile, const int Version) const
{
	/* 
	 * Store 2A03 sequences
	 */ 

	int Count = 0;

	// Count number of used sequences
	for (int i = 0; i < MAX_SEQUENCES; ++i)
		for (int j = 0; j < SEQ_COUNT; ++j)
			if (GetSequenceItemCount(INST_2A03, i, j) > 0)
				Count++;

	if (!Count) return true;		// // //
	// Sequences, version 6
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES, Version);
	pDocFile->WriteBlockInt(Count);
	
	CSequenceManager *pManager = GetSequenceManager(INST_2A03);		// // //

	for (int i = 0; i < SEQ_COUNT; ++i) {
		const CSequenceCollection *pCol = pManager->GetCollection(i);
		for (int j = 0; j < MAX_SEQUENCES; ++j) {
			const CSequence *pSeq = pCol->GetSequence(j);
			Count = pSeq->GetItemCount();
			if (pSeq != nullptr && Count) {
				// Store index
				pDocFile->WriteBlockInt(j);
				// Store type of sequence
				pDocFile->WriteBlockInt(i);
				// Store number of items in this sequence
				pDocFile->WriteBlockChar(Count);
				// Store loop point
				pDocFile->WriteBlockInt(pSeq->GetLoopPoint());
				// Store items
				for (int k = 0; k < Count; ++k) {
					pDocFile->WriteBlockChar(pSeq->GetItem(k));
				}
			}
		}
	}

	// v6
	for (int i = 0; i < SEQ_COUNT; ++i) {
		const CSequenceCollection *pCol = pManager->GetCollection(i);
		for (int j = 0; j < MAX_SEQUENCES; ++j) {
			const CSequence *pSeq = pCol->GetSequence(j);
			Count = pSeq->GetItemCount();
			if (pSeq != nullptr && Count) {
				// Store release point
				pDocFile->WriteBlockInt(pSeq->GetReleasePoint());
				// Store setting
				pDocFile->WriteBlockInt(pSeq->GetSetting());
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_SequencesVRC6(CDocumentFile *pDocFile, const int Version) const
{
	/* 
	 * Store VRC6 sequences
	 */ 

	int Count = 0;

	// Count number of used sequences
	for (int i = 0; i < MAX_SEQUENCES; ++i)
		for (int j = 0; j < SEQ_COUNT; ++j)
			if (GetSequenceItemCount(INST_VRC6, i, j) > 0)
				Count++;

	if (!Count) return true;		// // //
	// Sequences, version 6
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES_VRC6, Version);

	// Write it
	pDocFile->WriteBlockInt(Count);

	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			Count = GetSequenceItemCount(INST_VRC6, i, j);

			if (Count > 0) {
				const CSequence *pSeq = GetSequence(INST_VRC6, i, j);

				// Store index
				pDocFile->WriteBlockInt(i);
				// Store type of sequence
				pDocFile->WriteBlockInt(j);
				// Store number of items in this sequence
				pDocFile->WriteBlockChar(Count);
				// Store loop point
				pDocFile->WriteBlockInt(pSeq->GetLoopPoint());
				// Store items
				for (int k = 0; k < Count; ++k) {
					pDocFile->WriteBlockChar(pSeq->GetItem(k));
				}
			}
		}
	}

	// v6
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			Count = GetSequenceItemCount(INST_VRC6, i, j);

			if (Count > 0) {
				const CSequence *pSeq = GetSequence(INST_VRC6, i, j);

				// Store release point
				pDocFile->WriteBlockInt(pSeq->GetReleasePoint());
				// Store setting
				pDocFile->WriteBlockInt(pSeq->GetSetting());
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_SequencesN163(CDocumentFile *pDocFile, const int Version) const
{
	/* 
	 * Store N163 sequences
	 */ 

	int Count = 0;

	// Count number of used sequences
	for (int i = 0; i < MAX_SEQUENCES; ++i)
		for (int j = 0; j < SEQ_COUNT; ++j)
			if (GetSequenceItemCount(INST_N163, i, j) > 0)
				Count++;

	if (!Count) return true;		// // //
	// Sequences, version 0
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES_N163, Version);

	// Write it
	pDocFile->WriteBlockInt(Count);

	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			Count = GetSequenceItemCount(INST_N163, i, j);

			if (Count > 0) {
				const CSequence *pSeq = GetSequence(INST_N163, i, j);

				// Store index
				pDocFile->WriteBlockInt(i);
				// Store type of sequence
				pDocFile->WriteBlockInt(j);
				// Store number of items in this sequence
				pDocFile->WriteBlockChar(Count);
				// Store loop point
				pDocFile->WriteBlockInt(pSeq->GetLoopPoint());
				// Store release point
				pDocFile->WriteBlockInt(pSeq->GetReleasePoint());
				// Store setting
				pDocFile->WriteBlockInt(pSeq->GetSetting());
				// Store items
				for (int k = 0; k < Count; ++k) {
					pDocFile->WriteBlockChar(pSeq->GetItem(k));
				}
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_SequencesS5B(CDocumentFile *pDocFile, const int Version) const
{
	/* 
	 * Store 5B sequences
	 */ 

	int Count = 0;

	// Count number of used sequences
	for (int i = 0; i < MAX_SEQUENCES; ++i)
		for (int j = 0; j < SEQ_COUNT; ++j)
			if (GetSequenceItemCount(INST_S5B, i, j) > 0)
				Count++;

	if (!Count) return true;		// // //
	// Sequences, version 0
	pDocFile->CreateBlock(FILE_BLOCK_SEQUENCES_S5B, Version);

	// Write it
	pDocFile->WriteBlockInt(Count);

	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		for (int j = 0; j < SEQ_COUNT; ++j) {
			Count = GetSequenceItemCount(INST_S5B, i, j);

			if (Count > 0) {
				const CSequence *pSeq = GetSequence(INST_S5B, i, j);

				// Store index
				pDocFile->WriteBlockInt(i);
				// Store type of sequence
				pDocFile->WriteBlockInt(j);
				// Store number of items in this sequence
				pDocFile->WriteBlockChar(Count);
				// Store loop point
				pDocFile->WriteBlockInt(pSeq->GetLoopPoint());
				// Store release point
				pDocFile->WriteBlockInt(pSeq->GetReleasePoint());
				// Store setting
				pDocFile->WriteBlockInt(pSeq->GetSetting());
				// Store items
				for (int k = 0; k < Count; ++k) {
					pDocFile->WriteBlockChar(pSeq->GetItem(k));
				}
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Frames(CDocumentFile *pDocFile, const int Version) const
{
	/* Store frame count
	 *
	 * 1. Number of channels (5 for 2A03 only)
	 * 2. 
	 * 
	 */ 

	pDocFile->CreateBlock(FILE_BLOCK_FRAMES, Version);

	for (unsigned i = 0; i < m_iTrackCount; ++i) {
		CPatternData *pTune = m_pTracks[i];

		pDocFile->WriteBlockInt(pTune->GetFrameCount());
		pDocFile->WriteBlockInt(pTune->GetSongSpeed());
		pDocFile->WriteBlockInt(pTune->GetSongTempo());
		pDocFile->WriteBlockInt(pTune->GetPatternLength());

		for (unsigned int j = 0; j < pTune->GetFrameCount(); ++j) {
			for (unsigned k = 0; k < m_iChannelsAvailable; ++k) {
				pDocFile->WriteBlockChar((unsigned char)pTune->GetFramePattern(j, k));
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Patterns(CDocumentFile *pDocFile, const int Version) const
{
	/*
	 * Version changes: 
	 *
	 *  2: Support multiple tracks
	 *  3: Changed portamento effect
	 *  4: Switched portamento effects for VRC7 (1xx & 2xx), adjusted Pxx for FDS
	 *  5: Adjusted FDS octave
	 *  (6: Noise pitch slide effects fix)
	 *
	 */ 

	pDocFile->CreateBlock(FILE_BLOCK_PATTERNS, Version);

	stChanNote *Note;		// // //

	for (unsigned t = 0; t < m_iTrackCount; ++t) {
		for (unsigned i = 0; i < m_iChannelsAvailable; ++i) {
			for (unsigned x = 0; x < MAX_PATTERN; ++x) {
				unsigned Items = 0;

				// Save all rows
				unsigned int PatternLen = MAX_PATTERN_LENGTH;
				//unsigned int PatternLen = m_pTracks[t]->GetPatternLength();
				
				// Get the number of items in this pattern
				for (unsigned y = 0; y < PatternLen; ++y) {
					if (!m_pTracks[t]->IsCellFree(i, x, y))
						Items++;
				}

				if (Items > 0) {
					pDocFile->WriteBlockInt(t);		// Write track
					pDocFile->WriteBlockInt(i);		// Write channel
					pDocFile->WriteBlockInt(x);		// Write pattern
					pDocFile->WriteBlockInt(Items);	// Number of items

					for (unsigned y = 0; y < PatternLen; y++) {
						if (!m_pTracks[t]->IsCellFree(i, x, y)) {
							Note = m_pTracks[t]->GetPatternData(i, x, y);		// // //
							// AssertFileData(Note, "Cannot create note");
							pDocFile->WriteBlockInt(y);

							pDocFile->WriteBlockChar(Note->Note);
							pDocFile->WriteBlockChar(Note->Octave);
							pDocFile->WriteBlockChar(Note->Instrument);
							pDocFile->WriteBlockChar(Note->Vol);

							int EffColumns = (m_pTracks[t]->GetEffectColumnCount(i) + 1);

							for (int n = 0; n < EffColumns; n++) {
								pDocFile->WriteBlockChar(Note->EffNumber[n]);
								pDocFile->WriteBlockChar(Note->EffParam[n]);
							}
						}
					}
				}
			}
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_DSamples(CDocumentFile *pDocFile, const int Version) const
{
	int Count = GetSampleCount();		// // //
	if (!Count) return true;

	pDocFile->CreateBlock(FILE_BLOCK_DSAMPLES, Version);

	// Write sample count
	pDocFile->WriteBlockChar(Count);

	for (unsigned int i = 0; i < CDSampleManager::MAX_DSAMPLES; ++i) {
		if (const CDSample *pSamp = GetSample(i)) {
			// Write sample
			pDocFile->WriteBlockChar(i);
			int Length = strlen(pSamp->GetName());
			pDocFile->WriteBlockInt(Length);
			pDocFile->WriteBlock(pSamp->GetName(), Length);
			pDocFile->WriteBlockInt(pSamp->GetSize());
			pDocFile->WriteBlock(pSamp->GetData(), pSamp->GetSize());
		}
	}

	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_Comments(CDocumentFile *pDocFile, const int Version) const
{
	if (m_strComment.IsEmpty())
		return true;

	pDocFile->CreateBlock(FILE_BLOCK_COMMENTS, Version);
	pDocFile->WriteBlockInt(m_bDisplayComment ? 1 : 0);
	pDocFile->WriteString(m_strComment);
	return pDocFile->FlushBlock();
}

bool CFamiTrackerDoc::WriteBlock_ChannelLayout(CDocumentFile *pDocFile, const int Version) const
{
//	pDocFile->CreateBlock(FILE_CHANNEL_LAYOUT, Version);
	// Todo
	return pDocFile->FlushBlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Document load functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CFamiTrackerDoc::OpenDocument(LPCTSTR lpszPathName)
{
	CFileException ex;
	CDocumentFile  OpenFile;

	m_bBackupDone = false;
	m_bFileLoadFailed = true;

	// Open file
	if (!OpenFile.Open(lpszPathName, CFile::modeRead | CFile::shareDenyWrite, &ex)) {
		TCHAR   szCause[255];
		CString strFormatted;
		ex.GetErrorMessage(szCause, 255);
		strFormatted = _T("Could not open file.\n\n");
		strFormatted += szCause;
		AfxMessageBox(strFormatted);
		//OnNewDocument();
		return FALSE;
	}

	// Check if empty file
	if (OpenFile.GetLength() == 0) {
		// Setup default settings
		CreateEmpty();
		return TRUE;
	}
	
	m_pCurrentDocument = &OpenFile;		// // // closure
	try {		// // //
		// Read header ID and version
		OpenFile.ValidateFile();

		m_iFileVersion = OpenFile.GetFileVersion();

		if (m_iFileVersion < 0x0200U) {
			if (!OpenDocumentOld(&OpenFile))
				OpenFile.RaiseModuleException("General error");

			// Create a backup of this file, since it's an old version 
			// and something might go wrong when converting
			m_bForceBackup = true;

			// Auto-select old style vibrato for old files
			m_iVibratoStyle = VIBRATO_OLD;
			m_bLinearPitch = false;
		}
		else {
			if (!OpenDocumentNew(OpenFile))
				OpenFile.RaiseModuleException("General error");

			// Backup if files was of an older version
			m_bForceBackup = m_iFileVersion < CDocumentFile::FILE_VER;
		}
	}
	catch (CModuleException *e) {
		AfxMessageBox(e->GetErrorString().c_str(), MB_ICONERROR);
		m_pCurrentDocument = nullptr;		// // //
		delete e;
		return FALSE;
	}

	m_pCurrentDocument = nullptr;		// // //

#ifdef WIP
	// Force backups if compiled as beta
//	m_bForceBackup = true;
#endif

	// File is loaded
	m_bFileLoaded = true;
	m_bFileLoadFailed = false;

	theApp.GetSoundGenerator()->DocumentPropertiesChanged(this);

	return TRUE;
}

/**
 * This function reads the old obsolete file version. 
 */
BOOL CFamiTrackerDoc::OpenDocumentOld(CFile *pOpenFile)
{
	unsigned int i, c, ReadCount, FileBlock;

	// Delete loaded document
	DeleteContents();

	FileBlock = 0;

	// Only single track files
	CPatternData *pTrack = GetTrack(0);

	m_iVibratoStyle = VIBRATO_OLD;
	m_bLinearPitch = false;

	stInstrumentImport	ImportedInstruments;
	stSequenceImport	ImportedSequence;
	stDSampleImport		ImportedDSample;
	stChanNoteImport	ImportedNote;

	while (FileBlock != FB_EOF) {
		if (pOpenFile->Read(&FileBlock, sizeof(int)) == 0)
			FileBlock = FB_EOF;

		unsigned int Speed, FrameCount, Pattern, PatternLength;

		switch (FileBlock) {
			case FB_CHANNELS:
				pOpenFile->Read(&m_iChannelsAvailable, sizeof(int));
				break;

			case FB_SPEED:
				pOpenFile->Read(&Speed, sizeof(int));
				pTrack->SetSongSpeed(Speed + 1);
				break;

			case FB_MACHINE:
				pOpenFile->Read(&m_iMachine, sizeof(int));				
				break;

			case FB_ENGINESPEED:
				pOpenFile->Read(&m_iEngineSpeed, sizeof(int));
				break;

			case FB_INSTRUMENTS:
				pOpenFile->Read(&ReadCount, sizeof(int));
				if (ReadCount > MAX_INSTRUMENTS)
					ReadCount = MAX_INSTRUMENTS - 1;
				for (i = 0; i < ReadCount; i++) {
					pOpenFile->Read(&ImportedInstruments, sizeof(stInstrumentImport));
					if (ImportedInstruments.Free == false) {
						CInstrument2A03 *pInst = new CInstrument2A03();
						for (int j = 0; j < SEQ_COUNT; j++) {
							pInst->SetSeqEnable(j, ImportedInstruments.ModEnable[j]);
							pInst->SetSeqIndex(j, ImportedInstruments.ModIndex[j]);
						}
						pInst->SetName(ImportedInstruments.Name);

						if (ImportedInstruments.AssignedSample > 0) {
							int Pitch = 0;
							for (int y = 0; y < 6; y++) {
								for (int x = 0; x < 12; x++) {
									pInst->SetSample(y, x, ImportedInstruments.AssignedSample);
									pInst->SetSamplePitch(y, x, Pitch);
									Pitch = (Pitch + 1) % 16;
								}
							}
						}

						m_pInstrumentManager->InsertInstrument(i, pInst);		// // //
					}
				}
				break;

			case FB_SEQUENCES:
				pOpenFile->Read(&ReadCount, sizeof(int));
				m_vTmpSequences.SetSize(ReadCount);
				for (i = 0; i < ReadCount; i++) {
					pOpenFile->Read(&ImportedSequence, sizeof(stSequenceImport));
					if (ImportedSequence.Count > 0 && ImportedSequence.Count < MAX_SEQUENCE_ITEMS) {
						m_vTmpSequences[i].Count = ImportedSequence.Count;
						memcpy(m_vTmpSequences[i].Length, ImportedSequence.Length, ImportedSequence.Count);
						memcpy(m_vTmpSequences[i].Value, ImportedSequence.Value, ImportedSequence.Count);
					}
				}
				break;

			case FB_PATTERN_ROWS:
				pOpenFile->Read(&FrameCount, sizeof(int));
				pTrack->SetFrameCount(FrameCount);
				for (c = 0; c < FrameCount; c++) {
					for (i = 0; i < m_iChannelsAvailable; i++) {
						pOpenFile->Read(&Pattern, sizeof(int));
						pTrack->SetFramePattern(c, i, Pattern);
					}
				}
				break;

			case FB_PATTERNS:
				pOpenFile->Read(&ReadCount, sizeof(int));
				pOpenFile->Read(&PatternLength, sizeof(int));
				pTrack->SetPatternLength(PatternLength);
				for (unsigned int x = 0; x < m_iChannelsAvailable; x++) {
					for (c = 0; c < ReadCount; c++) {
						for (i = 0; i < PatternLength; i++) {
							pOpenFile->Read(&ImportedNote, sizeof(stChanNoteImport));
							if (ImportedNote.ExtraStuff1 == EF_PORTAOFF) {
								ImportedNote.ExtraStuff1 = EF_PORTAMENTO;
								ImportedNote.ExtraStuff2 = 0;
							}
							else if (ImportedNote.ExtraStuff1 == EF_PORTAMENTO) {
								if (ImportedNote.ExtraStuff2 < 0xFF)
									ImportedNote.ExtraStuff2++;
							}
							stChanNote *Note;
							Note = pTrack->GetPatternData(x, c, i);
							Note->EffNumber[0]	= static_cast<effect_t>(ImportedNote.ExtraStuff1);
							Note->EffParam[0]	= ImportedNote.ExtraStuff2;
							Note->Instrument	= ImportedNote.Instrument;
							Note->Note			= ImportedNote.Note;
							Note->Octave		= ImportedNote.Octave;
							Note->Vol			= 0;
							if (Note->Note == 0)
								Note->Instrument = MAX_INSTRUMENTS;
							if (Note->Vol == 0)
								Note->Vol = MAX_VOLUME;
						}
					}
				}
				break;

			case FB_DSAMPLES:
				pOpenFile->Read(&ReadCount, sizeof(int));
				for (i = 0; i < ReadCount; i++) {
					pOpenFile->Read(&ImportedDSample, sizeof(stDSampleImport));
					if (ImportedDSample.SampleSize != 0 && ImportedDSample.SampleSize < 0x4000) {
						ImportedDSample.SampleData = new char[ImportedDSample.SampleSize];
						pOpenFile->Read(ImportedDSample.SampleData, ImportedDSample.SampleSize);
					}
					else
						ImportedDSample.SampleData = NULL;
					CDSample *pSamp = new CDSample();		// // //
					pSamp->SetName(ImportedDSample.Name);
					pSamp->SetData(ImportedDSample.SampleSize, ImportedDSample.SampleData);
					SetSample(i, pSamp);
				}
				break;

			case FB_SONGNAME:
				pOpenFile->Read(m_strName, sizeof(char) * 32);
				break;

			case FB_SONGARTIST:
				pOpenFile->Read(m_strArtist, sizeof(char) * 32);
				break;
		
			case FB_SONGCOPYRIGHT:
				pOpenFile->Read(m_strCopyright, sizeof(char) * 32);
				break;
			
			default:
				FileBlock = FB_EOF;
		}
	}

	SetupChannels(m_iExpansionChip);

	ReorderSequences();
	ConvertSequences();

	pOpenFile->Close();

	return TRUE;
}

/**
 *  This function opens the most recent file version
 *
 */
BOOL CFamiTrackerDoc::OpenDocumentNew(CDocumentFile &DocumentFile)
{
	static std::unordered_map<std::string, bool (CFamiTrackerDoc::*)(CDocumentFile*)> FTM_READ_FUNC;
	FTM_READ_FUNC[FILE_BLOCK_PARAMS]			= &CFamiTrackerDoc::ReadBlock_Parameters;
	FTM_READ_FUNC[FILE_BLOCK_INFO]				= &CFamiTrackerDoc::ReadBlock_SongInfo;
	FTM_READ_FUNC[FILE_BLOCK_INSTRUMENTS]		= &CFamiTrackerDoc::ReadBlock_Instruments;
	FTM_READ_FUNC[FILE_BLOCK_SEQUENCES]			= &CFamiTrackerDoc::ReadBlock_Sequences;
	FTM_READ_FUNC[FILE_BLOCK_FRAMES]			= &CFamiTrackerDoc::ReadBlock_Frames;
	FTM_READ_FUNC[FILE_BLOCK_PATTERNS]			= &CFamiTrackerDoc::ReadBlock_Patterns;
	FTM_READ_FUNC[FILE_BLOCK_DSAMPLES]			= &CFamiTrackerDoc::ReadBlock_DSamples;
	FTM_READ_FUNC[FILE_BLOCK_HEADER]			= &CFamiTrackerDoc::ReadBlock_Header;
	FTM_READ_FUNC[FILE_BLOCK_COMMENTS]			= &CFamiTrackerDoc::ReadBlock_Comments;
	FTM_READ_FUNC[FILE_BLOCK_SEQUENCES_VRC6]	= &CFamiTrackerDoc::ReadBlock_SequencesVRC6;
	FTM_READ_FUNC[FILE_BLOCK_SEQUENCES_N163]	= &CFamiTrackerDoc::ReadBlock_SequencesN163;
	FTM_READ_FUNC[FILE_BLOCK_SEQUENCES_N106]	= &CFamiTrackerDoc::ReadBlock_SequencesN163;	// Backward compatibility
	FTM_READ_FUNC[FILE_BLOCK_SEQUENCES_S5B]		= &CFamiTrackerDoc::ReadBlock_SequencesS5B;		// // //
	FTM_READ_FUNC[FILE_BLOCK_DETUNETABLES]		= &CFamiTrackerDoc::ReadBlock_DetuneTables;		// // //
	FTM_READ_FUNC[FILE_BLOCK_GROOVES]			= &CFamiTrackerDoc::ReadBlock_Grooves;			// // //
	FTM_READ_FUNC[FILE_BLOCK_BOOKMARKS]			= &CFamiTrackerDoc::ReadBlock_Bookmarks;		// // //
	
	const char *BlockID;
	bool FileFinished = false;
	bool ErrorFlag = false;

#ifdef _DEBUG
	int _msgs_ = 0;
#endif

#ifdef TRANSPOSE_FDS
	m_bAdjustFDSArpeggio = false;
#endif

	// File version checking
	m_iFileVersion = DocumentFile.GetFileVersion();

	// Delete loaded document
	DeleteContents();

	if (m_iFileVersion < 0x0210) {
		// This has to be done for older files
		AllocateTrack(0);
	}

	// Read all blocks
	while (!DocumentFile.Finished() && !FileFinished && !ErrorFlag) {
		ErrorFlag = DocumentFile.ReadBlock();
		BlockID = DocumentFile.GetBlockHeaderID();

		if (!strcmp(BlockID, "END")) {
			FileFinished = true;
		}
		else {
			try {
				ErrorFlag = CALL_MEMBER_FN(this, FTM_READ_FUNC.at(BlockID))(&DocumentFile);
			}
			catch (std::out_of_range) {
			// This shouldn't show up in release (debug only)
#ifdef _DEBUG
				if (++_msgs_ < 5)
					AfxMessageBox(_T("Unknown file block!"));
#endif
				if (DocumentFile.IsFileIncomplete())
					ErrorFlag = true;
			}
		}
	}

	DocumentFile.Close();

	if (ErrorFlag) {
		AfxMessageBox(IDS_FILE_LOAD_ERROR, MB_ICONERROR);
		DeleteContents();
		return FALSE;
	}

	if (m_iFileVersion <= 0x0201)
		ReorderSequences();

	if (m_iFileVersion < 0x0300)
		ConvertSequences();

#ifdef TRANSPOSE_FDS
	if (m_bAdjustFDSArpeggio) {
		int Channel = GetChannelIndex(CHANID_FDS);
		if (Channel != -1) {
			stChanNote Note;
			for (int t = 0; t < m_iTrackCount; ++t) for (int p = 0; p < MAX_PATTERN; ++p) for (int r = 0; r < MAX_PATTERN_LENGTH; ++r) {
				GetDataAtPattern(t, p, Channel, r, &Note);
				if (Note.Note >= NOTE_C && Note.Note <= NOTE_B) {
					int Trsp = MIDI_NOTE(Note.Octave, Note.Note) + NOTE_RANGE * 2;
					Trsp = Trsp >= NOTE_COUNT ? NOTE_COUNT - 1 : Trsp;
					Note.Note = GET_NOTE(Trsp);
					Note.Octave = GET_OCTAVE(Trsp);
					SetDataAtPattern(t, p, Channel, r, &Note);
				}
			}
		}
		for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
			if (auto pInstrument = std::dynamic_pointer_cast<CInstrumentFDS>(GetInstrument(i))) {
				CSequence *pSeq = pInstrument->GetSequence(SEQ_ARPEGGIO);
				if (pSeq != nullptr && pSeq->GetItemCount() > 0 && pSeq->GetSetting() == SETTING_ARP_FIXED)
					for (unsigned int j = 0; j < pSeq->GetItemCount(); ++j) {
						int Trsp = pSeq->GetItem(j) + NOTE_RANGE * 2;
						pSeq->SetItem(j, Trsp >= NOTE_COUNT ? NOTE_COUNT - 1 : Trsp);
					}
			}
		}
	}
#endif

	return TRUE;
}

bool CFamiTrackerDoc::ReadBlock_Parameters(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();

	// Get first track for module versions that require that
	CPatternData *pTrack = GetTrack(0);

	if (Version == 1) {
		pTrack->SetSongSpeed(pDocFile->GetBlockInt());
	}
	else
		m_iExpansionChip = pDocFile->GetBlockChar();

	m_iChannelsAvailable	= AssertRange(pDocFile->GetBlockInt(), 1, MAX_CHANNELS, "Channel count");		// // //
	m_iMachine				= static_cast<machine_t>(pDocFile->GetBlockInt());
	m_iEngineSpeed			= pDocFile->GetBlockInt();

	AssertFileData(m_iMachine == NTSC || m_iMachine == PAL, "Unknown machine");

	if (Version > 2)
		m_iVibratoStyle = (vibrato_t)pDocFile->GetBlockInt();
	else
		m_iVibratoStyle = VIBRATO_OLD;

	// TODO read m_bLinearPitch

	m_vHighlight = DEFAULT_HIGHLIGHT;		// // //

	if (Version > 3) {
		m_vHighlight.First = pDocFile->GetBlockInt();
		m_vHighlight.Second = pDocFile->GetBlockInt();
	}

	// This is strange. Sometimes expansion chip is set to 0xFF in files
	if (m_iChannelsAvailable == 5)
		m_iExpansionChip = 0;

	if (m_iFileVersion == 0x0200) {
		int Speed = pTrack->GetSongSpeed();
		if (Speed < 20)
			pTrack->SetSongSpeed(Speed + 1);
	}

	if (Version == 1) {
		if (pTrack->GetSongSpeed() > 19) {
			pTrack->SetSongTempo(pTrack->GetSongSpeed());
			pTrack->SetSongSpeed(6);
		}
		else {
			pTrack->SetSongTempo(m_iMachine == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
		}
	}

	// Read namco channel count
	if (Version >= 5 && (m_iExpansionChip & SNDCHIP_N163))
		m_iNamcoChannels = AssertRange(pDocFile->GetBlockInt(), 1, 8, "N163 channel count");
	else		// // //
		m_iNamcoChannels = 0;

	if (Version >= 6) {
		m_iSpeedSplitPoint = pDocFile->GetBlockInt();
	}
	else {
		// Determine if new or old split point is preferred
		m_iSpeedSplitPoint = OLD_SPEED_SPLIT_POINT;
	}

	SetupChannels(m_iExpansionChip);

	return false;
}

bool CFamiTrackerDoc::ReadBlock_SongInfo(CDocumentFile *pDocFile)		// // //
{
	pDocFile->GetBlock(m_strName, 32);
	pDocFile->GetBlock(m_strArtist, 32);
	pDocFile->GetBlock(m_strCopyright, 32);

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Header(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();

	if (Version == 1) {
		// Single track
		m_iTrackCount = 1;
		CPatternData *pTrack = GetTrack(0);
		for (unsigned int i = 0; i < m_iChannelsAvailable; ++i) {
			// Channel type (unused)
			pDocFile->GetBlockChar();
			// Effect columns
			pTrack->SetEffectColumnCount(i, pDocFile->GetBlockChar());
		}
	}
	else if (Version >= 2) {
		// Multiple tracks
		m_iTrackCount = AssertRange(pDocFile->GetBlockChar() + 1, 1, static_cast<int>(MAX_TRACKS), "Track count");	// 0 means one track

		// Add tracks to document
		for (unsigned i = 0; i < m_iTrackCount; ++i) {
			AllocateTrack(i);
		}

		// Track names
		if (Version >= 3) {
			for (unsigned i = 0; i < m_iTrackCount; ++i) {
				m_sTrackNames[i] = pDocFile->ReadString();
			}
		}

		for (unsigned i = 0; i < m_iChannelsAvailable; ++i) {
			unsigned char ChannelType = pDocFile->GetBlockChar();					// Channel type (unused)
			for (unsigned j = 0; j < m_iTrackCount; ++j) {
				CPatternData *pTrack = GetTrack(j);
				unsigned char ColumnCount = pDocFile->GetBlockChar();
				pTrack->SetEffectColumnCount(i, ColumnCount);		// Effect columns
			}
		}

		if (Version >= 4) {
			// Read highlight settings for tracks
			for (unsigned int i = 0; i < m_iTrackCount; ++i) {
				CPatternData *pTrack = GetTrack(i);
				// TODO read highlight
				pTrack->SetHighlight(m_vHighlight);		// // //
			}
		}
		else {
			// Use global highlight
			for (unsigned int i = 0; i < m_iTrackCount; ++i) {
				CPatternData *pTrack = GetTrack(i);
				pTrack->SetHighlight(m_vHighlight);		// // //
			}
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Comments(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();
	m_bDisplayComment = (pDocFile->GetBlockInt() == 1) ? true : false;
	m_strComment = pDocFile->ReadString();
	return false;
}

bool CFamiTrackerDoc::ReadBlock_ChannelLayout(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();
	// Todo
	return false;
}

bool CFamiTrackerDoc::ReadBlock_Instruments(CDocumentFile *pDocFile)
{
	/*
	 * Version changes
	 *
	 *  2 - Extended DPCM octave range
	 *  3 - Added settings to the arpeggio sequence
	 *
	 */
	
	int Version = pDocFile->GetBlockVersion();

	// Number of instruments
	const int Count = AssertRange(pDocFile->GetBlockInt(), 0, CInstrumentManager::MAX_INSTRUMENTS, "Instrument count");

	for (int i = 0; i < Count; ++i) {
		// Instrument index
		int index = AssertRange(pDocFile->GetBlockInt(), 0, CInstrumentManager::MAX_INSTRUMENTS - 1, "Instrument index");

		// Read instrument type and create an instrument
		inst_type_t Type = (inst_type_t)pDocFile->GetBlockChar();
		auto pInstrument = CInstrumentManager::CreateNew(Type);
		m_pInstrumentManager->InsertInstrument(index, pInstrument); // this registers the instrument content provider

		try {
			// Load the instrument
			AssertFileData(pInstrument, "Failed to create instrument");
			pInstrument->Load(pDocFile);
			// Read name
			int size = AssertRange(pDocFile->GetBlockInt(), 0, CInstrument::INST_NAME_MAX, "Instrument name length");
			char Name[CInstrument::INST_NAME_MAX + 1];
			pDocFile->GetBlock(Name, size);
			Name[size] = 0;
			pInstrument->SetName(Name);
		}
		catch (CModuleException *e) {
			pDocFile->SetDefaultFooter(e);
			e->AppendError("At instrument %02X,", index);
			m_pInstrumentManager->RemoveInstrument(index);
			throw;
		}

	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Sequences(CDocumentFile *pDocFile)
{
	unsigned int ReleasePoint = -1, Settings = 0;
	int Version = pDocFile->GetBlockVersion();
	
	unsigned int Count = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "2A03 sequence count");

	if (Version == 1) {
		m_vTmpSequences.SetSize(MAX_SEQUENCES);
		for (unsigned int i = 0; i < Count; ++i) {
			unsigned int Index = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
			unsigned int SeqCount = static_cast<unsigned char>(pDocFile->GetBlockChar());
			AssertRange(SeqCount, 0U, static_cast<unsigned>(MAX_SEQUENCE_ITEMS - 1), "Sequence item count");
			m_vTmpSequences[Index].Count = SeqCount;
			for (unsigned int j = 0; j < SeqCount; ++j) {
				m_vTmpSequences[Index].Value[j] = pDocFile->GetBlockChar();
				m_vTmpSequences[Index].Length[j] = pDocFile->GetBlockChar();
			}
		}
	}
	else if (Version == 2) {
		m_vSequences.SetSize(MAX_SEQUENCES);
		for (unsigned int i = 0; i < Count; ++i) {
			unsigned int Index = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
			unsigned int Type = AssertRange(pDocFile->GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
			unsigned int SeqCount = static_cast<unsigned char>(pDocFile->GetBlockChar());
			AssertRange(SeqCount, 0U, static_cast<unsigned>(MAX_SEQUENCE_ITEMS - 1), "Sequence item count");
			m_vSequences[Index][Type].Count = SeqCount;
			for (unsigned int j = 0; j < SeqCount; ++j) {
				m_vSequences[Index][Type].Value[j] = pDocFile->GetBlockChar();
				m_vSequences[Index][Type].Length[j] = pDocFile->GetBlockChar();
			}

		}
	}
	else if (Version >= 3) {
		CSequenceManager *pManager = GetSequenceManager(INST_2A03);		// // //
		int Indices[MAX_SEQUENCES * SEQ_COUNT];
		int Types[MAX_SEQUENCES * SEQ_COUNT];

		for (unsigned int i = 0; i < Count; ++i) {
			unsigned int Index = Indices[i] = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
			unsigned int Type = Types[i] = AssertRange(pDocFile->GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
			try {
				unsigned char SeqCount = pDocFile->GetBlockChar();
				CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
				pSeq->Clear();
				pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

				unsigned int LoopPoint = AssertRange(pDocFile->GetBlockInt(), -1, static_cast<int>(SeqCount), "Sequence loop point");
				// Work-around for some older files
				if (LoopPoint != SeqCount)
					pSeq->SetLoopPoint(LoopPoint);

				if (Version == 4) {
					ReleasePoint = pDocFile->GetBlockInt();
					Settings = pDocFile->GetBlockInt();
					pSeq->SetReleasePoint(ReleasePoint);
					pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
				}

				// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
				for (int j = 0; j < SeqCount; ++j) {
					char Value = pDocFile->GetBlockChar();
					if (j < MAX_SEQUENCE_ITEMS)		// // //
						pSeq->SetItem(j, Value);
				}
			}
			catch (CModuleException *e) {
				e->AppendError("At 2A03 %d sequence %d,", Type, Index);
				throw;
			}
		}

		if (Version == 5) {
			// Version 5 saved the release points incorrectly, this is fixed in ver 6
			for (unsigned int i = 0; i < MAX_SEQUENCES; ++i) {
				for (int j = 0; j < SEQ_COUNT; ++j) try {
					int ReleasePoint = pDocFile->GetBlockInt();
					int Settings = pDocFile->GetBlockInt();
					CSequence *pSeq = pManager->GetCollection(j)->GetSequence(i);
					int Length = pSeq->GetItemCount();
					if (Length > 0) {
						pSeq->SetReleasePoint(AssertRange(ReleasePoint, -1, Length - 1, "Sequence release point"));
						pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
					}
				}
				catch (CModuleException *e) {
					e->AppendError("At 2A03 %d sequence %d,", j, i);
					throw;
				}
			}
		}
		else if (Version >= 6) {
			// Read release points correctly stored
			for (unsigned int i = 0; i < Count; ++i) try {
				CSequence *pSeq = pManager->GetCollection(Types[i])->GetSequence(Indices[i]);
				pSeq->SetReleasePoint(AssertRange(
					pDocFile->GetBlockInt(), -1, static_cast<int>(pSeq->GetItemCount()) - 1, "Sequence release point"));
				pSeq->SetSetting(static_cast<seq_setting_t>(pDocFile->GetBlockInt()));		// // //
			}
			catch (CModuleException *e) {
				e->AppendError("At 2A03 %d sequence %d,", Types[i], Indices[i]);
				throw;
			}
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_SequencesVRC6(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();
	
	unsigned int Count = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "VRC6 sequence count");

	CSequenceManager *pManager = GetSequenceManager(INST_VRC6);		// // //

	int Indices[MAX_SEQUENCES * SEQ_COUNT];
	int Types[MAX_SEQUENCES * SEQ_COUNT];
	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = Indices[i] = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int Type = Types[i] = AssertRange(pDocFile->GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = pDocFile->GetBlockChar();
			CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(AssertRange(pDocFile->GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point"));

			if (Version == 4) {
				pSeq->SetReleasePoint(AssertRange(pDocFile->GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
				pSeq->SetSetting(static_cast<seq_setting_t>(pDocFile->GetBlockInt()));		// // //
			}

			// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
			for (unsigned int j = 0; j < SeqCount; ++j) {
				char Value = pDocFile->GetBlockChar();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At VRC6 %d sequence %d,", Type, Index);
			throw;
		}
	}

	if (Version == 5) {
		// Version 5 saved the release points incorrectly, this is fixed in ver 6
		for (int i = 0; i < MAX_SEQUENCES; ++i) {
			for (int j = 0; j < SEQ_COUNT; ++j) try {
				int ReleasePoint = pDocFile->GetBlockInt();
				int Settings = pDocFile->GetBlockInt();
				CSequence *pSeq = pManager->GetCollection(j)->GetSequence(i);
				int Length = pSeq->GetItemCount();
				if (Length > 0) {
					pSeq->SetReleasePoint(AssertRange(ReleasePoint, -1, Length - 1, "Sequence release point"));
					pSeq->SetSetting(static_cast<seq_setting_t>(Settings));		// // //
				}
			}
			catch (CModuleException *e) {
				e->AppendError("At VRC6 %d sequence %d,", j, i);
				throw;
			}
		}
	}
	else if (Version >= 6) {
		for (unsigned int i = 0; i < Count; ++i) try {
			CSequence *pSeq = pManager->GetCollection(Types[i])->GetSequence(Indices[i]);
			pSeq->SetReleasePoint(AssertRange(pDocFile->GetBlockInt(), -1, static_cast<int>(pSeq->GetItemCount()) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(pDocFile->GetBlockInt()));		// // //
		}
		catch (CModuleException *e) {
			e->AppendError("At VRC6 %d sequence %d,", Types[i], Indices[i]);
			throw;
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_SequencesN163(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();

	unsigned int Count = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "N163 sequence count");

	CSequenceManager *pManager = GetSequenceManager(INST_N163);		// // //

	for (unsigned int i = 0; i < Count; i++) {
		unsigned int  Index		   = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int  Type		   = AssertRange(pDocFile->GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = pDocFile->GetBlockChar();
			CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(AssertRange(pDocFile->GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point"));
			pSeq->SetReleasePoint(AssertRange(pDocFile->GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(pDocFile->GetBlockInt()));		// // //

			// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
			for (int j = 0; j < SeqCount; ++j) {
				char Value = pDocFile->GetBlockChar();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At N163 %d sequence %d,", Type, Index);
			throw;
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_SequencesS5B(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();

	unsigned int Count = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES * SEQ_COUNT, "5B sequence count");

	CSequenceManager *pManager = GetSequenceManager(INST_S5B);		// // //

	for (unsigned int i = 0; i < Count; i++) {
		unsigned int  Index		   = AssertRange(pDocFile->GetBlockInt(), 0, MAX_SEQUENCES - 1, "Sequence index");
		unsigned int  Type		   = AssertRange(pDocFile->GetBlockInt(), 0, SEQ_COUNT - 1, "Sequence type");
		try {
			unsigned char SeqCount = pDocFile->GetBlockChar();
			CSequence *pSeq = pManager->GetCollection(Type)->GetSequence(Index);
			pSeq->Clear();
			pSeq->SetItemCount(SeqCount < MAX_SEQUENCE_ITEMS ? SeqCount : MAX_SEQUENCE_ITEMS);

			pSeq->SetLoopPoint(AssertRange(pDocFile->GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence loop point"));
			pSeq->SetReleasePoint(AssertRange(pDocFile->GetBlockInt(), -1, static_cast<int>(SeqCount) - 1, "Sequence release point"));
			pSeq->SetSetting(static_cast<seq_setting_t>(pDocFile->GetBlockInt()));		// // //

			// AssertRange(SeqCount, 0, MAX_SEQUENCE_ITEMS, "Sequence item count");
			for (int j = 0; j < SeqCount; ++j) {
				char Value = pDocFile->GetBlockChar();
				if (j < MAX_SEQUENCE_ITEMS)		// // //
					pSeq->SetItem(j, Value);
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At 5B %d sequence %d,", Type, Index);
			throw;
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Frames(CDocumentFile *pDocFile)
{
	unsigned int Version = pDocFile->GetBlockVersion();

	if (Version == 1) {
		unsigned int FrameCount = AssertRange(pDocFile->GetBlockInt(), 1, MAX_FRAMES, "Track frame count");
		m_iChannelsAvailable = AssertRange(pDocFile->GetBlockInt(), 0, MAX_CHANNELS, "Channel count");
		CPatternData *pTrack = GetTrack(0);
		pTrack->SetFrameCount(FrameCount);
		for (unsigned i = 0; i < FrameCount; ++i) {
			for (unsigned j = 0; j < m_iChannelsAvailable; ++j) {
				unsigned Pattern = static_cast<unsigned char>(pDocFile->GetBlockChar());
				AssertRange(Pattern, 0U, static_cast<unsigned>(MAX_PATTERN - 1), "Pattern index");
				pTrack->SetFramePattern(i, j, Pattern);
			}
		}
	}
	else if (Version > 1) {

		for (unsigned y = 0; y < m_iTrackCount; ++y) {
			unsigned int FrameCount = AssertRange(pDocFile->GetBlockInt(), 1, MAX_FRAMES, "Track frame count");
			unsigned int Speed = AssertRange(pDocFile->GetBlockInt(), 0, MAX_TEMPO, "Track default speed");

			CPatternData *pTrack = GetTrack(y);
			pTrack->SetFrameCount(FrameCount);

			if (Version >= 3) {
				unsigned int Tempo = AssertRange(pDocFile->GetBlockInt(), 0, MAX_TEMPO, "Track default tempo");
				pTrack->SetSongTempo(Tempo);
				pTrack->SetSongSpeed(Speed);
			}
			else {
				if (Speed < 20) {
					pTrack->SetSongTempo(m_iMachine == NTSC ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL);
					pTrack->SetSongSpeed(Speed);
				}
				else {
					pTrack->SetSongTempo(Speed);
					pTrack->SetSongSpeed(DEFAULT_SPEED);
				}
			}

			unsigned PatternLength = AssertRange(pDocFile->GetBlockInt(), 1, MAX_PATTERN_LENGTH, "Track default row count");
			pTrack->SetPatternLength(PatternLength);
			
			for (unsigned i = 0; i < FrameCount; ++i) {
				for (unsigned j = 0; j < m_iChannelsAvailable; ++j) {
					// Read pattern index
					int Pattern = static_cast<unsigned char>(pDocFile->GetBlockChar());
					pTrack->SetFramePattern(i, j, AssertRange(Pattern, 0, MAX_PATTERN - 1, "Pattern index"));
				}
			}
		}
	}

	return false;
}

bool CFamiTrackerDoc::ReadBlock_Patterns(CDocumentFile *pDocFile)
{
	unsigned int Version = pDocFile->GetBlockVersion();

#ifdef TRANSPOSE_FDS
	m_bAdjustFDSArpeggio = Version < 5;
#endif

	if (Version == 1) {
		int PatternLen = AssertRange(pDocFile->GetBlockInt(), 0, MAX_PATTERN_LENGTH, "Pattern data count");
		CPatternData *pTrack = GetTrack(0);
		pTrack->SetPatternLength(PatternLen);
	}

	while (!pDocFile->BlockDone()) {
		unsigned Track;
		if (Version > 1)
			Track = AssertRange(pDocFile->GetBlockInt(), 0, static_cast<int>(MAX_TRACKS) - 1, "Pattern track index");
		else if (Version == 1)
			Track = 0;

		unsigned Channel = AssertRange(pDocFile->GetBlockInt(), 0, MAX_CHANNELS - 1, "Pattern channel index");
		unsigned Pattern = AssertRange(pDocFile->GetBlockInt(), 0, MAX_PATTERN - 1, "Pattern index");
		unsigned Items	= AssertRange(pDocFile->GetBlockInt(), 0, MAX_PATTERN_LENGTH, "Pattern data count");

		CPatternData *pTrack = GetTrack(Track);

		for (unsigned i = 0; i < Items; ++i) try {
			unsigned Row;
			if (m_iFileVersion == 0x0200)
				Row = pDocFile->GetBlockChar();
			else
				Row = AssertRange(pDocFile->GetBlockInt(), 0, 0xFF, "Row index");		// // //

			try {
				stChanNote *Note = pTrack->GetPatternData(Channel, Pattern, Row);
				*Note = BLANK_NOTE;		// // //

				Note->Note = AssertRange(pDocFile->GetBlockChar(), NONE, ECHO, "Note value");
				Note->Octave = AssertRange(pDocFile->GetBlockChar(), 0, OCTAVE_RANGE - 1, "Octave value");
				Note->Instrument = AssertRange(pDocFile->GetBlockChar(), 0, m_pInstrumentManager->MAX_INSTRUMENTS, "Instrument index");
				Note->Vol = AssertRange(pDocFile->GetBlockChar(), 0, MAX_VOLUME, "Channel volume");

				for (int n = 0; n < (pTrack->GetEffectColumnCount(Channel) + 1); ++n) try {
					unsigned char EffectNumber = pDocFile->GetBlockChar();
					unsigned char EffectParam = pDocFile->GetBlockChar();
					if (Version < 3) {
						if (EffectNumber == EF_PORTAOFF) {
							EffectNumber = EF_PORTAMENTO;
							EffectParam = 0;
						}
						else if (EffectNumber == EF_PORTAMENTO) {
							if (EffectParam < 0xFF)
								EffectParam++;
						}
					}
					if (Note->EffNumber[n] = static_cast<effect_t>(AssertRange(EffectNumber, EF_NONE, EF_COUNT - 1, "Effect index")))
						Note->EffParam[n] = EffectParam; // skip on no effect
					if (m_iFileVersion == 0x200) break;		// // //
				}
				catch (CModuleException *e) {
					e->AppendError("At effect column fx%d,", n + 1);
					throw;
				}

	//			if (Note->Vol > MAX_VOLUME)
	//				Note->Vol &= 0x0F;

				// Specific for version 2.0
				if (m_iFileVersion == 0x0200) {

					if (Note->EffNumber[0] == EF_SPEED && Note->EffParam[0] < 20)
						Note->EffParam[0]++;

					if (Note->Vol == 0)
						Note->Vol = MAX_VOLUME;
					else {
						Note->Vol--;
						Note->Vol &= 0x0F;
					}

					if (Note->Note == 0)
						Note->Instrument = MAX_INSTRUMENTS;
				}

				if (ExpansionEnabled(SNDCHIP_N163) && GetChipType(Channel) == SNDCHIP_N163) {		// // //
					for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n)
						if (Note->EffNumber[n] == EF_SAMPLE_OFFSET)
							Note->EffNumber[n] = EF_N163_WAVE_BUFFER;
				}

				if (Version == 3) {
					// Fix for VRC7 portamento
					if (ExpansionEnabled(SNDCHIP_VRC7) && Channel > 4) {
						for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
							switch (Note->EffNumber[n]) {
							case EF_PORTA_DOWN:
								Note->EffNumber[n] = EF_PORTA_UP;
								break;
							case EF_PORTA_UP:
								Note->EffNumber[n] = EF_PORTA_DOWN;
								break;
							}
						}
					}
					// FDS pitch effect fix
					else if (ExpansionEnabled(SNDCHIP_FDS) && GetChannelType(Channel) == CHANID_FDS) {
						for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
							switch (Note->EffNumber[n]) {
							case EF_PITCH:
								if (Note->EffParam[n] != 0x80)
									Note->EffParam[n] = (0x100 - Note->EffParam[n]) & 0xFF;
								break;
							}
						}
					}
				}
				/*
				if (Version < 6) {
					// Noise pitch slide fix
					if (GetChannelType(Channel) == CHANID_NOISE) {
						for (int n = 0; n < MAX_EFFECT_COLUMNS; ++n) {
							switch (Note->EffNumber[n]) {
								case EF_PORTA_DOWN:
									Note->EffNumber[n] = EF_PORTA_UP;
									Note->EffParam[n] = Note->EffParam[n] << 4;
									break;
								case EF_PORTA_UP:
									Note->EffNumber[n] = EF_PORTA_DOWN;
									Note->EffParam[n] = Note->EffParam[n] << 4;
									break;
								case EF_PORTAMENTO:
									Note->EffParam[n] = Note->EffParam[n] << 4;
									break;
								case EF_SLIDE_UP:
									Note->EffParam[n] = Note->EffParam[n] + 0x70;
									break;
								case EF_SLIDE_DOWN:
									Note->EffParam[n] = Note->EffParam[n] + 0x70;
									break;
							}
						}
					}
				}
				*/
			}
			catch (CModuleException *e) {
				e->AppendError("At row %02X,", Row);
				throw;
			}
		}
		catch (CModuleException *e) {
			e->AppendError("At pattern %02X, channel %d, track %d,", Pattern, Channel, Track + 1);
			throw;
		}
	}
	
	return false;
}

bool CFamiTrackerDoc::ReadBlock_DSamples(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();

	unsigned int Count = AssertRange(
		static_cast<unsigned char>(pDocFile->GetBlockChar()), 0U, CDSampleManager::MAX_DSAMPLES, "DPCM sample count");

	for (unsigned int i = 0; i < Count; ++i) {
		unsigned int Index = AssertRange(
			static_cast<unsigned char>(pDocFile->GetBlockChar()), 0U, CDSampleManager::MAX_DSAMPLES - 1, "DPCM sample index");
		CDSample *pSample = nullptr;
		try {
			pSample = new CDSample();		// // //
			unsigned int Len = AssertRange(pDocFile->GetBlockInt(), 0, CDSample::MAX_NAME_SIZE - 1, "DPCM sample name length");
			char Name[CDSample::MAX_NAME_SIZE] = {};
			pDocFile->GetBlock(Name, Len);
			Name[Len] = 0;
			pSample->SetName(Name);
			int Size = AssertRange(pDocFile->GetBlockInt(), 0, CDSample::MAX_SIZE, "DPCM sample size");
			if ((Size & 0x0F) != 0x01) {
				CModuleException *e = m_pCurrentDocument->GetException();
				e->AppendError("Invalid DPCM sample size (0x%X bytes)", Size);
				e->Raise();
			}
			char *pData = new char[Size];
			pDocFile->GetBlock(pData, Size);
			pSample->SetData(Size, pData);
		}
		catch (CModuleException *e) {
			e->AppendError("At DPCM sample %d,", Index);
			throw;
		}
		SetSample(Index, pSample);
	}

	return false;
}

// // // Detune tables

#include "DetuneDlg.h" // TODO: bad, encapsulate detune tables

bool CFamiTrackerDoc::ReadBlock_DetuneTables(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();	// Ver 1
	int Count = AssertRange(pDocFile->GetBlockChar(), 0, 6, "Detune table count");
	for (int i = 0; i < Count; i++) {
		int Chip = AssertRange(pDocFile->GetBlockChar(), 0, 5, "Detune table index");
		try {
			int Item = AssertRange(pDocFile->GetBlockChar(), 0, NOTE_COUNT, "Detune table note count");
			for (int j = 0; j < Item; j++) {
				int Note = AssertRange(pDocFile->GetBlockChar(), 0, NOTE_COUNT - 1, "Detune table note index");
				int Offset = pDocFile->GetBlockInt();
				m_iDetuneTable[Chip][Note] = Offset;
			}
		}
		catch (CModuleException *e) {
			e->AppendError("In %s detune table,", CDetuneDlg::CHIP_STR[Chip]);
			throw;
		}
	}

	return false;
}

bool CFamiTrackerDoc::WriteBlock_DetuneTables(CDocumentFile *pDocFile, const int Version) const
{
	int NoteUsed[6], ChipCount = 0;
	for (int i = 0; i < 6; i++) {
		NoteUsed[i] = 0;
		for (int j = 0; j < NOTE_COUNT; j++)
			if (m_iDetuneTable[i][j] != 0)
				NoteUsed[i]++;
		if (NoteUsed[i]) ChipCount++;
	}
	if (!ChipCount) return true;

	pDocFile->CreateBlock(FILE_BLOCK_DETUNETABLES, Version);
	pDocFile->WriteBlockChar(ChipCount);
	
	for (int i = 0; i < 6; i++) {
		if (!NoteUsed[i]) continue;
		pDocFile->WriteBlockChar(i);
		pDocFile->WriteBlockChar(NoteUsed[i]);
		for (int j = 0; j < NOTE_COUNT; j++) {
			if (m_iDetuneTable[i][j] == 0) continue;
			pDocFile->WriteBlockChar(j);
			pDocFile->WriteBlockInt(m_iDetuneTable[i][j]);
		}
	}

	return pDocFile->FlushBlock();
}

// // // Groove table

bool CFamiTrackerDoc::ReadBlock_Grooves(CDocumentFile *pDocFile)
{
	const int Version = pDocFile->GetBlockVersion();	// Ver 1
	const int Count = AssertRange(pDocFile->GetBlockChar(), 0, MAX_GROOVE, "Groove count");

	for (int i = 0; i < Count; i++) {
		int Index = pDocFile->GetBlockChar();
		int Size = pDocFile->GetBlockChar();
		if (m_pGrooveTable[Index] == NULL)
			m_pGrooveTable[Index] = new CGroove();
		m_pGrooveTable[Index]->SetSize(Size);
		for (int j = 0; j < Size; j++)
			m_pGrooveTable[Index]->SetEntry(j, pDocFile->GetBlockChar());
	}

	//Count = pDocFile->GetBlockChar();
	AssertFileData(pDocFile->GetBlockChar() == m_iTrackCount, "Use-groove flag count does not match track count");
	for (unsigned i = 0; i < m_iTrackCount; ++i) {
		CPatternData *pTrack = GetTrack(i);
		pTrack->SetSongGroove(pDocFile->GetBlockChar() == 1);
		int Speed = pTrack->GetSongSpeed();
		if (pTrack->GetSongGroove())
			AssertRange(Speed, 0, MAX_GROOVE - 1, "Track default groove index");
		else
			AssertRange(Speed, 1, MAX_TEMPO, "Track default speed");
	}

	return false;
}

bool CFamiTrackerDoc::WriteBlock_Grooves(CDocumentFile *pDocFile, const int Version) const
{
	int Count = 0;
	for (int i = 0; i < MAX_GROOVE; i++)
		if (m_pGrooveTable[i] != NULL) Count++;
	if (!Count) return true;
	pDocFile->CreateBlock(FILE_BLOCK_GROOVES, Version);
	pDocFile->WriteBlockChar(Count);
	
	for (int i = 0; i < MAX_GROOVE; i++) if (m_pGrooveTable[i] != NULL) {
		int Size = m_pGrooveTable[i]->GetSize();
		pDocFile->WriteBlockChar(i);
		pDocFile->WriteBlockChar(Size);
		for (int j = 0; j < Size; j++)
			pDocFile->WriteBlockChar(m_pGrooveTable[i]->GetEntry(j));
	}
	
	pDocFile->WriteBlockChar(m_iTrackCount);
	for (unsigned i = 0; i < m_iTrackCount; ++i)
		pDocFile->WriteBlockChar(GetTrack(i)->GetSongGroove());

	return pDocFile->FlushBlock();
}

// // // Bookmarks

bool CFamiTrackerDoc::ReadBlock_Bookmarks(CDocumentFile *pDocFile)
{
	int Version = pDocFile->GetBlockVersion();	// Ver 1
	int Count = pDocFile->GetBlockInt();

	for (int i = 0; i < Count; i++) {
		stBookmark Mark = {};
		unsigned int Track = AssertRange(static_cast<unsigned char>(pDocFile->GetBlockChar()), 0, m_iTrackCount - 1, "Bookmark track index");
		int Frame = static_cast<unsigned char>(pDocFile->GetBlockChar());
		int Row = static_cast<unsigned char>(pDocFile->GetBlockChar());
		Mark.Frame = AssertRange(Frame, 0, static_cast<int>(m_pTracks[Track]->GetFrameCount()) - 1, "Bookmark frame index");
		Mark.Row = AssertRange(Row, 0, static_cast<int>(m_pTracks[Track]->GetPatternLength()) - 1, "Bookmark row index");
		Mark.Highlight.First = pDocFile->GetBlockInt();
		Mark.Highlight.Second = pDocFile->GetBlockInt();
		Mark.Persist = pDocFile->GetBlockChar() != 0;
		Mark.Name = new CString(pDocFile->ReadString());
		
		if (m_pBookmarkList[Track] == NULL)
			m_pBookmarkList[Track] = new std::vector<stBookmark>();
		m_pBookmarkList[Track]->push_back(Mark);
	}

	return false;
}

bool CFamiTrackerDoc::WriteBlock_Bookmarks(CDocumentFile *pDocFile, const int Version) const
{
	int Count = 0;
	for (unsigned int i = 0; i < m_iTrackCount; i++)
		if (m_pBookmarkList[i] != NULL) Count += m_pBookmarkList[i]->size();
	if (!Count) return true;
	pDocFile->CreateBlock(FILE_BLOCK_BOOKMARKS, Version);
	pDocFile->WriteBlockInt(Count);
	
	for (unsigned int i = 0; i < m_iTrackCount; i++) if (m_pBookmarkList[i] != NULL) {
		for (auto it = m_pBookmarkList[i]->begin(); it < m_pBookmarkList[i]->end(); it++) {
			pDocFile->WriteBlockChar(i);
			pDocFile->WriteBlockChar(it->Frame);
			pDocFile->WriteBlockChar(it->Row);
			pDocFile->WriteBlockInt(it->Highlight.First);
			pDocFile->WriteBlockInt(it->Highlight.Second);
			pDocFile->WriteBlockChar(it->Persist);
			//pDocFile->WriteBlockInt(it->Name->GetLength());
			//pDocFile->WriteBlock(it->Name, (int)strlen(Name));	
			pDocFile->WriteString(*it->Name);
		}
	}

	return pDocFile->FlushBlock();
}

// FTM import ////

CFamiTrackerDoc *CFamiTrackerDoc::LoadImportFile(LPCTSTR lpszPathName) const
{
	// Import a module as new subtunes
	CFamiTrackerDoc *pImported = new CFamiTrackerDoc();

	pImported->DeleteContents();

	// Load into a new document
	if (!pImported->OpenDocument(lpszPathName))
		SAFE_RELEASE(pImported);

	return pImported;
}

bool CFamiTrackerDoc::ImportInstruments(CFamiTrackerDoc *pImported, int *pInstTable)
{
	// Copy instruments to current module
	//
	// pInstTable must point to an int array of size MAX_INSTRUMENTS
	//

	int SamplesTable[MAX_DSAMPLES];
	int SequenceTable2A03[MAX_SEQUENCES][SEQ_COUNT];
	int SequenceTableVRC6[MAX_SEQUENCES][SEQ_COUNT];
	int SequenceTableN163[MAX_SEQUENCES][SEQ_COUNT];
	int SequenceTableS5B[MAX_SEQUENCES][SEQ_COUNT];		// // //

	memset(SamplesTable, 0, sizeof(int) * MAX_DSAMPLES);
	memset(SequenceTable2A03, 0, sizeof(int) * MAX_SEQUENCES * SEQ_COUNT);
	memset(SequenceTableVRC6, 0, sizeof(int) * MAX_SEQUENCES * SEQ_COUNT);
	memset(SequenceTableN163, 0, sizeof(int) * MAX_SEQUENCES * SEQ_COUNT);
	memset(SequenceTableS5B, 0, sizeof(int) * MAX_SEQUENCES * SEQ_COUNT);		// // //

	// Check instrument count
	if (GetInstrumentCount() + pImported->GetInstrumentCount() > MAX_INSTRUMENTS) {
		// Out of instrument slots
		AfxMessageBox(IDS_IMPORT_INSTRUMENT_COUNT, MB_ICONERROR);
		return false;
	}

	static const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};		// // //
	static const uint8 chip[] = {SNDCHIP_NONE, SNDCHIP_VRC6, SNDCHIP_N163, SNDCHIP_S5B};
	int (*seqTable[])[SEQ_COUNT] = {SequenceTable2A03, SequenceTableVRC6, SequenceTableN163, SequenceTableS5B};

	// Copy sequences
	for (size_t i = 0; i < sizeof(chip); i++) for (int t = 0; t < SEQ_COUNT; ++t) {
		if (GetSequenceCount(inst[i], t) + pImported->GetSequenceCount(inst[i], t) > MAX_SEQUENCES) {		// // //
			AfxMessageBox(IDS_IMPORT_SEQUENCE_COUNT, MB_ICONERROR);
			return false;
		}
		for (unsigned int s = 0; s < MAX_SEQUENCES; ++s) if (pImported->GetSequenceItemCount(inst[i], s, t) > 0) {
			CSequence *pImportSeq = pImported->GetSequence(inst[i], s, t);
			int index = -1;
			for (unsigned j = 0; j < MAX_SEQUENCES; ++j) {
				if (GetSequenceItemCount(inst[i], j, t)) continue;
				// TODO: continue if blank sequence is used by some instrument
				CSequence *pSeq = GetSequence(inst[i], j, t);
				pSeq->Copy(pImportSeq);
				// Save a reference to this sequence
				seqTable[i][s][t] = j;
				break;
			}
		}
	}

	bool bOutOfSampleSpace = false;

	// Copy DPCM samples
	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (const CDSample *pImportDSample = pImported->GetSample(i)) {		// // //
			int Index = GetFreeSampleSlot();
			if (Index != -1) {
				CDSample *pDSample = new CDSample(*pImportDSample);		// // //
				SetSample(Index, pDSample);
				// Save a reference to this DPCM sample
				SamplesTable[i] = Index;
			}
			else
				bOutOfSampleSpace = true;
		}
	}

	if (bOutOfSampleSpace) {
		// Out of sample space
		AfxMessageBox(IDS_IMPORT_SAMPLE_SLOTS, MB_ICONEXCLAMATION);
		return false;
	}

	// Copy instruments
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (pImported->IsInstrumentUsed(i)) {
			CInstrument *pInst = pImported->GetInstrument(i)->Clone();
			inst_type_t Type = pInst->GetType();
			// Update references
			switch (Type) {
			case INST_2A03: case INST_VRC6: case INST_N163: case INST_S5B:		// // //
				{
					// Update sequence references
					CSeqInstrument *pInstrument = static_cast<CSeqInstrument*>(pInst);
					for (int t = 0; t < SEQ_COUNT; ++t) if (pInstrument->GetSeqEnable(t)) {
						for (size_t j = 0; j < sizeof(chip); j++) if (inst[j] == Type) {
							pInstrument->SetSeqIndex(t, seqTable[j][pInstrument->GetSeqIndex(t)][t]);
							break;
						}
					}
				}
				break;
			case INST_FDS: case INST_VRC7:		// // //
				// no operations
				break;
			default:
				AfxDebugBreak();	// Add code for this instrument
			}
			if (Type == INST_2A03) {
				CInstrument2A03 *pInstrument = static_cast<CInstrument2A03*>(pInst);
				// Update DPCM samples
				for (int o = 0; o < OCTAVE_RANGE; ++o) for (int n = 0; n < NOTE_RANGE; ++n) {
					int Sample = pInstrument->GetSample(o, n);
					if (Sample != 0)
						pInstrument->SetSample(o, n, SamplesTable[Sample - 1] + 1);
				}
			}
			// Update samples
			int Index = AddInstrument(pInst);
			// Save a reference to this instrument
			pInstTable[i] = Index;
		}
	}

	return true;
}

bool CFamiTrackerDoc::ImportGrooves(CFamiTrackerDoc *pImported, int *pGrooveMap)		// // //
{
	int Index = 0;
	for (int i = 0; i < MAX_GROOVE; i++) {
		if (pImported->GetGroove(i) != NULL) {
			while (GetGroove(Index) != NULL) Index++;
			if (Index >= MAX_GROOVE) {
				AfxMessageBox(IDS_IMPORT_GROOVE_SLOTS, MB_ICONEXCLAMATION);
				return false;
			}
			pGrooveMap[i] = Index;
			m_pGrooveTable[Index] = new CGroove();
			m_pGrooveTable[Index]->Copy(pImported->GetGroove(i));
		}
	}

	return true;
}

bool CFamiTrackerDoc::ImportDetune(CFamiTrackerDoc *pImported)		// // //
{
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		m_iDetuneTable[i][j] = pImported->GetDetuneOffset(i, j);

	theApp.GetSoundGenerator()->LoadMachineSettings(m_iMachine, m_iEngineSpeed, m_iNamcoChannels);
	return true;
}

bool CFamiTrackerDoc::ImportTrack(int Track, CFamiTrackerDoc *pImported, int *pInstTable, int *pGrooveMap)		// // //
{
	// Import a selected track from specified source document

	int NewTrack = AddTrack();

	if (NewTrack == -1)
		return false;

	// Copy parameters
	SetPatternLength(NewTrack, pImported->GetPatternLength(Track));
	SetFrameCount(NewTrack, pImported->GetFrameCount(Track));
	SetSongTempo(NewTrack, pImported->GetSongTempo(Track));
	SetSongGroove(NewTrack, pImported->GetSongGroove(Track));
	if (GetSongGroove(NewTrack))
		SetSongSpeed(NewTrack, pGrooveMap[pImported->GetSongSpeed(Track)]);
	else
		SetSongSpeed(NewTrack, pImported->GetSongSpeed(Track));

	// Copy track name
	SetTrackTitle(NewTrack, pImported->GetTrackTitle(Track));

	// Copy frames
	for (unsigned int f = 0; f < pImported->GetFrameCount(Track); ++f) {
		for (unsigned int c = 0; c < GetAvailableChannels(); ++c) {
			SetPatternAtFrame(NewTrack, f, c, pImported->GetPatternAtFrame(Track, f, c));
		}
	}

	// // // Copy bookmarks
	auto List = pImported->GetBookmarkList(Track);
	SetBookmarkList(NewTrack, List);
	SAFE_RELEASE(List);

	stChanNote data;

	// Copy patterns
	for (unsigned int p = 0; p < MAX_PATTERN; ++p) {
		for (unsigned int c = 0; c < GetAvailableChannels(); ++c) {
			for (unsigned int r = 0; r < pImported->GetPatternLength(Track); ++r) {
				// Get note
				pImported->GetDataAtPattern(Track, p, c, r, &data);
				// Translate instrument number
				if (data.Instrument < MAX_INSTRUMENTS)
					data.Instrument = pInstTable[data.Instrument];
				for (int i = 0; i < MAX_EFFECT_COLUMNS; i++)		// // //
					if (data.EffNumber[i] == EF_GROOVE && data.EffParam[i] < MAX_GROOVE)
						data.EffParam[i] = pGrooveMap[data.EffParam[i]];
				// Store
				SetDataAtPattern(NewTrack, p, c, r, &data);
			}
		}
	}

	// Effect columns
	for (unsigned int c = 0; c < GetAvailableChannels(); ++c) {
		SetEffColumns(NewTrack, c, pImported->GetEffColumns(Track, c));
	}

	return true;
}

// End of file load/save

// DMC Stuff

const CDSample *CFamiTrackerDoc::GetSample(unsigned int Index) const
{
	ASSERT(Index < MAX_DSAMPLES);
	return m_pInstrumentManager->GetDSampleManager()->GetDSample(Index);		// // //
}

void CFamiTrackerDoc::SetSample(unsigned int Index, CDSample *pSamp)		// // //
{
	ASSERT(Index < MAX_DSAMPLES);
	if (m_pInstrumentManager->GetDSampleManager()->SetDSample(Index, pSamp)) {
		SetModifiedFlag();
		SetExceededFlag();		// // //
	}
}

bool CFamiTrackerDoc::IsSampleUsed(unsigned int Index) const
{
	ASSERT(Index < MAX_DSAMPLES);
	return m_pInstrumentManager->GetDSampleManager()->IsSampleUsed(Index);		// // //
}

unsigned int CFamiTrackerDoc::GetSampleCount() const
{
	return m_pInstrumentManager->GetDSampleManager()->GetSampleCount();
}

int CFamiTrackerDoc::GetFreeSampleSlot() const
{
	return m_pInstrumentManager->GetDSampleManager()->GetFirstFree();
}

void CFamiTrackerDoc::RemoveSample(unsigned int Index)
{
	SetSample(Index, nullptr);		// // //
}

unsigned int CFamiTrackerDoc::GetTotalSampleSize() const
{
	return m_pInstrumentManager->GetDSampleManager()->GetTotalSize();
}

// ---------------------------------------------------------------------------------------------------------
// Document access functions
// ---------------------------------------------------------------------------------------------------------

//
// Sequences
//

CSequence *CFamiTrackerDoc::GetSequence(inst_type_t InstType, unsigned int Index, int Type)		// // //
{
	return m_pInstrumentManager->GetSequence(InstType, Type, Index);
}

CSequence *CFamiTrackerDoc::GetSequence(inst_type_t InstType, unsigned int Index, int Type) const		// // //
{
	return m_pInstrumentManager->GetSequence(InstType, Type, Index);
}

unsigned int CFamiTrackerDoc::GetSequenceItemCount(inst_type_t InstType, unsigned int Index, int Type) const		// // //
{
	ASSERT(Index < MAX_SEQUENCES);
	ASSERT(Type >= 0 && Type < SEQ_COUNT);

	const CSequence *pSeq = GetSequence(InstType, Index, Type);
	if (pSeq == NULL)
		return 0;
	return pSeq->GetItemCount();
}

int CFamiTrackerDoc::GetFreeSequence(inst_type_t InstType, int Type, CSeqInstrument *pInst) const		// // //
{
	ASSERT(Type >= 0 && Type < SEQ_COUNT);
	return m_pInstrumentManager->GetFreeSequenceIndex(InstType, Type, pInst);
}

int CFamiTrackerDoc::GetSequenceCount(inst_type_t InstType, int Type) const		// // //
{
	// Return number of allocated sequences of Type
	ASSERT(Type >= 0 && Type < SEQ_COUNT);

	int Count = 0;
	for (int i = 0; i < MAX_SEQUENCES; ++i) {
		if (GetSequenceItemCount(InstType, i, Type) > 0) // TODO: fix this and the instrument interface
			++Count;
	}
	return Count;
}

//
// Song info
//

const char* CFamiTrackerDoc::GetSongName() const
{ 
	return m_strName; 
}

const char* CFamiTrackerDoc::GetSongArtist() const
{ 
	return m_strArtist; 
}

const char* CFamiTrackerDoc::GetSongCopyright() const
{ 
	return m_strCopyright; 
}

void CFamiTrackerDoc::SetSongName(const char *pName)
{
	ASSERT(pName != NULL);
	if (strcmp(m_strName, pName) != 0) {
		bool nul = false;		// // //
		char str[32];
		for (int i = 0; i < 32; i++) {
			str[i] = nul ? 0 : pName[i];
			if (!str[i]) nul = true;
		}
		memcpy_s(m_strName, 32, str, 32);
		SetModifiedFlag();
		SetExceededFlag();		// // //
	}
}

void CFamiTrackerDoc::SetSongArtist(const char *pArtist)
{
	ASSERT(pArtist != NULL);
	if (strcmp(m_strArtist, pArtist) != 0) {
		bool nul = false;		// // //
		char str[32];
		for (int i = 0; i < 32; i++) {
			str[i] = nul ? 0 : pArtist[i];
			if (!str[i]) nul = true;
		}
		memcpy_s(m_strArtist, 32, str, 32);
		SetModifiedFlag();
		SetExceededFlag();		// // //
	}
}

void CFamiTrackerDoc::SetSongCopyright(const char *pCopyright)
{
	ASSERT(pCopyright != NULL);
	if (strcmp(m_strCopyright, pCopyright) != 0) {
		bool nul = false;		// // //
		char str[32];
		for (int i = 0; i < 32; i++) {
			str[i] = nul ? 0 : pCopyright[i];
			if (!str[i]) nul = true;
		}
		memcpy_s(m_strCopyright, 32, str, 32);
		SetModifiedFlag();
		SetExceededFlag();		// // //
	}
}

//
// Instruments
//

std::shared_ptr<CInstrument> CFamiTrackerDoc::GetInstrument(unsigned int Index) const
{
	return m_pInstrumentManager->GetInstrument(Index);
}

unsigned int CFamiTrackerDoc::GetInstrumentCount() const
{
	return m_pInstrumentManager->GetInstrumentCount();
}

bool CFamiTrackerDoc::IsInstrumentUsed(unsigned int Index) const
{
	return m_pInstrumentManager->IsInstrumentUsed(Index);
}

int CFamiTrackerDoc::AddInstrument(CInstrument *pInstrument)
{
	const int Slot = m_pInstrumentManager->GetFirstUnused();
	if (Slot == INVALID_INSTRUMENT)
		return INVALID_INSTRUMENT;
	AddInstrument(pInstrument, Slot);		// // //
	return Slot;
}

void CFamiTrackerDoc::AddInstrument(CInstrument *pInstrument, unsigned int Slot)
{
	if (m_pInstrumentManager->InsertInstrument(Slot, pInstrument)) {
		SetModifiedFlag();
		SetExceededFlag();		// // //
	}
}

int CFamiTrackerDoc::AddInstrument(const char *pName, int ChipType)
{
	CInstrument *pInst = theApp.GetChannelMap()->GetChipInstrument(ChipType);
	if (!pInst) {
#ifdef _DEBUG
		MessageBox(NULL, _T("(TODO) add instrument definitions for this chip"), _T("Stop"), MB_OK);
#endif
		return INVALID_INSTRUMENT;
	}
	pInst->RegisterManager(m_pInstrumentManager);
	pInst->Setup();
	pInst->SetName(pName);

	return AddInstrument(pInst);
}

void CFamiTrackerDoc::RemoveInstrument(unsigned int Index)
{
	if (m_pInstrumentManager->RemoveInstrument(Index)) {
		SetModifiedFlag();
		SetExceededFlag();		// // //
	}
}

int CFamiTrackerDoc::CloneInstrument(unsigned int Index)
{
	ASSERT(Index < MAX_INSTRUMENTS);

	if (!IsInstrumentUsed(Index))
		return INVALID_INSTRUMENT;

	const int Slot = m_pInstrumentManager->GetFirstUnused();

	if (Slot != INVALID_INSTRUMENT) {
		m_pInstrumentManager->InsertInstrument(Slot, m_pInstrumentManager->GetInstrument(Index)->Clone());
		SetModifiedFlag();
		SetExceededFlag();		// // //
	}

	return Slot;
}

void CFamiTrackerDoc::GetInstrumentName(unsigned int Index, char *pName) const
{
	if (auto pInst = m_pInstrumentManager->GetInstrument(Index))
		pInst->GetName(pName);
}

void CFamiTrackerDoc::SetInstrumentName(unsigned int Index, const char *pName)
{
	auto pInst = m_pInstrumentManager->GetInstrument(Index);
	ASSERT(pInst);

	if (strcmp(pInst->GetName(), pName) != 0) {
		pInst->SetName(pName);
		SetModifiedFlag();
		SetExceededFlag();		// // //
	}
}

inst_type_t CFamiTrackerDoc::GetInstrumentType(unsigned int Index) const
{
	return m_pInstrumentManager->GetInstrumentType(Index);
}

int CFamiTrackerDoc::DeepCloneInstrument(unsigned int Index) 
{
	int Slot = CloneInstrument(Index);

	if (Slot != INVALID_INSTRUMENT) {
		auto newInst = m_pInstrumentManager->GetInstrument(Slot);
		const inst_type_t it = newInst->GetType();
		if (auto pInstrument = std::dynamic_pointer_cast<CSeqInstrument>(newInst)) {
			for (int i = 0; i < SEQ_COUNT; i++) {
				int freeSeq = GetFreeSequence(it, i);
				if (freeSeq != -1) {
					if (pInstrument->GetSeqEnable(i))
						GetSequence(it, unsigned(freeSeq), i)->Copy(pInstrument->GetSequence(i));
					pInstrument->SetSeqIndex(i, freeSeq);
				}
			}
		}
		SetModifiedFlag();
		SetExceededFlag();		// // //
	}

	return Slot;
}

void CFamiTrackerDoc::SaveInstrument(unsigned int Index, CString FileName) const
{
	// Saves an instrument to a file
	//

	CInstrumentFile file(FileName, CFile::modeCreate | CFile::modeWrite);
	auto pInstrument = GetInstrument(Index);
	ASSERT(pInstrument);
	
	if (file.m_hFile == CFile::hFileNull) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR, MB_ICONERROR);
		return;
	}

	// Write header
	file.Write(INST_HEADER, (UINT)strlen(INST_HEADER));
	file.Write(INST_VERSION, (UINT)strlen(INST_VERSION));

	// Write type
	file.WriteChar(pInstrument->GetType());

	// Write name
	char Name[256];
	pInstrument->GetName(Name);
	int NameLen = (int)strlen(Name);
	file.WriteInt(NameLen);
	file.Write(Name, NameLen);

	// Write instrument data
	pInstrument->SaveFile(&file);

	file.Close();
}

int CFamiTrackerDoc::LoadInstrument(CString FileName)
{
	// Loads an instrument from file, return allocated slot or INVALID_INSTRUMENT if failed
	//
	int iInstMaj, iInstMin;
	sscanf_s(INST_VERSION, "%i.%i", &iInstMaj, &iInstMin);
	static const int I_CURRENT_VER = iInstMaj * 10 + iInstMin;
	
	int Slot = m_pInstrumentManager->GetFirstUnused();
	try {
		if (Slot == INVALID_INSTRUMENT)
			throw IDS_INST_LIMIT;

		// Open file
		// // // CFile implements RAII
		CInstrumentFile file(FileName, CFile::modeRead);
		if (file.m_hFile == CFile::hFileNull)
			throw IDS_FILE_OPEN_ERROR;

		// Signature
		const UINT HEADER_LEN = strlen(INST_HEADER);
		char Text[256] = {};
		file.Read(Text, HEADER_LEN);
		if (strcmp(Text, INST_HEADER) != 0)
			throw IDS_INSTRUMENT_FILE_FAIL;
		
		// Version
		file.Read(Text, static_cast<UINT>(strlen(INST_VERSION)));
		sscanf_s(Text, "%i.%i", &iInstMaj, &iInstMin);		// // //
		int iInstVer = iInstMaj * 10 + iInstMin;
		if (iInstVer > I_CURRENT_VER)
			throw IDS_INST_VERSION_UNSUPPORTED;
		
		m_csDocumentLock.Lock();

		inst_type_t InstType = static_cast<inst_type_t>(file.ReadChar());
		if (InstType == INST_NONE)
			InstType = INST_2A03;
		auto pInstrument = CInstrumentManager::CreateNew(InstType);
		AssertFileData(pInstrument, "Failed to create instrument");
		m_pInstrumentManager->InsertInstrument(Slot, pInstrument);
		
		// Name
		unsigned int NameLen = AssertRange(static_cast<int>(file.ReadInt()), 0, CInstrument::INST_NAME_MAX, "Instrument name length");
		file.Read(Text, NameLen);
		Text[NameLen] = 0;
		pInstrument->SetName(Text);

		pInstrument->LoadFile(&file, iInstVer);		// // //
		m_csDocumentLock.Unlock();
		return Slot;
	}
	catch (int ID) {		// // // TODO: put all error messages into string table then add exception ctor
		m_csDocumentLock.Unlock();
		AfxMessageBox(ID, MB_ICONERROR);
		return INVALID_INSTRUMENT;
	}
	catch (CModuleException *e) {
		m_csDocumentLock.Unlock();
		m_pInstrumentManager->RemoveInstrument(Slot);
		AfxMessageBox(e->GetErrorString().c_str(), MB_ICONERROR);
		delete e;
		return INVALID_INSTRUMENT;
	}
}

//
// // // General document
//

void CFamiTrackerDoc::SetFrameCount(unsigned int Track, unsigned int Count)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Count <= MAX_FRAMES);

	CPatternData *pTrack = GetTrack(Track);
	if (pTrack->GetFrameCount() != Count) {
		pTrack->SetFrameCount(Count);
		SetModifiedFlag();
		SetExceededFlag();			// // //
		if (m_pBookmarkList[Track]) {
			auto it = m_pBookmarkList[Track]->begin();
			while (it != m_pBookmarkList[Track]->end()) {
				if (it->Frame >= Count)
					it = m_pBookmarkList[Track]->erase(it);
				else it++;
			}
		}
	}
}

void CFamiTrackerDoc::SetPatternLength(unsigned int Track, unsigned int Length)
{ 
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Length <= MAX_PATTERN_LENGTH);

	CPatternData *pTrack = GetTrack(Track);
	if (pTrack->GetPatternLength() != Length) {
		pTrack->SetPatternLength(Length);
		SetModifiedFlag();
		SetExceededFlag();			// // //
	}
}

void CFamiTrackerDoc::SetSongSpeed(unsigned int Track, unsigned int Speed)
{
	ASSERT(Track < MAX_TRACKS);
	CPatternData *pTrack = GetTrack(Track);
	if (pTrack->GetSongGroove())		// // //
		ASSERT(Speed < MAX_GROOVE);
	else
		ASSERT(Speed <= MAX_TEMPO);

	if (pTrack->GetSongSpeed() != Speed) {
		pTrack->SetSongSpeed(Speed);
		SetModifiedFlag();
		SetExceededFlag();			// // //
	}
}

void CFamiTrackerDoc::SetSongTempo(unsigned int Track, unsigned int Tempo)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Tempo <= MAX_TEMPO);

	CPatternData *pTrack = GetTrack(Track);
	if (pTrack->GetSongTempo() != Tempo) {
		pTrack->SetSongTempo(Tempo);
		SetModifiedFlag();
		SetExceededFlag();			// // //
	}
}

void CFamiTrackerDoc::SetSongGroove(unsigned int Track, bool Groove)		// // //
{
	CPatternData *pTrack = GetTrack(Track);
	if (pTrack->GetSongGroove() != Groove) {
		pTrack->SetSongGroove(Groove);
		SetModifiedFlag();
		SetExceededFlag();
	}
}

unsigned int CFamiTrackerDoc::GetPatternLength(unsigned int Track) const
{ 
	ASSERT(Track < MAX_TRACKS);
	return GetTrack(Track)->GetPatternLength(); 
}

unsigned int CFamiTrackerDoc::GetFrameCount(unsigned int Track) const 
{ 
	ASSERT(Track < MAX_TRACKS);
	return GetTrack(Track)->GetFrameCount(); 
}

unsigned int CFamiTrackerDoc::GetSongSpeed(unsigned int Track) const
{ 
	ASSERT(Track < MAX_TRACKS);
	return GetTrack(Track)->GetSongSpeed(); 
}

unsigned int CFamiTrackerDoc::GetSongTempo(unsigned int Track) const
{ 
	ASSERT(Track < MAX_TRACKS);
	return GetTrack(Track)->GetSongTempo(); 
}

bool CFamiTrackerDoc::GetSongGroove(unsigned int Track) const		// // //
{ 
	ASSERT(Track < MAX_TRACKS);
	return GetTrack(Track)->GetSongGroove();
}

unsigned int CFamiTrackerDoc::GetEffColumns(unsigned int Track, unsigned int Channel) const
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Channel < MAX_CHANNELS);
	return GetTrack(Track)->GetEffectColumnCount(Channel);
}

void CFamiTrackerDoc::SetEffColumns(unsigned int Track, unsigned int Channel, unsigned int Columns)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Columns < MAX_EFFECT_COLUMNS);

	GetChannel(Channel)->SetColumnCount(Columns);
	GetTrack(Track)->SetEffectColumnCount(Channel, Columns);

	SetModifiedFlag();
}

void CFamiTrackerDoc::SetEngineSpeed(unsigned int Speed)
{
	ASSERT(Speed <= 800); // hardcoded at the moment, TODO: fix this
	ASSERT(Speed >= 10 || Speed == 0);

	m_iEngineSpeed = Speed;
	SetModifiedFlag();
	SetExceededFlag();		// // //
}

void CFamiTrackerDoc::SetMachine(machine_t Machine)
{
	ASSERT(Machine == PAL || Machine == NTSC);
	m_iMachine = Machine;
	SetModifiedFlag();
	SetExceededFlag();		// // //
}

unsigned int CFamiTrackerDoc::GetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel) const
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES && Channel < MAX_CHANNELS);
	return GetTrack(Track)->GetFramePattern(Frame, Channel);
}

void CFamiTrackerDoc::SetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Pattern)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Pattern < MAX_PATTERN);

	GetTrack(Track)->SetFramePattern(Frame, Channel, Pattern);
}

unsigned int CFamiTrackerDoc::GetFrameRate() const
{
	if (m_iEngineSpeed == 0)
		return (m_iMachine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;
	
	return m_iEngineSpeed;
}

//// Pattern functions ////////////////////////////////////////////////////////////////////////////////

void CFamiTrackerDoc::SetNoteData(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, const stChanNote *pData)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(pData != NULL);
	// Get notes from the pattern
	CPatternData *pTrack = GetTrack(Track);
	int Pattern = pTrack->GetFramePattern(Frame, Channel);
	memcpy(pTrack->GetPatternData(Channel, Pattern, Row), pData, sizeof(stChanNote));
	SetModifiedFlag();
}

void CFamiTrackerDoc::GetNoteData(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *pData) const
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(pData != NULL);
	// Sets the notes of the pattern
	CPatternData *pTrack = GetTrack(Track);
	int Pattern = pTrack->GetFramePattern(Frame, Channel);
	memcpy(pData, pTrack->GetPatternData(Channel, Pattern, Row), sizeof(stChanNote));
}

void CFamiTrackerDoc::SetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, const stChanNote *pData)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Pattern < MAX_PATTERN);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(pData != NULL);
	// Set a note to a direct pattern
	CPatternData *pTrack = GetTrack(Track);
	memcpy(pTrack->GetPatternData(Channel, Pattern, Row), pData, sizeof(stChanNote));
	SetModifiedFlag();
}

void CFamiTrackerDoc::GetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *pData) const
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Pattern < MAX_PATTERN);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);
	ASSERT(pData != NULL);

	// Get note from a direct pattern
	CPatternData *pTrack = GetTrack(Track);
	memcpy(pData, pTrack->GetPatternData(Channel, Pattern, Row), sizeof(stChanNote));
}

bool CFamiTrackerDoc::InsertRow(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	CPatternData *pTrack = GetTrack(Track);
	int Pattern = pTrack->GetFramePattern(Frame, Channel);
	int PatternLen = pTrack->GetPatternLength();
	stChanNote Note = BLANK_NOTE;		// // //

	for (unsigned int i = PatternLen - 1; i > Row; i--) {
		memcpy(
			pTrack->GetPatternData(Channel, Pattern, i), 
			pTrack->GetPatternData(Channel, Pattern, i - 1), 
			sizeof(stChanNote));
	}

	*pTrack->GetPatternData(Channel, Pattern, Row) = Note;

	SetModifiedFlag();

	return true;
}

void CFamiTrackerDoc::ClearPatterns(unsigned int Track)
{
	ASSERT(Track < MAX_TRACKS);

	CPatternData *pTrack = GetTrack(Track);
	pTrack->ClearEverything();

	SetModifiedFlag();
}

void CFamiTrackerDoc::ClearPattern(unsigned int Track, unsigned int Frame, unsigned int Channel)
{
	// Clear entire pattern
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);

	CPatternData *pTrack = GetTrack(Track);
	int Pattern = pTrack->GetFramePattern(Frame, Channel);
	pTrack->ClearPattern(Channel, Pattern);

	SetModifiedFlag();
}

bool CFamiTrackerDoc::ClearRow(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	CPatternData *pTrack = GetTrack(Track);
	int Pattern = pTrack->GetFramePattern(Frame, Channel);
	*pTrack->GetPatternData(Channel, Pattern, Row) = BLANK_NOTE;		// // //
	
	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::ClearRowField(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, cursor_column_t Column)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	CPatternData *pTrack = GetTrack(Track);
	int Pattern = pTrack->GetFramePattern(Frame, Channel);
	stChanNote *pNote = pTrack->GetPatternData(Channel, Pattern, Row);

	switch (Column) {
		case C_NOTE:			// Note
			pNote->Note = 0;
			pNote->Octave = 0;
			pNote->Instrument = MAX_INSTRUMENTS;	// Fix the old behaviour
			pNote->Vol = MAX_VOLUME;
			break;
		case C_INSTRUMENT1:		// Instrument
		case C_INSTRUMENT2:
			pNote->Instrument = MAX_INSTRUMENTS;
			break;
		case C_VOLUME:			// Volume
			pNote->Vol = MAX_VOLUME;
			break;
		case C_EFF1_NUM:			// Effect 1
		case C_EFF1_PARAM1:
		case C_EFF1_PARAM2:
			pNote->EffNumber[0] = EF_NONE;
			pNote->EffParam[0] = 0;
			break;
		case C_EFF2_NUM:		// Effect 2
		case C_EFF2_PARAM1:
		case C_EFF2_PARAM2:
			pNote->EffNumber[1] = EF_NONE;
			pNote->EffParam[1] = 0;
			break;
		case C_EFF3_NUM:		// Effect 3
		case C_EFF3_PARAM1:
		case C_EFF3_PARAM2:
			pNote->EffNumber[2] = EF_NONE;
			pNote->EffParam[2] = 0;
			break;
		case C_EFF4_NUM:		// Effect 4
		case C_EFF4_PARAM1:
		case C_EFF4_PARAM2:
			pNote->EffNumber[3] = EF_NONE;
			pNote->EffParam[3] = 0;
			break;
	}
	
	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::RemoveNote(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);
	ASSERT(Channel < MAX_CHANNELS);
	ASSERT(Row < MAX_PATTERN_LENGTH);

	CPatternData *pTrack = GetTrack(Track);
	int Pattern = pTrack->GetFramePattern(Frame, Channel);
	stChanNote Note = BLANK_NOTE;		// // //

	unsigned int PatternLen = pTrack->GetPatternLength();

	for (unsigned int i = Row - 1; i < (PatternLen - 1); i++) {
		memcpy(
			pTrack->GetPatternData(Channel, Pattern, i), 
			pTrack->GetPatternData(Channel, Pattern, i + 1),
			sizeof(stChanNote));
	}

	*pTrack->GetPatternData(Channel, Pattern, PatternLen - 1) = Note;

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::PullUp(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row)
{
	// Todo: Use Track
	ASSERT(Track < MAX_TRACKS);

	int PatternLen = GetPatternLength(Track);
	stChanNote Data;

	for (int i = Row; i < PatternLen - 1; ++i) {
		GetNoteData(Track, Frame, Channel, i + 1, &Data);
		SetNoteData(Track, Frame, Channel, i, &Data);
	}

	// Last note on pattern
	ClearRow(Track, Frame, Channel, PatternLen - 1);

	SetModifiedFlag();

	return true;
}

void CFamiTrackerDoc::CopyPattern(unsigned int Track, int Target, int Source, int Channel)
{
	// Copy one pattern to another
	ASSERT(Track < MAX_TRACKS);

	int PatternLen = GetPatternLength(Track);
	stChanNote Data;

	for (int i = 0; i < PatternLen; ++i) {
		GetDataAtPattern(Track, Source, Channel, i, &Data);
		SetDataAtPattern(Track, Target, Channel, i, &Data);
	}

	SetModifiedFlag();
}

void CFamiTrackerDoc::SwapChannels(unsigned int Track, unsigned int First, unsigned int Second)		// // //
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(First < MAX_CHANNELS);
	ASSERT(Second < MAX_CHANNELS);

	CPatternData *pTrack = GetTrack(Track);
	pTrack->SwapChannels(First, Second);

	unsigned int Temp = GetEffColumns(Track, First);
	SetEffColumns(Track, First, GetEffColumns(Track, Second));
	SetEffColumns(Track, Second, Temp);

	SetModifiedFlag();
}

//// Frame functions //////////////////////////////////////////////////////////////////////////////////

bool CFamiTrackerDoc::InsertFrame(unsigned int Track, unsigned int Frame)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);

	int FrameCount = GetFrameCount(Track);
	int Channels = GetAvailableChannels();

	if (FrameCount == MAX_FRAMES)
		return false;

	SetFrameCount(Track, FrameCount + 1);

	for (unsigned int i = FrameCount; i > Frame; --i) {
		for (int j = 0; j < Channels; ++j) {
			SetPatternAtFrame(Track, i, j, GetPatternAtFrame(Track, i - 1, j));
		}
	}

	// Select free patterns 
	for (int i = 0; i < Channels; ++i) {
		SetPatternAtFrame(Track, Frame, i, GetFirstFreePattern(Track, i));
	}

	if (m_pBookmarkList[Track]) {		// // //
		for (auto it = m_pBookmarkList[Track]->begin(); it < m_pBookmarkList[Track]->end(); it++) {
			if (it->Frame >= Frame)
				it->Frame++;
		}
	}

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::RemoveFrame(unsigned int Track, unsigned int Frame)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);

	int FrameCount = GetFrameCount(Track);
	int Channels = GetAvailableChannels();

	for (int i = 0; i < Channels; ++i) {
		SetPatternAtFrame(Track, Frame, i, 0);
	}

	if (FrameCount == 1)
		return false;

	for (int i = Frame; i < FrameCount - 1; ++i) {
		for (int j = 0; j < Channels; ++j) {
			SetPatternAtFrame(Track, i, j, GetPatternAtFrame(Track, i + 1, j));
		}
	}

	if (m_pBookmarkList[Track]) {		// // //
		auto it = m_pBookmarkList[Track]->begin();
		while (it < m_pBookmarkList[Track]->end()) {
			if (it->Frame == Frame)
				it = m_pBookmarkList[Track]->erase(it);
			else {
				if (it->Frame > Frame)
					it->Frame--;
				it++;
			}
		}
	}

	SetFrameCount(Track, FrameCount - 1);

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::DuplicateFrame(unsigned int Track, unsigned int Frame)
{
	// Create a copy of selected frame
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);

	int Frames = GetFrameCount(Track);
	int Channels = GetAvailableChannels();

	if (Frames == MAX_FRAMES)
		return false;

	SetFrameCount(Track, Frames + 1);

	for (unsigned int i = Frames; i > (Frame + 1); --i) {
		for (int j = 0; j < Channels; ++j) {
			SetPatternAtFrame(Track, i, j, GetPatternAtFrame(Track, i - 1, j));
		}
	}

	for (int i = 0; i < Channels; ++i) {
		SetPatternAtFrame(Track, Frame + 1, i, GetPatternAtFrame(Track, Frame, i));
	}

	if (m_pBookmarkList[Track]) {		// // //
		for (auto it = m_pBookmarkList[Track]->begin(); it < m_pBookmarkList[Track]->end(); it++) {
			if (it->Frame >= Frame)
				it->Frame++;
		}
	}

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::DuplicatePatterns(unsigned int Track, unsigned int Frame)
{
	ASSERT(Track < MAX_TRACKS);

	// Create a copy of selected frame including patterns
	int Frames = GetFrameCount(Track);
	int Channels = GetAvailableChannels();

	// insert new frame with next free pattern numbers
	if (!InsertFrame(Track, Frame))
		return false;

	// copy old patterns into new
	for (int i = 0; i < Channels; ++i) {
		for(unsigned int j = 0; j < MAX_PATTERN_LENGTH; j++) {
			stChanNote note;
			GetNoteData(Track, Frame - 1, i, j, &note);
			SetNoteData(Track, Frame, i, j, &note);
		}
	}

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::MoveFrameDown(unsigned int Track, unsigned int Frame)
{
	int Channels = GetAvailableChannels();

	if (Frame == (GetFrameCount(Track) - 1))
		return false;

	for (int i = 0; i < Channels; ++i) {
		int Pattern = GetPatternAtFrame(Track, Frame, i);
		SetPatternAtFrame(Track, Frame, i, GetPatternAtFrame(Track, Frame + 1, i));
		SetPatternAtFrame(Track, Frame + 1, i, Pattern);
	}

	if (m_pBookmarkList[Track]) {		// // //
		for (auto it = m_pBookmarkList[Track]->begin(); it < m_pBookmarkList[Track]->end(); it++) {
			if (it->Frame == Frame)
				it->Frame++;
			else if (it->Frame == Frame + 1)
				it->Frame--;
		}
	}

	SetModifiedFlag();

	return true;
}

bool CFamiTrackerDoc::MoveFrameUp(unsigned int Track, unsigned int Frame)
{
	int Channels = GetAvailableChannels();

	if (Frame == 0)
		return false;

	for (int i = 0; i < Channels; ++i) {
		int Pattern = GetPatternAtFrame(Track, Frame, i);
		SetPatternAtFrame(Track, Frame, i, GetPatternAtFrame(Track, Frame - 1, i));
		SetPatternAtFrame(Track, Frame - 1, i, Pattern);
	}

	if (m_pBookmarkList[Track]) {		// // //
		for (auto it = m_pBookmarkList[Track]->begin(); it < m_pBookmarkList[Track]->end(); it++) {
			if (it->Frame == Frame)
				it->Frame--;
			else if (it->Frame == Frame + 1)
				it->Frame++;
		}
	}

	SetModifiedFlag();

	return true;
}

void CFamiTrackerDoc::DeleteFrames(unsigned int Track, unsigned int Frame, int Count)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(Frame < MAX_FRAMES);

	for (int i = 0; i < Count; ++i) {
		RemoveFrame(Track, Frame);
	}

	SetModifiedFlag();
}

//// Track functions //////////////////////////////////////////////////////////////////////////////////

CString CFamiTrackerDoc::GetTrackTitle(unsigned int Track) const
{
	ASSERT(Track < MAX_TRACKS);
	return m_sTrackNames[Track];
}

int CFamiTrackerDoc::AddTrack()
{
	// Add new track. Returns -1 on failure, or added track number otherwise

	int NewTrack = m_iTrackCount;

	if (NewTrack >= MAX_TRACKS)
		return -1;

	AllocateTrack(NewTrack);

	++m_iTrackCount;

	SetModifiedFlag();
	SetExceededFlag();		// // //

	return NewTrack;
}

void CFamiTrackerDoc::RemoveTrack(unsigned int Track)
{
	ASSERT(Track < MAX_TRACKS);
	ASSERT(m_iTrackCount > 1);
	ASSERT(m_pTracks[Track] != NULL);

	delete m_pTracks[Track];

	// Move down all other tracks
	for (unsigned int i = Track; i < m_iTrackCount - 1; ++i) {
		m_sTrackNames[i] = m_sTrackNames[i + 1];
		m_pTracks[i] = m_pTracks[i + 1];
		m_pBookmarkList[i] = m_pBookmarkList[i + 1];		// // //
	}

	m_pTracks[m_iTrackCount - 1] = NULL;
	m_pBookmarkList[m_iTrackCount - 1] = NULL;

	--m_iTrackCount;
/*
	if (m_iTrack >= m_iTrackCount)
		m_iTrack = m_iTrackCount - 1;	// Last track was removed
*/
//	SwitchToTrack(m_iTrack);

	SetModifiedFlag();
	SetExceededFlag();		// // //
}

void CFamiTrackerDoc::SetTrackTitle(unsigned int Track, const CString &title)
{
	if (m_sTrackNames[Track] == title)
		return;

	m_sTrackNames[Track] = title;
	SetModifiedFlag();
	SetExceededFlag();		// // //
}

void CFamiTrackerDoc::MoveTrackUp(unsigned int Track)
{
	ASSERT(Track > 0);

	SwapTracks(Track, Track - 1);
	SetModifiedFlag();
	SetExceededFlag();		// // //
}

void CFamiTrackerDoc::MoveTrackDown(unsigned int Track)
{
	ASSERT(Track < MAX_TRACKS);

	SwapTracks(Track, Track + 1);
	SetModifiedFlag();
	SetExceededFlag();		// // //
}

void CFamiTrackerDoc::SwapTracks(unsigned int Track1, unsigned int Track2)
{
	std::swap(m_sTrackNames[Track1], m_sTrackNames[Track2]);		// // //
	std::swap(m_pTracks[Track1], m_pTracks[Track2]);
	std::swap(m_pBookmarkList[Track1], m_pBookmarkList[Track2]);
}

void CFamiTrackerDoc::AllocateTrack(unsigned int Track)
{
	// Allocate a new song if not already done
	if (m_pTracks[Track] == NULL) {
		int Tempo = (m_iMachine == NTSC) ? DEFAULT_TEMPO_NTSC : DEFAULT_TEMPO_PAL;
		m_pTracks[Track] = new CPatternData(DEFAULT_ROW_COUNT);		// // //
		m_pTracks[Track]->SetSongTempo(Tempo);
		m_sTrackNames[Track] = DEFAULT_TRACK_NAME;
	}
}

CPatternData* CFamiTrackerDoc::GetTrack(unsigned int Track)
{
	ASSERT(Track < MAX_TRACKS);
	// Ensure track is allocated
	AllocateTrack(Track);
	return m_pTracks[Track];
}

CPatternData* CFamiTrackerDoc::GetTrack(unsigned int Track) const
{
	// TODO make m_pTracks mutable instead?
	ASSERT(Track < MAX_TRACKS);
	ASSERT(m_pTracks[Track] != NULL);

	return m_pTracks[Track];
}

unsigned int CFamiTrackerDoc::GetTrackCount() const
{
	return m_iTrackCount;
}

void CFamiTrackerDoc::SelectExpansionChip(unsigned char Chip, bool Move)
{
	// // // Move pattern data upon removing expansion chips
	if (Move) {
		int oldIndex[CHANNELS] = {};
		int newIndex[CHANNELS] = {};
		for (int j = 0; j < CHANNELS; j++) {
			oldIndex[j] = GetChannelPosition(j, m_iExpansionChip);
			newIndex[j] = GetChannelPosition(j, Chip);
		}
		for (unsigned int i = 0; i < m_iTrackCount; ++i) {
			CPatternData *pTrack = m_pTracks[i];
			CPatternData *pNew = new CPatternData(GetPatternLength(i));
			pNew->SetHighlight(pTrack->GetRowHighlight());
			pNew->SetSongTempo(pTrack->GetSongTempo());
			pNew->SetSongSpeed(pTrack->GetSongSpeed());
			pNew->SetSongGroove(pTrack->GetSongGroove());
			pNew->SetFrameCount(pTrack->GetFrameCount());
			for (int j = 0; j < CHANNELS; j++) {
				if (oldIndex[j] != -1 && newIndex[j] != -1) {
					pNew->SetEffectColumnCount(newIndex[j], pTrack->GetEffectColumnCount(oldIndex[j]));
					for (int f = 0; f < MAX_FRAMES; f++)
						pNew->SetFramePattern(f, newIndex[j], pTrack->GetFramePattern(f, oldIndex[j]));
					for (int p = 0; p < MAX_PATTERN; p++)
						memcpy(pNew->GetPatternData(newIndex[j], p, 0), pTrack->GetPatternData(oldIndex[j], p, 0), MAX_PATTERN_LENGTH * sizeof(stChanNote));
				}
			}
			SAFE_RELEASE(pTrack);
			m_pTracks[i] = pNew;
		}
	}
	// Complete sound chip setup
	SetupChannels(Chip);
	ApplyExpansionChip();

	if (!(Chip & SNDCHIP_N163))		// // //
		m_iNamcoChannels = 0;
}

void CFamiTrackerDoc::SetupChannels(unsigned char Chip)
{
	// This will select a chip in the sound emulator

	if (Chip != SNDCHIP_NONE) {
		// Do not allow expansion chips in PAL mode
		SetMachine(NTSC);
	}

	// Store the chip
	m_iExpansionChip = Chip;

	// Register the channels
	theApp.GetSoundGenerator()->RegisterChannels(Chip, this); 

	m_iChannelsAvailable = GetChannelCount();

	/*if (Chip & SNDCHIP_N163) {			// // //
		m_iChannelsAvailable -= (8 - m_iNamcoChannels);
	}*/

	// Must call ApplyExpansionChip after this
}

void CFamiTrackerDoc::ApplyExpansionChip()
{
	// Tell the sound emulator to switch expansion chip
	theApp.GetSoundGenerator()->SelectChip(m_iExpansionChip);

	// Change period tables
	theApp.GetSoundGenerator()->LoadMachineSettings(m_iMachine, m_iEngineSpeed, m_iNamcoChannels);

	SetModifiedFlag();
	SetExceededFlag();			// // //
}

CSequenceManager *const CFamiTrackerDoc::GetSequenceManager(int InstType) const
{
	return m_pInstrumentManager->GetSequenceManager(InstType);
}

CInstrumentManager *const CFamiTrackerDoc::GetInstrumentManager() const
{
	return m_pInstrumentManager;
}

bool CFamiTrackerDoc::ExpansionEnabled(int Chip) const
{
	// Returns true if a specified chip is enabled
	return (GetExpansionChip() & Chip) == Chip; 
}

void CFamiTrackerDoc::SetNamcoChannels(int Channels, bool Move)
{
	int oldIndex[CHANNELS] = {};		// // //
	int newIndex[CHANNELS] = {};
	for (int j = 0; j < CHANNELS; j++)
		oldIndex[j] = GetChannelIndex(j);

	if (Channels == 0) {		// // //
		SelectExpansionChip(m_iExpansionChip & ~SNDCHIP_N163, true);
		return;
	}
	if (!ExpansionEnabled(SNDCHIP_N163))
		SelectExpansionChip(m_iExpansionChip | SNDCHIP_N163, true);

	ASSERT(Channels <= 8);
	m_iNamcoChannels = Channels;
	
	// // // Move pattern data upon removing N163 channels
	if (Move) {
		for (int j = 0; j < CHANNELS; j++)
			newIndex[j] = GetChannelPosition(j, m_iExpansionChip);
		for (unsigned int i = 0; i < m_iTrackCount; ++i) {
			CPatternData *pTrack = m_pTracks[i];
			CPatternData *pNew = new CPatternData(GetPatternLength(i));
			pNew->SetHighlight(pTrack->GetRowHighlight());
			pNew->SetSongTempo(pTrack->GetSongTempo());
			pNew->SetSongSpeed(pTrack->GetSongSpeed());
			pNew->SetSongGroove(pTrack->GetSongGroove());
			pNew->SetFrameCount(pTrack->GetFrameCount());
			for (int j = 0; j < CHANNELS; j++) {
				if (oldIndex[j] != -1 && newIndex[j] != -1) {
					pNew->SetEffectColumnCount(newIndex[j], pTrack->GetEffectColumnCount(oldIndex[j]));
					for (int f = 0; f < MAX_FRAMES; f++)
						pNew->SetFramePattern(f, newIndex[j], pTrack->GetFramePattern(f, oldIndex[j]));
					for (int p = 0; p < MAX_PATTERN; p++)
						memcpy(pNew->GetPatternData(newIndex[j], p, 0), pTrack->GetPatternData(oldIndex[j], p, 0), MAX_PATTERN_LENGTH * sizeof(stChanNote));
				}
			}
			SAFE_RELEASE(pTrack);
			m_pTracks[i] = pNew;
		}
	}
}

int CFamiTrackerDoc::GetNamcoChannels() const
{
	if (!ExpansionEnabled(SNDCHIP_N163)) return 0;		// // //
	return m_iNamcoChannels;
}

void CFamiTrackerDoc::ConvertSequence(stSequence *pOldSequence, CSequence *pNewSequence, int Type)
{
	// This function is used to convert old version sequences (used by older file versions)
	// to the current version

	if (pOldSequence->Count == 0 || pOldSequence->Count >= MAX_SEQUENCE_ITEMS)
		return;	// Invalid sequence

	// Save a pointer to this
	int iLoopPoint = -1;
	int iLength = 0;
	int ValPtr = 0;

	// Store the sequence
	int Count = pOldSequence->Count;

	for (int i = 0; i < Count; ++i) {
		int Value  = pOldSequence->Value[i];
		int Length = pOldSequence->Length[i];

		if (Length < 0) {
			iLoopPoint = 0;
			for (int l = signed(pOldSequence->Count) + Length - 1; l < signed(pOldSequence->Count) - 1; l++)
				iLoopPoint += (pOldSequence->Length[l] + 1);
		}
		else {
			for (int l = 0; l < Length + 1; l++) {
				if ((Type == SEQ_PITCH || Type == SEQ_HIPITCH) && l > 0)
					pNewSequence->SetItem(ValPtr++, 0);
				else
					pNewSequence->SetItem(ValPtr++, (unsigned char)Value);
				iLength++;
			}
		}
	}

	if (iLoopPoint != -1) {
		if (iLoopPoint > iLength)
			iLoopPoint = iLength;
		iLoopPoint = iLength - iLoopPoint;
	}

	pNewSequence->SetItemCount(ValPtr);
	pNewSequence->SetLoopPoint(iLoopPoint);
}

unsigned int CFamiTrackerDoc::GetFirstFreePattern(unsigned int Track, unsigned int Channel) const
{
	CPatternData *pTrack = GetTrack(Track);

	for (int i = 0; i < MAX_PATTERN; ++i) {
		if (!pTrack->IsPatternInUse(Channel, i) && pTrack->IsPatternEmpty(Channel, i))
			return i;
	}

	return 0;
}

bool CFamiTrackerDoc::IsPatternEmpty(unsigned int Track, unsigned int Channel, unsigned int Pattern) const
{
	return GetTrack(Track)->IsPatternEmpty(Channel, Pattern);
}

// Channel interface, these functions must be synchronized!!!

int CFamiTrackerDoc::GetChannelType(int Channel) const
{
	ASSERT(m_iRegisteredChannels != 0);
	ASSERT(Channel < m_iRegisteredChannels);
	return m_iChannelTypes[Channel];
}

int CFamiTrackerDoc::GetChipType(int Channel) const
{
	ASSERT(m_iRegisteredChannels != 0);
	ASSERT(Channel < m_iRegisteredChannels);
	return m_pChannels[Channel]->GetChip();
}

int CFamiTrackerDoc::GetChannelCount() const
{
	return m_iRegisteredChannels;
}

int CFamiTrackerDoc::GetChannelPosition(int Channel, unsigned char Chip)		// // //
{
	unsigned int pos = Channel;
	if (pos == CHANID_MMC5_VOICE) return -1;

	if (!(Chip & SNDCHIP_S5B)) {
		if (pos > CHANID_S5B_CH3) pos -= 3;
		else if (pos >= CHANID_S5B_CH1) return -1;
	}
	if (!(Chip & SNDCHIP_VRC7)) {
		if (pos > CHANID_VRC7_CH6) pos -= 6;
		else if (pos >= CHANID_VRC7_CH1) return -1;
	}
	if (!(Chip & SNDCHIP_FDS)) {
		if (pos > CHANID_FDS) pos -= 1;
		else if (pos >= CHANID_FDS) return -1;
	}
		if (pos > CHANID_N163_CH8) pos -= 8 - (!(Chip & SNDCHIP_N163) ? 0 : m_iNamcoChannels);
		else if (pos > CHANID_MMC5_VOICE + (!(Chip & SNDCHIP_N163) ? 0 : m_iNamcoChannels)) return -1;
	if (pos > CHANID_MMC5_VOICE) pos -= 1;
	if (!(Chip & SNDCHIP_MMC5)) {
		if (pos > CHANID_MMC5_SQUARE2) pos -= 2;
		else if (pos >= CHANID_MMC5_SQUARE1) return -1;
	}
	if (!(Chip & SNDCHIP_VRC6)) {
		if (pos > CHANID_VRC6_SAWTOOTH) pos -= 3;
		else if (pos >= CHANID_VRC6_PULSE1) return -1;
	}

	return pos;
}

void CFamiTrackerDoc::ResetChannels()
{
	// Clears all channels from the document
	m_iRegisteredChannels = 0;
}

void CFamiTrackerDoc::RegisterChannel(CTrackerChannel *pChannel, int ChannelType, int ChipType)
{
	// Adds a channel to the document
	m_pChannels[m_iRegisteredChannels] = pChannel;
	m_iChannelTypes[m_iRegisteredChannels] = ChannelType;
	m_iChannelChip[m_iRegisteredChannels] = ChipType;
	m_iRegisteredChannels++;
}

CTrackerChannel *CFamiTrackerDoc::GetChannel(int Index) const
{
	ASSERT(m_iRegisteredChannels != 0 && Index < m_iRegisteredChannels);
	ASSERT(m_pChannels[Index] != NULL);
	return m_pChannels[Index];
}

int CFamiTrackerDoc::GetChannelIndex(int Channel) const
{
	// Translate channel ID to index, returns -1 if not found
	ASSERT(m_iRegisteredChannels != 0);
	for (int i = 0; i < m_iRegisteredChannels; ++i) {
		if (m_pChannels[i]->GetID() == Channel)
			return i;
	}
	return -1;
}

// Vibrato functions

vibrato_t CFamiTrackerDoc::GetVibratoStyle() const
{
	return m_iVibratoStyle;
}

void CFamiTrackerDoc::SetVibratoStyle(vibrato_t Style)
{
	m_iVibratoStyle = Style;
	theApp.GetSoundGenerator()->SetupVibratoTable(Style);
}

// Linear pitch slides

bool CFamiTrackerDoc::GetLinearPitch() const
{
	return m_bLinearPitch;
}

void CFamiTrackerDoc::SetLinearPitch(bool Enable)
{
	m_bLinearPitch = Enable;
}

// Attributes

CString CFamiTrackerDoc::GetFileTitle() const 
{
	// Return file name without extension
	CString FileName = GetTitle();

	static const LPCSTR EXT[] = {_T(".ftm"), _T(".0cc"), _T(".ftm.bak"), _T(".0cc.bak")};		// // //
	// Remove extension

	for (size_t i = 0; i < sizeof(EXT) / sizeof(LPCSTR); ++i) {
		int Len = lstrlen(EXT[i]);
		if (FileName.Right(Len).CompareNoCase(EXT[i]) == 0)
			return FileName.Left(FileName.GetLength() - Len);
	}

	return FileName;
}

bool CFamiTrackerDoc::IsFileLoaded() const
{
	return m_bFileLoaded;
}

bool CFamiTrackerDoc::HasLastLoadFailed() const
{
	return m_bFileLoadFailed;
}

#ifdef AUTOSAVE

// Auto-save (experimental)

void CFamiTrackerDoc::SetupAutoSave()
{
	TCHAR TempPath[MAX_PATH], TempFile[MAX_PATH];

	GetTempPath(MAX_PATH, TempPath);
	GetTempFileName(TempPath, _T("Aut"), 21587, TempFile);

	// Check if file exists
	CFile file;
	if (file.Open(TempFile, CFile::modeRead)) {
		file.Close();
		if (AfxMessageBox(_T("It might be possible to recover last document, do you want to try?"), MB_YESNO) == IDYES) {
			OpenDocument(TempFile);
			SelectExpansionChip(m_iExpansionChip);
		}
		else {
			DeleteFile(TempFile);
		}
	}

	TRACE("Doc: Allocated file for auto save: ");
	TRACE(TempFile);
	TRACE("\n");

	m_sAutoSaveFile = TempFile;
}

void CFamiTrackerDoc::ClearAutoSave()
{
	if (m_sAutoSaveFile.GetLength() == 0)
		return;

	DeleteFile(m_sAutoSaveFile);

	m_sAutoSaveFile = _T("");
	m_iAutoSaveCounter = 0;

	TRACE("Doc: Removed auto save file\n");
}

void CFamiTrackerDoc::AutoSave()
{
	// Autosave
	if (!m_iAutoSaveCounter || !m_bFileLoaded || m_sAutoSaveFile.GetLength() == 0)
		return;

	m_iAutoSaveCounter--;

	if (m_iAutoSaveCounter == 0) {
		TRACE("Doc: Performing auto save\n");
		SaveDocument(m_sAutoSaveFile);
	}
}

#endif

//
// Comment functions
//

void CFamiTrackerDoc::SetComment(CString &comment, bool bShowOnLoad)
{
	m_strComment = comment;
	m_bDisplayComment = bShowOnLoad;
	SetModifiedFlag();
	SetExceededFlag();		// // //
}

CString CFamiTrackerDoc::GetComment() const
{
	return m_strComment;
}

bool CFamiTrackerDoc::ShowCommentOnOpen() const
{
	return m_bDisplayComment;
}

void CFamiTrackerDoc::SetSpeedSplitPoint(int SplitPoint)
{
	m_iSpeedSplitPoint = SplitPoint;
}

int CFamiTrackerDoc::GetSpeedSplitPoint() const
{
	return m_iSpeedSplitPoint;
}

void CFamiTrackerDoc::SetHighlight(unsigned int Track, const stHighlight Hl)		// // //
{
	CPatternData *pTrack = GetTrack(Track);
	pTrack->SetHighlight(Hl);
}

stHighlight CFamiTrackerDoc::GetHighlight(unsigned int Track) const		// // //
{
	CPatternData *pTrack = GetTrack(Track);
	return pTrack->GetRowHighlight();
}

void CFamiTrackerDoc::SetHighlight(stHighlight Hl)		// // //
{
	// TODO remove
	m_vHighlight = Hl;
}

stHighlight CFamiTrackerDoc::GetHighlight() const		// // //
{
	return m_vHighlight;
}

unsigned int CFamiTrackerDoc::GetHighlightAtRow(unsigned int Track, unsigned int Frame, unsigned int Row) const		// // //
{
	while (Frame < 0) Frame += GetFrameCount(Track);
	Frame %= GetFrameCount(Track);

	stHighlight Hl = m_vHighlight;
	stHighlight *New = NULL;
	int RowOffs = 0;
	bool Hit = false;
	
	if (m_pBookmarkList[Track]) {
		const int PackedPos = Frame * MAX_PATTERN_LENGTH + Row;
		int Min = MAX_FRAMES * MAX_PATTERN_LENGTH;
		for (auto it = m_pBookmarkList[Track]->begin(); it < m_pBookmarkList[Track]->end(); it++) {
			int NewPos = PackedPos - (it->Frame * MAX_PATTERN_LENGTH + it->Row);
			if (NewPos == 0)
				Hit = true;
			if (NewPos >= 0 && NewPos < Min) {
				Min = NewPos;
				New = &it->Highlight;

				RowOffs = it->Row;
				if (New->First != -1 && (it->Persist || it->Frame == Frame))
					Hl.First = New->First;
				if (New->Second != -1 && (it->Persist || it->Frame == Frame))
					Hl.Second = New->Second;
				Hl.Offset = New->Offset;
			}
		}
	}

	if (Hl.Second > 0 && !((Row - Hl.Offset - RowOffs) % Hl.Second))
		return 2 | (Hit << 7);
	if (Hl.First > 0 && !((Row - Hl.Offset - RowOffs) % Hl.First))
		return 1 | (Hit << 7);

	return Hit << 7;
}

unsigned int CFamiTrackerDoc::ScanActualLength(unsigned int Track, unsigned int Count) const		// // //
{
	// Return number for frames played for a certain number of loops

	char RowVisited[MAX_FRAMES][MAX_PATTERN_LENGTH];		// // //
	int JumpTo = -1;
	int SkipTo = -1;
	int FirstLoop = 0;
	int SecondLoop = 0;
	unsigned int f = 0;		// // //
	unsigned int r = 0;		// // //
	bool bScanning = true;
	unsigned int FrameCount = GetFrameCount(Track);
	int RowCount = 0;
	// // //

	memset(RowVisited, 0, MAX_FRAMES * MAX_PATTERN_LENGTH);		// // //

	while (bScanning) {
		bool hasJump = false;
		for (int j = 0; j < GetChannelCount(); ++j) {
			stChanNote *Note;
			Note = m_pTracks[Track]->GetPatternData(j, m_pTracks[Track]->GetFramePattern(f, j), r);
			GetNoteData(Track, f, j, r, Note);
			for (unsigned l = 0; l < GetEffColumns(Track, j) + 1; ++l) {
				switch (Note->EffNumber[l]) {
					case EF_JUMP:
						JumpTo = Note->EffParam[l];
						SkipTo = 0;
						hasJump = true;
						break;
					case EF_SKIP:
						if (hasJump) break;
						JumpTo = (f + 1) % FrameCount;
						SkipTo = Note->EffParam[l];
						break;
					case EF_HALT:
						Count = 1;
						bScanning = false;
						break;
				}
			}
		}

		switch (RowVisited[f][r]) {
		case 0: ++FirstLoop; break;
		case 1: ++SecondLoop; break;
		case 2: bScanning = false; break;
		}
		
		++RowVisited[f][r++];

		if (JumpTo > -1) {
			f = std::min(static_cast<unsigned int>(JumpTo), FrameCount - 1);
			JumpTo = -1;
		}
		if (SkipTo > -1) {
			r = std::min(static_cast<unsigned int>(SkipTo), GetPatternLength(Track) - 1);
			SkipTo = -1;
		}
		if (r >= GetPatternLength(Track)) {		// // //
			++f;
			r = 0;
		}
		if (f >= FrameCount)
			f = 0;
	}

	return FirstLoop + SecondLoop * (Count - 1);		// // //
}

double CFamiTrackerDoc::GetStandardLength(int Track, unsigned int ExtraLoops) const		// // //
{
	char RowVisited[MAX_FRAMES][MAX_PATTERN_LENGTH];
	int JumpTo = -1;
	int SkipTo = -1;
	double FirstLoop = 0.0;
	double SecondLoop = 0.0;
	bool IsGroove = GetSongGroove(Track);
	double Tempo = GetSongTempo(Track);
	double Speed = GetSongSpeed(Track);
	if (!GetSongTempo(Track))
		Tempo = 2.5 * GetFrameRate();
	int GrooveIndex = GetSongSpeed(Track) * (m_pGrooveTable[GetSongSpeed(Track)] != NULL), GroovePointer = 0;
	bool bScanning = true;
	unsigned int FrameCount = GetFrameCount(Track);

	if (IsGroove && GetGroove(GetSongSpeed(Track)) == NULL) {
		IsGroove = false;
		Speed = DEFAULT_SPEED;
	}

	memset(RowVisited, 0, MAX_FRAMES * MAX_PATTERN_LENGTH);

	unsigned int f = 0;
	unsigned int r = 0;
	while (bScanning) {
		bool hasJump = false;
		for (int j = 0; j < GetChannelCount(); ++j) {
			stChanNote* Note;
			Note = m_pTracks[Track]->GetPatternData(j, m_pTracks[Track]->GetFramePattern(f, j), r);
			for (unsigned l = 0; l < GetEffColumns(Track, j) + 1; ++l) {
				switch (Note->EffNumber[l]) {
				case EF_JUMP:
					JumpTo = Note->EffParam[l];
					SkipTo = 0;
					hasJump = true;
					break;
				case EF_SKIP:
					if (hasJump) break;
					JumpTo = (f + 1) % FrameCount;
					SkipTo = Note->EffParam[l];
					break;
				case EF_HALT:
					ExtraLoops = 0;
					bScanning = false;
					break;
				case EF_SPEED:
					if (GetSongTempo(Track) && Note->EffParam[l] >= m_iSpeedSplitPoint)
						Tempo = Note->EffParam[l];
					else {
						IsGroove = false;
						Speed = Note->EffParam[l];
					}
					break;
				case EF_GROOVE:
					if (m_pGrooveTable[Note->EffParam[l]] == NULL) break;
					IsGroove = true;
					GrooveIndex = Note->EffParam[l];
					GroovePointer = 0;
					break;
				}
			}
		}
		if (IsGroove)
			Speed = m_pGrooveTable[GrooveIndex]->GetEntry(GroovePointer++);
		
		switch (RowVisited[f][r]) {
		case 0: FirstLoop += Speed / Tempo; break;
		case 1: SecondLoop += Speed / Tempo; break;
		case 2: bScanning = false; break;
		}
		
		++RowVisited[f][r++];

		if (JumpTo > -1) {
			f = std::min(static_cast<unsigned int>(JumpTo), FrameCount - 1);
			JumpTo = -1;
		}
		if (SkipTo > -1) {
			r = std::min(static_cast<unsigned int>(SkipTo), GetPatternLength(Track) - 1);
			SkipTo = -1;
		}
		if (r >= GetPatternLength(Track)) {		// // //
			++f;
			r = 0;
		}
		if (f >= FrameCount)
			f = 0;
	}

	return (2.5 * (FirstLoop + SecondLoop * ExtraLoops));
}

// Operations

void CFamiTrackerDoc::RemoveUnusedInstruments()
{
	for (int i = 0; i < MAX_INSTRUMENTS; ++i) {
		if (IsInstrumentUsed(i)) {
			bool Used = false;
			for (unsigned int j = 0; j < m_iTrackCount; ++j) {
				for (unsigned int Channel = 0; Channel < m_iChannelsAvailable; ++Channel) {
					for (unsigned int Frame = 0; Frame < m_pTracks[j]->GetFrameCount(); ++Frame) {
						unsigned int Pattern = m_pTracks[j]->GetFramePattern(Frame, Channel);
						for (unsigned int Row = 0; Row < m_pTracks[j]->GetPatternLength(); ++Row) {
							stChanNote *pNote = m_pTracks[j]->GetPatternData(Channel, Pattern, Row);
							if (pNote->Instrument == i)
								Used = true;
						}
					}
				}
			}
			if (!Used)
				RemoveInstrument(i);
		}
	}

	static const inst_type_t inst[] = {INST_2A03, INST_VRC6, INST_N163, INST_S5B};
	static const uint8 chip[] = {SNDCHIP_NONE, SNDCHIP_VRC6, SNDCHIP_N163, SNDCHIP_S5B};

	// Also remove unused sequences
	for (unsigned int i = 0; i < MAX_SEQUENCES; ++i) for (int j = 0; j < SEQ_COUNT; ++j) {
		for (size_t c = 0; c < sizeof(chip); c++) if (GetSequenceItemCount(inst[c], i, j) > 0) {		// // //
			bool Used = false;
			for (int k = 0; k < MAX_INSTRUMENTS; ++k) {
				if (IsInstrumentUsed(k) && GetInstrumentType(k) == inst[c]) {
					auto pInstrument = std::static_pointer_cast<CSeqInstrument>(GetInstrument(k));
					if (pInstrument->GetSeqIndex(j) == i && pInstrument->GetSeqEnable(j)) {		// // //
						Used = true; break;
					}
				}
			}
			if (!Used)
				GetSequence(inst[c], i, j)->Clear();
		}
	}
}

void CFamiTrackerDoc::RemoveUnusedPatterns()
{
	for (unsigned int i = 0; i < m_iTrackCount; ++i) {
		for (unsigned int c = 0; c < m_iChannelsAvailable; ++c) {
			for (unsigned int p = 0; p < MAX_PATTERN; ++p) {
				bool bRemove(true);
				// Check if pattern is used in frame list
				unsigned int FrameCount = m_pTracks[i]->GetFrameCount();
				for (unsigned int f = 0; f < FrameCount; ++f) {
					bRemove = (m_pTracks[i]->GetFramePattern(f, c) == p) ? false : bRemove;
				}
				if (bRemove)
					m_pTracks[i]->ClearPattern(c, p);
			}
		}
	}
	SetModifiedFlag();		// // //
	SetExceededFlag();
}

void CFamiTrackerDoc::RemoveUnusedSamples()		// // //
{
	bool AssignUsed[MAX_INSTRUMENTS][OCTAVE_RANGE][NOTE_RANGE];
	memset(AssignUsed, 0, MAX_INSTRUMENTS * OCTAVE_RANGE * NOTE_RANGE);

	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (IsSampleUsed(i)) {
			bool Used = false;
			for (unsigned int j = 0; j < m_iTrackCount; ++j) {
				for (unsigned int Frame = 0; Frame < m_pTracks[j]->GetFrameCount(); ++Frame) {
					unsigned int Pattern = m_pTracks[j]->GetFramePattern(Frame, CHANID_DPCM);
					for (unsigned int Row = 0; Row < m_pTracks[j]->GetPatternLength(); ++Row) {
						stChanNote *pNote = m_pTracks[j]->GetPatternData(CHANID_DPCM, Pattern, Row);
						int Index = pNote->Instrument;
						if (pNote->Note < NOTE_C || pNote->Note > NOTE_B || Index == MAX_INSTRUMENTS) continue;		// // //
						if (GetInstrumentType(Index) != INST_2A03) continue;
						AssignUsed[Index][pNote->Octave][pNote->Note - 1] = true;
						auto pInst = std::static_pointer_cast<CInstrument2A03>(GetInstrument(Index));
						if (pInst->GetSample(pNote->Octave, pNote->Note - 1) == i + 1)
							Used = true;
					}
				}
			}
			if (!Used)
				RemoveSample(i);
		}
	}
	// also remove unused assignments
	for (int i = 0; i < MAX_INSTRUMENTS; i++) if (IsInstrumentUsed(i))
		if (auto pInst = std::dynamic_pointer_cast<CInstrument2A03>(GetInstrument(i)))
			for (int o = 0; o < OCTAVE_RANGE; o++) for (int n = 0; n < NOTE_RANGE; n++)
				if (!AssignUsed[i][o][n])
					pInst->SetSample(o, n, 0);

	SetModifiedFlag();		// // //
	SetExceededFlag();
}

void CFamiTrackerDoc::MergeDuplicatedPatterns()
{
	for (unsigned int i = 0; i < m_iTrackCount; ++i)
	for (unsigned int c = 0; c < m_iChannelsAvailable; ++c)
	{
		TRACE2("Trim: %d, %d\n", i, c);

		unsigned int uiPatternUsed[MAX_PATTERN];

		// mark all as unused
		for (unsigned int ui=0; ui < MAX_PATTERN; ++ui)
		{
			uiPatternUsed[ui] = MAX_PATTERN;
		}

		// map used patterns to themselves
		for (unsigned int f=0; f < m_pTracks[i]->GetFrameCount(); ++f)
		{
			unsigned int uiPattern = m_pTracks[i]->GetFramePattern(f,c);
			uiPatternUsed[uiPattern] = uiPattern;
		}

		// remap duplicates
		for (unsigned int ui=0; ui < MAX_PATTERN; ++ui)
		{
			if (uiPatternUsed[ui] == MAX_PATTERN) continue;
			for (unsigned int uj=0; uj < ui; ++uj)
			{
				unsigned int uiLen = m_pTracks[i]->GetPatternLength();
				bool bSame = true;
				for (unsigned int uk = 0; uk < uiLen; ++uk)
				{
					stChanNote* a = m_pTracks[i]->GetPatternData(c, ui, uk);
					stChanNote* b = m_pTracks[i]->GetPatternData(c, uj, uk);
					if (0 != ::memcmp(a, b, sizeof(stChanNote)))
					{
						bSame = false;
						break;
					}
				}
				if (bSame)
				{
					uiPatternUsed[ui] = uj;
					TRACE2("Duplicate: %d = %d\n", ui, uj);
					break;
				}
			}
		}

		// apply mapping
		for (unsigned int f=0; f < m_pTracks[i]->GetFrameCount(); ++f)
		{
			unsigned int uiPattern = m_pTracks[i]->GetFramePattern(f,c);
			m_pTracks[i]->SetFramePattern(f,c,uiPatternUsed[uiPattern]);
		}
	}

	SetModifiedFlag();		// // //
	SetExceededFlag();
}

void CFamiTrackerDoc::PopulateUniquePatterns()		// // //
{
	for (unsigned int i = 0; i < m_iTrackCount; i++) {
		const int Rows = GetPatternLength(i);
		const int Frames = GetFrameCount(i);
		CPatternData *pTrack = m_pTracks[i];
		CPatternData *pNew = new CPatternData(Rows);

		pNew->SetSongSpeed(GetSongSpeed(i));
		pNew->SetSongTempo(GetSongTempo(i));
		pNew->SetFrameCount(Frames);
		pNew->SetSongGroove(GetSongGroove(i));

		for (int c = 0; c < GetChannelCount(); c++) {
			pNew->SetEffectColumnCount(c, GetEffColumns(i, c));
			for (int f = 0; f < Frames; f++) {
				pNew->SetFramePattern(f, c, f);
				for (int r = 0; r < Rows; r++)
					memcpy(pNew->GetPatternData(c, f, r),
					pTrack->GetPatternData(c, pTrack->GetFramePattern(f, c), r), sizeof(stChanNote));
			}
		}

		SAFE_RELEASE(pTrack);
		m_pTracks[i] = pNew;
	}

	SetModifiedFlag();
	SetExceededFlag();
}

void CFamiTrackerDoc::SwapInstruments(int First, int Second)
{
	// Swap instruments
	auto pFirst = m_pInstrumentManager->GetInstrument(First);
	auto pSecond = m_pInstrumentManager->GetInstrument(Second);
	m_pInstrumentManager->InsertInstrument(Second, pFirst);
	m_pInstrumentManager->InsertInstrument(First, pSecond);
	// m_pInstrumentManager->Swap(First, Second);
	
	// Scan patterns
	const unsigned int Count = m_iRegisteredChannels;
	for (unsigned int i = 0; i < m_iTrackCount; ++i) {
		CPatternData *pTrack = m_pTracks[i];
		for (int j = 0; j < MAX_PATTERN; ++j) {
			for (unsigned int k = 0; k < Count; ++k) {
				for (int l = 0; l < MAX_PATTERN_LENGTH; ++l) {
					stChanNote *pData = pTrack->GetPatternData(k, j, l);
					if (pData->Instrument == First)
						pData->Instrument = Second;
					else if (pData->Instrument == Second)
						pData->Instrument = First;
				}
			}
		}
	}
}

static void UpdateEchoTranspose(stChanNote *Note, int *Value, unsigned int EffColumns)
{
	for (int j = EffColumns; j >= 0; j--) {
		const int Param = Note->EffParam[j] & 0x0F;
		switch (Note->EffNumber[j]) {
		case EF_SLIDE_UP:
			*Value += Param; return;
		case EF_SLIDE_DOWN:
			*Value -= Param; return;
		case EF_TRANSPOSE: // Sometimes there are not enough ticks for the transpose to take place
			if (Note->EffParam[j] & 0x80)
				*Value -= Param;
			else
				*Value += Param;
			return;
		}
	}
}

stFullState CFamiTrackerDoc::RetrieveSoundState(unsigned int Track, unsigned int Frame, unsigned int Row, int Channel)
{
	stFullState S = {};
	S.State = new stChannelState[m_iRegisteredChannels]();
	S.Tempo = S.Speed = S.GroovePos = -1;

	for (int c = 0; c < m_iRegisteredChannels; c++) {
		stChannelState *State = &S.State[c];
		State->ChannelIndex = GetChannelType(c);
		memset(State->Effect, -1, EF_COUNT * sizeof(int));
		memset(State->Echo, -1, ECHO_BUFFER_LENGTH * sizeof(int));
		// State->Mute = CFamiTrackerView::GetView()->IsChannelMuted(i);
	}
	
	stChanNote Note;
	int totalRows = 0;
	int *BufferPos = new int[m_iRegisteredChannels];
	int (*Transpose)[ECHO_BUFFER_LENGTH + 1] = new int[m_iRegisteredChannels][ECHO_BUFFER_LENGTH + 1]();
	memset(BufferPos, -1, m_iRegisteredChannels * sizeof(int));
	bool maskFDS = false; // no need to create per-channel array since only one FDS channel exists
						  // may not be the case in future additions

	while (true) {
		for (int c = m_iRegisteredChannels - 1; c >= 0; c--) {
			// if (Channel != -1) c = GetChannelIndex(Channel);
			stChannelState *State = &S.State[c];
			int EffColumns = GetEffColumns(Track, c);
			GetNoteData(Track, Frame, c, Row, &Note);
		
			if (Note.Note != NONE && Note.Note != RELEASE) {
				for (int i = 0; i < std::min(BufferPos[c], ECHO_BUFFER_LENGTH + 1); i++) {
					if (State->Echo[i] == ECHO_BUFFER_ECHO) {
						UpdateEchoTranspose(&Note, &Transpose[c][i], EffColumns);
						switch (Note.Note) {
						case HALT: State->Echo[i] = ECHO_BUFFER_HALT; break;
						case ECHO: State->Echo[i] = ECHO_BUFFER_ECHO + Note.Octave; break;
						default:
							int NewNote = MIDI_NOTE(Note.Octave, Note.Note) + Transpose[c][i];
							NewNote = std::max(std::min(NewNote, NOTE_COUNT - 1), 0);
							State->Echo[i] = NewNote;
						}
					}
					else if (State->Echo[i] > ECHO_BUFFER_ECHO && State->Echo[i] <= ECHO_BUFFER_ECHO + ECHO_BUFFER_LENGTH)
						State->Echo[i]--;
				}
				if (BufferPos[c] >= 0 && BufferPos[c] <= ECHO_BUFFER_LENGTH) {
					// WriteEchoBuffer(&Note, BufferPos, EffColumns);
					int Value;
					switch (Note.Note) {
					case HALT: Value = ECHO_BUFFER_HALT; break;
					case ECHO: Value = ECHO_BUFFER_ECHO + Note.Octave; break;
					default:
						Value = MIDI_NOTE(Note.Octave, Note.Note);
						UpdateEchoTranspose(&Note, &Value, EffColumns);
						Value = std::max(std::min(Value, NOTE_COUNT - 1), 0);
					}
					State->Echo[BufferPos[c]] = Value;
					UpdateEchoTranspose(&Note, &Transpose[c][BufferPos[c]], EffColumns);
				}
				BufferPos[c]++;
			}
			if (BufferPos[c] < 0)
				BufferPos[c] = 0;

			if (State->Instrument == MAX_INSTRUMENTS)
				if (Note.Instrument != MAX_INSTRUMENTS)
					State->Instrument = Note.Instrument;

			if (State->Volume == MAX_VOLUME)
				if (Note.Vol != MAX_VOLUME)
					State->Volume = Note.Vol;
		
			CTrackerChannel *ch = GetChannel(c);
			ASSERT(ch != NULL);
			for (int k = EffColumns; k >= 0; k--) {
				unsigned char fx = Note.EffNumber[k], xy = Note.EffParam[k];
				switch (fx) {
				// ignore effects that cannot have memory
				case EF_NONE: case EF_PORTAOFF:
				case EF_DAC: case EF_DPCM_PITCH: case EF_RETRIGGER:
				case EF_DELAY: case EF_DELAYED_VOLUME: case EF_NOTE_RELEASE: case EF_TRANSPOSE:
				case EF_JUMP: case EF_SKIP: // no true backward iterator
					continue;
				case EF_HALT:
					Row = Frame = 0; goto outer;
				case EF_SPEED:
					if (S.Speed == -1 && (xy < GetSpeedSplitPoint() || GetSongTempo(Track) == 0)) {
						S.Speed = xy; if (S.Speed < 1) S.Speed = 1;
						S.GroovePos = -2;
					}
					else if (S.Tempo == -1 && xy >= GetSpeedSplitPoint()) S.Tempo = xy;
					continue;
				case EF_GROOVE:
					if (S.GroovePos == -1 && xy < MAX_GROOVE) {
						S.GroovePos = totalRows;
						S.Speed = xy;
					}
					continue;
				case EF_VOLUME:
					if (!ch->IsEffectCompatible(fx, xy)) continue;
					if (State->Effect_LengthCounter == -1 && xy >= 0xE0 && xy <= 0xE3)
						State->Effect_LengthCounter = xy;
					else if (State->Effect[fx] == -1 && xy <= 0x1F) {
						State->Effect[fx] = xy;
						if (State->Effect_LengthCounter == -1) State->Effect_LengthCounter = 0xE2;
					}
					continue;
				case EF_NOTE_CUT:
					if (!ch->IsEffectCompatible(fx, xy)) continue;
					if (xy <= 0x7F) continue;
					if (State->Effect[fx] == -1) {
						State->Effect[fx] = xy;
						if (State->Effect_LengthCounter == -1) State->Effect_LengthCounter = 0xE2;
					}
					continue;
				case EF_FDS_MOD_DEPTH:
					if (!ch->IsEffectCompatible(fx, xy)) continue;
					if (State->Effect_AutoFMMult == -1 && xy >= 0x80)
						State->Effect_AutoFMMult = xy;
					continue;
				case EF_FDS_MOD_SPEED_HI:
					if (!ch->IsEffectCompatible(fx, xy)) continue;
					if (xy <= 0x0F)
						maskFDS = true;
					else if (!maskFDS && State->Effect[fx] == -1) {
						State->Effect[fx] = xy;
						if (State->Effect_AutoFMMult == -1) State->Effect_AutoFMMult = -2;
					}
					continue;
				case EF_FDS_MOD_SPEED_LO:
					if (!ch->IsEffectCompatible(fx, xy)) continue;
					maskFDS = true;
					continue;
				case EF_SAMPLE_OFFSET:
				case EF_FDS_VOLUME: case EF_FDS_MOD_BIAS:
				case EF_SUNSOFT_ENV_LO: case EF_SUNSOFT_ENV_HI: case EF_SUNSOFT_ENV_TYPE:
				case EF_N163_WAVE_BUFFER:
					if (!ch->IsEffectCompatible(fx, xy)) continue;
				case EF_VIBRATO: case EF_TREMOLO: case EF_PITCH: case EF_DUTY_CYCLE: case EF_VOLUME_SLIDE:
					if (State->Effect[fx] == -1)
						State->Effect[fx] = xy;
					continue;

				case EF_SWEEPUP: case EF_SWEEPDOWN: case EF_SLIDE_UP: case EF_SLIDE_DOWN:
				case EF_PORTAMENTO: case EF_ARPEGGIO: case EF_PORTA_UP: case EF_PORTA_DOWN:
					if (State->Effect[EF_PORTAMENTO] == -1) { // anything else within can be used here
						State->Effect[EF_PORTAMENTO] = fx == EF_PORTAMENTO ? xy : -2;
						State->Effect[EF_ARPEGGIO] = fx == EF_ARPEGGIO ? xy : -2;
						State->Effect[EF_PORTA_UP] = fx == EF_PORTA_UP ? xy : -2;
						State->Effect[EF_PORTA_DOWN] = fx == EF_PORTA_DOWN ? xy : -2;
					}
					continue;
				}
			}
			// if (Channel != -1) break;
		}
	outer:
		if (Row) Row--;
		else if (Frame) Row = GetFrameLength(Track, --Frame) - 1;
		else break;
		totalRows++;
	}
	if (S.GroovePos == -1 && GetSongGroove(Track)) {
		S.GroovePos = totalRows;
		S.Speed = GetSongSpeed(Track);
	}
	
	SAFE_RELEASE_ARRAY(BufferPos);
	SAFE_RELEASE_ARRAY(Transpose);
	return S;
}

void CFamiTrackerDoc::SetDetuneOffset(int Chip, int Note, int Detune)		// // //
{
	m_iDetuneTable[Chip][Note] = Detune;
}

int CFamiTrackerDoc::GetDetuneOffset(int Chip, int Note) const		// // //
{
	return m_iDetuneTable[Chip][Note];
}

void CFamiTrackerDoc::ResetDetuneTables()		// // //
{
	for (int i = 0; i < 6; i++) for (int j = 0; j < NOTE_COUNT; j++)
		m_iDetuneTable[i][j] = 0;
}

CGroove* CFamiTrackerDoc::GetGroove(int Index) const		// // //
{
	return m_pGrooveTable[Index];
}

void CFamiTrackerDoc::SetGroove(int Index, const CGroove* Groove)
{
	if (Groove == NULL) {
		delete m_pGrooveTable[Index];
		m_pGrooveTable[Index] = NULL;
	}
	else m_pGrooveTable[Index] = new CGroove(*Groove);
}

std::vector<stBookmark> *const CFamiTrackerDoc::GetBookmarkList(unsigned int Track)		// // //
{
	if (m_pBookmarkList[Track] == NULL) {
		m_pBookmarkList[Track] = new std::vector<stBookmark>();
	}
	return new std::vector<stBookmark>(*m_pBookmarkList[Track]);
}

void CFamiTrackerDoc::SetBookmarkList(unsigned int Track, std::vector<stBookmark> *const List)
{
	SAFE_RELEASE(m_pBookmarkList[Track]);
	m_pBookmarkList[Track] = new std::vector<stBookmark>(*List);
}

void CFamiTrackerDoc::ClearBookmarkList(unsigned int Track)
{
	std::vector<stBookmark> *List = m_pBookmarkList[Track];
	if (List != NULL) {
		for (size_t i = 0; i < List->size(); i++)
			SAFE_RELEASE((*List)[i].Name);
		SAFE_RELEASE(m_pBookmarkList[Track]);
	}
}

void CFamiTrackerDoc::SetExceededFlag(bool Exceed)
{
	m_bExceeded = Exceed;
}

int CFamiTrackerDoc::GetFrameLength(unsigned int Track, unsigned int Frame) const
{
	// // // moved from PatternEditor.cpp
	const int PatternLength = GetPatternLength(Track);	// default length
	
	int HaltPoint = PatternLength;

	for (int j = 0; j < PatternLength; ++j) {
		for (int i = 0; i < GetChannelCount(); ++i) {
			int Columns = GetEffColumns(Track, i) + 1;
			stChanNote Note;
			GetNoteData(Track, Frame, i, j, &Note);
			// First look for pattern data, allow this to cancel earlier pattern lengths
			/*
			if (Note.Note != NONE || Note.Instrument != MAX_INSTRUMENTS || Note.Vol != 0x10)
				HaltPoint = PatternLength;
			for (int k = 0; k < Columns; ++k) {
				if (Note.EffNumber[k] != EF_NONE)
					HaltPoint = PatternLength;
			}
			*/
			// Then look for pattern break commands
			for (int k = 0; k < Columns; ++k) {
				switch (Note.EffNumber[k]) {
					case EF_SKIP:
					case EF_JUMP:
					case EF_HALT:
						HaltPoint = j + 1;
						return HaltPoint;
				}
			}
		}
	}

	return HaltPoint;
}

void CFamiTrackerDoc::MakeKraid()			// // // Easter Egg
{
	// Basic info
	for (int i = GetTrackCount() - 1; i > 0; i--) RemoveTrack(i);
	SetTrackTitle(0, _T("New song"));
	m_pTracks[0]->ClearEverything();
	SetEngineSpeed(0);
	SetMachine(NTSC);
	SetFrameCount(0, 14);
	SetPatternLength(0, 24);
	SetSongSpeed(0, 8);
	SetSongTempo(0, 150);
	SetSongGroove(0, false);
	SetEffColumns(0, 0, 1);
	SetEffColumns(0, 1, 0);
	SetEffColumns(0, 2, 0);
	SetEffColumns(0, 3, 0);
	SetEffColumns(0, 4, 0);
	SetSpeedSplitPoint(32);
	SelectExpansionChip(SNDCHIP_NONE);
	SetHighlight(DEFAULT_HIGHLIGHT);
	ResetDetuneTables();
	for (int i = 0; i < MAX_GROOVE; i++)
		SAFE_RELEASE(m_pGrooveTable[i]);
	for (int i = 0; i < MAX_TRACKS; ++i)
		ClearBookmarkList(i);

	// Patterns
	SetPatternAtFrame(0,  0, 0, 0); SetPatternAtFrame(0,  0, 1, 0); SetPatternAtFrame(0,  0, 2, 0);
	SetPatternAtFrame(0,  1, 0, 0); SetPatternAtFrame(0,  1, 1, 0); SetPatternAtFrame(0,  1, 2, 0);
	SetPatternAtFrame(0,  2, 0, 0); SetPatternAtFrame(0,  2, 1, 0); SetPatternAtFrame(0,  2, 2, 0);
	SetPatternAtFrame(0,  3, 0, 0); SetPatternAtFrame(0,  3, 1, 0); SetPatternAtFrame(0,  3, 2, 0);
	SetPatternAtFrame(0,  4, 0, 1); SetPatternAtFrame(0,  4, 1, 1); SetPatternAtFrame(0,  4, 2, 1);
	SetPatternAtFrame(0,  5, 0, 1); SetPatternAtFrame(0,  5, 1, 1); SetPatternAtFrame(0,  5, 2, 1);
	SetPatternAtFrame(0,  6, 0, 2); SetPatternAtFrame(0,  6, 1, 2); SetPatternAtFrame(0,  6, 2, 2);
	SetPatternAtFrame(0,  7, 0, 3); SetPatternAtFrame(0,  7, 1, 2); SetPatternAtFrame(0,  7, 2, 2);
	SetPatternAtFrame(0,  8, 0, 3); SetPatternAtFrame(0,  8, 1, 2); SetPatternAtFrame(0,  8, 2, 2);
	SetPatternAtFrame(0,  9, 0, 3); SetPatternAtFrame(0,  9, 1, 2); SetPatternAtFrame(0,  9, 2, 2);
	SetPatternAtFrame(0, 10, 0, 4); SetPatternAtFrame(0, 10, 1, 3); SetPatternAtFrame(0, 10, 2, 3);
	SetPatternAtFrame(0, 11, 0, 5); SetPatternAtFrame(0, 11, 1, 3); SetPatternAtFrame(0, 11, 2, 3);
	SetPatternAtFrame(0, 12, 0, 6); SetPatternAtFrame(0, 12, 1, 4); SetPatternAtFrame(0, 12, 2, 4);
	SetPatternAtFrame(0, 13, 0, 6); SetPatternAtFrame(0, 13, 1, 4); SetPatternAtFrame(0, 13, 2, 4);
	for (int i = 0; i < 14; i++){
		SetPatternAtFrame(0, i, 3, 0);
		SetPatternAtFrame(0, i, 4, 0);
	}

	// Instruments
	for (int i = 0; i < MAX_INSTRUMENTS; i++) RemoveInstrument(i);
	auto kraidInst = std::static_pointer_cast<CInstrument2A03>(CInstrumentManager::CreateNew(INST_2A03));
	kraidInst->SetSeqEnable(SEQ_VOLUME, 1);
	kraidInst->SetSeqIndex(SEQ_VOLUME, 0);
	kraidInst->SetName("Lead ");
	m_pInstrumentManager->InsertInstrument(0, kraidInst);
	kraidInst = std::static_pointer_cast<CInstrument2A03>(CInstrumentManager::CreateNew(INST_2A03));
	kraidInst->SetSeqEnable(SEQ_VOLUME, 1);
	kraidInst->SetSeqIndex(SEQ_VOLUME, 1);
	kraidInst->SetName("Echo");
	m_pInstrumentManager->InsertInstrument(1, kraidInst);
	kraidInst = std::static_pointer_cast<CInstrument2A03>(CInstrumentManager::CreateNew(INST_2A03));
	kraidInst->SetSeqEnable(SEQ_VOLUME, 1);
	kraidInst->SetSeqIndex(SEQ_VOLUME, 2);
	kraidInst->SetName("Triangle");
	m_pInstrumentManager->InsertInstrument(2, kraidInst);

	// Sequences
	CSequence *kraidSeq;
	kraidSeq = GetSequence(INST_2A03, 0, SEQ_VOLUME);
	kraidSeq->SetItemCount(1);
	kraidSeq->SetItem(0, 6);
	kraidSeq->SetLoopPoint(-1);
	kraidSeq->SetReleasePoint(-1);
	kraidSeq = GetSequence(INST_2A03, 1, SEQ_VOLUME);
	kraidSeq->SetItemCount(1);
	kraidSeq->SetItem(0, 2);
	kraidSeq->SetLoopPoint(-1);
	kraidSeq->SetReleasePoint(-1);
	kraidSeq = GetSequence(INST_2A03, 2, SEQ_VOLUME);
	kraidSeq->SetItemCount(1);
	kraidSeq->SetItem(0, 15);
	kraidSeq->SetLoopPoint(-1);
	kraidSeq->SetReleasePoint(-1);

	// Triangle
	stChanNote* kraidRow = new stChanNote;
	*kraidRow = BLANK_NOTE;
	kraidRow->Instrument = 2;
	for (int i = 0; i < 24; i += 6) {
		kraidRow->Note = NOTE_E; kraidRow->Octave = 2; SetDataAtPattern(0, 0, 2, i    , kraidRow);
								 kraidRow->Octave = 3; SetDataAtPattern(0, 0, 2, i + 2, kraidRow);}
	for (int i = 0; i < 12; i += 6) {
		kraidRow->Note = NOTE_C; kraidRow->Octave = 2; SetDataAtPattern(0, 1, 2, i     , kraidRow);
								 kraidRow->Octave = 3; SetDataAtPattern(0, 1, 2, i +  2, kraidRow);
		kraidRow->Note = NOTE_D; kraidRow->Octave = 2; SetDataAtPattern(0, 1, 2, i + 12, kraidRow);
								 kraidRow->Octave = 3; SetDataAtPattern(0, 1, 2, i + 14, kraidRow);}
	for (int i = 0; i < 6; i += 2) {
		kraidRow->Note = NOTE_E;  kraidRow->Octave = 2 + i / 2; SetDataAtPattern(0, 2, 2, i     , kraidRow);
		kraidRow->Note = NOTE_Fs;                               SetDataAtPattern(0, 2, 2, i +  6, kraidRow);
		kraidRow->Note = NOTE_F;                                SetDataAtPattern(0, 2, 2, i + 12, kraidRow);
		kraidRow->Note = NOTE_B;  kraidRow->Octave = 1 + i / 2; SetDataAtPattern(0, 2, 2, i + 18, kraidRow);}
	kraidRow->Note = NOTE_E; kraidRow->Octave = 1;	SetDataAtPattern(0, 4, 2,  0, kraidRow);
	kraidRow->Note = NOTE_C; kraidRow->Octave = 2;	SetDataAtPattern(0, 3, 2, 12, kraidRow);
	kraidRow->Note = NOTE_E;						SetDataAtPattern(0, 3, 2,  0, kraidRow);
	kraidRow->Note = NOTE_G;						SetDataAtPattern(0, 3, 2, 16, kraidRow);
	kraidRow->Note = NOTE_A;						SetDataAtPattern(0, 3, 2, 18, kraidRow);
	kraidRow->Note = NOTE_B;						SetDataAtPattern(0, 3, 2,  4, kraidRow);
													SetDataAtPattern(0, 3, 2, 10, kraidRow);
													SetDataAtPattern(0, 3, 2, 22, kraidRow);
	kraidRow->Note = NOTE_C; kraidRow->Octave = 3;	SetDataAtPattern(0, 3, 2,  6, kraidRow);

	// Pulse 2
	kraidRow->Instrument = 0;
	kraidRow->Note = NOTE_As; kraidRow->Octave = 2;	SetDataAtPattern(0, 0, 1, 22, kraidRow);
	kraidRow->Note = NOTE_B;						SetDataAtPattern(0, 0, 1,  4, kraidRow);
													SetDataAtPattern(0, 0, 1, 18, kraidRow);
													SetDataAtPattern(0, 1, 1, 10, kraidRow);
													SetDataAtPattern(0, 2, 1,  1, kraidRow);
													SetDataAtPattern(0, 2, 1,  3, kraidRow);
													SetDataAtPattern(0, 2, 1,  5, kraidRow);
	kraidRow->Note = NOTE_C;  kraidRow->Octave = 3;	SetDataAtPattern(0, 0, 1, 10, kraidRow);
													SetDataAtPattern(0, 2, 1, 13, kraidRow);
													SetDataAtPattern(0, 2, 1, 15, kraidRow);
													SetDataAtPattern(0, 2, 1, 17, kraidRow);
	kraidRow->Note = NOTE_D; 						SetDataAtPattern(0, 0, 1, 16, kraidRow);
													SetDataAtPattern(0, 1, 1,  4, kraidRow);
													SetDataAtPattern(0, 1, 1, 16, kraidRow);
	kraidRow->Note = NOTE_Ds;						SetDataAtPattern(0, 2, 1, 19, kraidRow);
													SetDataAtPattern(0, 2, 1, 21, kraidRow);
													SetDataAtPattern(0, 2, 1, 23, kraidRow);
	kraidRow->Note = NOTE_E; 						SetDataAtPattern(0, 0, 1,  0, kraidRow);
													SetDataAtPattern(0, 1, 1,  6, kraidRow);
													SetDataAtPattern(0, 1, 1, 22, kraidRow);
													SetDataAtPattern(0, 2, 1,  7, kraidRow);
													SetDataAtPattern(0, 2, 1,  9, kraidRow);
													SetDataAtPattern(0, 2, 1, 11, kraidRow);
													SetDataAtPattern(0, 3, 1, 18, kraidRow);
	kraidRow->Note = NOTE_Fs;						SetDataAtPattern(0, 0, 1, 12, kraidRow);
													SetDataAtPattern(0, 1, 1, 12, kraidRow);
													SetDataAtPattern(0, 2, 1, 20, kraidRow);
													SetDataAtPattern(0, 3, 1,  0, kraidRow);
													SetDataAtPattern(0, 3, 1, 10, kraidRow);
													SetDataAtPattern(0, 3, 1, 17, kraidRow);
	kraidRow->Note = NOTE_G; 						SetDataAtPattern(0, 1, 1,  0, kraidRow);
													SetDataAtPattern(0, 2, 1,  0, kraidRow);
													SetDataAtPattern(0, 2, 1,  2, kraidRow);
													SetDataAtPattern(0, 2, 1,  4, kraidRow);
													SetDataAtPattern(0, 3, 1,  1, kraidRow);
													SetDataAtPattern(0, 3, 1,  9, kraidRow);
													SetDataAtPattern(0, 3, 1, 16, kraidRow);
													SetDataAtPattern(0, 3, 1, 19, kraidRow);
	kraidRow->Note = NOTE_A; 						SetDataAtPattern(0, 0, 1,  6, kraidRow);
													SetDataAtPattern(0, 1, 1, 18, kraidRow);
													SetDataAtPattern(0, 2, 1, 12, kraidRow);
													SetDataAtPattern(0, 2, 1, 14, kraidRow);
													SetDataAtPattern(0, 2, 1, 16, kraidRow);
													SetDataAtPattern(0, 2, 1, 18, kraidRow);
													SetDataAtPattern(0, 3, 1,  2, kraidRow);
													SetDataAtPattern(0, 3, 1,  8, kraidRow);
													SetDataAtPattern(0, 3, 1, 15, kraidRow);
													SetDataAtPattern(0, 3, 1, 20, kraidRow);
	kraidRow->Note = NOTE_As;						SetDataAtPattern(0, 2, 1,  6, kraidRow);
													SetDataAtPattern(0, 2, 1,  8, kraidRow);
													SetDataAtPattern(0, 2, 1, 10, kraidRow);
	kraidRow->Note = NOTE_B; 						SetDataAtPattern(0, 2, 1, 22, kraidRow);
													SetDataAtPattern(0, 3, 1,  3, kraidRow);
													SetDataAtPattern(0, 3, 1,  5, kraidRow);
													SetDataAtPattern(0, 3, 1, 11, kraidRow);
	kraidRow->Note = NOTE_D;  kraidRow->Octave = 4;	SetDataAtPattern(0, 3, 1,  4, kraidRow);
													SetDataAtPattern(0, 3, 1,  7, kraidRow);
													SetDataAtPattern(0, 3, 1, 14, kraidRow);
													SetDataAtPattern(0, 3, 1, 21, kraidRow);
	kraidRow->Note = NOTE_E;						SetDataAtPattern(0, 3, 1, 22, kraidRow);
													SetDataAtPattern(0, 4, 1,  6, kraidRow);
													SetDataAtPattern(0, 4, 1, 18, kraidRow);
	kraidRow->Note = NOTE_Fs;						SetDataAtPattern(0, 3, 1,  6, kraidRow);
													SetDataAtPattern(0, 3, 1, 13, kraidRow);
													SetDataAtPattern(0, 4, 1,  4, kraidRow);
													SetDataAtPattern(0, 4, 1,  8, kraidRow);
													SetDataAtPattern(0, 4, 1, 16, kraidRow);
													SetDataAtPattern(0, 4, 1, 20, kraidRow);
	kraidRow->Note = NOTE_G;						SetDataAtPattern(0, 4, 1,  2, kraidRow);
													SetDataAtPattern(0, 4, 1, 10, kraidRow);
													SetDataAtPattern(0, 4, 1, 14, kraidRow);
													SetDataAtPattern(0, 4, 1, 22, kraidRow);
	kraidRow->Note = NOTE_A;						SetDataAtPattern(0, 3, 1, 12, kraidRow);
													SetDataAtPattern(0, 4, 1,  0, kraidRow);
													SetDataAtPattern(0, 4, 1, 12, kraidRow);
	kraidRow->Note = HALT; kraidRow->Octave = 0; kraidRow->Instrument = MAX_INSTRUMENTS;
	SetDataAtPattern(0, 3, 1, 23, kraidRow);
	SetDataAtPattern(0, 4, 1,  1, kraidRow);
	SetDataAtPattern(0, 4, 1,  3, kraidRow);
	SetDataAtPattern(0, 4, 1,  5, kraidRow);
	SetDataAtPattern(0, 4, 1,  7, kraidRow);
	SetDataAtPattern(0, 4, 1,  9, kraidRow);
	SetDataAtPattern(0, 4, 1, 11, kraidRow);
	SetDataAtPattern(0, 4, 1, 13, kraidRow);
	SetDataAtPattern(0, 4, 1, 15, kraidRow);
	SetDataAtPattern(0, 4, 1, 17, kraidRow);
	SetDataAtPattern(0, 4, 1, 19, kraidRow);
	SetDataAtPattern(0, 4, 1, 21, kraidRow);
	SetDataAtPattern(0, 4, 1, 23, kraidRow);
	GetDataAtPattern(0, 0, 1, 0, kraidRow); kraidRow->EffNumber[0] = EF_DUTY_CYCLE; kraidRow->EffParam[0] = 2; SetDataAtPattern(0, 0, 1, 0, kraidRow);
	GetDataAtPattern(0, 1, 1, 0, kraidRow); kraidRow->EffNumber[0] = EF_DUTY_CYCLE; kraidRow->EffParam[0] = 2; SetDataAtPattern(0, 1, 1, 0, kraidRow);
	GetDataAtPattern(0, 2, 1, 0, kraidRow); kraidRow->EffNumber[0] = EF_DUTY_CYCLE; kraidRow->EffParam[0] = 2; SetDataAtPattern(0, 2, 1, 0, kraidRow);

	// Pulse 1
	for (int i = 0; i < 23; i++){
		GetDataAtPattern(0, 0, 1, i, kraidRow);
		if (kraidRow->Note != NONE) {
			kraidRow->Instrument = 1; kraidRow->EffNumber[1] = EF_DELAY; kraidRow->EffParam[1] = 3;
			SetDataAtPattern(0, 0, 0, i + 1, kraidRow);}
		GetDataAtPattern(0, 1, 1, i, kraidRow);
		if (kraidRow->Note != NONE) {
			kraidRow->Instrument = 1; kraidRow->EffNumber[1] = EF_DELAY; kraidRow->EffParam[1] = 3;
			SetDataAtPattern(0, 1, 0, i + 1, kraidRow);}
		GetDataAtPattern(0, 2, 1, i, kraidRow);
		if (kraidRow->Note != NONE) {
			kraidRow->Instrument = 1; kraidRow->EffNumber[1] = EF_DELAY; kraidRow->EffParam[1] = 3;
			SetDataAtPattern(0, 2, 0, i + 1, kraidRow);
			SetDataAtPattern(0, 3, 0, i + 1, kraidRow);}
		GetDataAtPattern(0, 3, 1, i, kraidRow);
		if (kraidRow->Note != NONE) {
			kraidRow->Instrument = 1; kraidRow->EffNumber[1] = EF_DELAY; kraidRow->EffParam[1] = 3;
			SetDataAtPattern(0, 4, 0, i + 1, kraidRow);
			SetDataAtPattern(0, 5, 0, i + 1, kraidRow);}
		GetDataAtPattern(0, 4, 1, i, kraidRow);
		if (kraidRow->Note != NONE) {
			kraidRow->Instrument = 1; kraidRow->EffNumber[1] = EF_DELAY; kraidRow->EffParam[1] = 3;
			SetDataAtPattern(0, 6, 0, i + 1, kraidRow);}
	}
	kraidRow->Note = HALT; kraidRow->Octave = 0; kraidRow->Instrument = MAX_INSTRUMENTS;
	SetDataAtPattern(0, 0, 0, 0, kraidRow);
	SetDataAtPattern(0, 5, 0, 0, kraidRow);
	SetDataAtPattern(0, 6, 0, 0, kraidRow);
	kraidRow->Note = NOTE_Ds; kraidRow->Octave = 3; kraidRow->Instrument = 1;
	SetDataAtPattern(0, 3, 0, 0, kraidRow);
	SetDataAtPattern(0, 4, 0, 0, kraidRow);

	// Done
	delete kraidRow, kraidSeq;
}
