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

#pragma once


// Synchronization objects
#include <afxmt.h>
#include <vector>

// Get access to some APU constants
#include "APU/Types.h"
// Constants, types and enums
#include "FamiTrackerTypes.h"

#define TRANSPOSE_FDS

// #define DISABLE_SAVE		// // //

// Default song settings
const unsigned int DEFAULT_TEMPO_NTSC		 = 150;
const unsigned int DEFAULT_TEMPO_PAL		 = 125;
const unsigned int DEFAULT_SPEED			 = 6;
const unsigned int DEFAULT_MACHINE_TYPE		 = NTSC;
const unsigned int DEFAULT_SPEED_SPLIT_POINT = 32;
const unsigned int OLD_SPEED_SPLIT_POINT	 = 21;

// Cursor columns
enum cursor_column_t {
	C_NOTE, 
	C_INSTRUMENT1, 
	C_INSTRUMENT2, 
	C_VOLUME, 
	C_EFF_NUM, 
	C_EFF_PARAM1, 
	C_EFF_PARAM2,
	C_EFF2_NUM, 
	C_EFF2_PARAM1, 
	C_EFF2_PARAM2, 
	C_EFF3_NUM, 
	C_EFF3_PARAM1, 
	C_EFF3_PARAM2, 
	C_EFF4_NUM, 
	C_EFF4_PARAM1, 
	C_EFF4_PARAM2
};

const unsigned int COLUMNS = 7;

// Special assert used when loading files
#ifdef _DEBUG
	#define ASSERT_FILE_DATA(Statement) ASSERT(Statement)
#else
	#define ASSERT_FILE_DATA(Statement) if (!(Statement)) return true
#endif

// View update modes (TODO check these and remove inappropriate flags)
enum {
	UPDATE_NONE = 0,		// No update
	UPDATE_TRACK = 1,		// Track has been added, removed or changed
	UPDATE_PATTERN,			// Pattern data has been edited
	UPDATE_FRAME,			// Frame data has been edited
	UPDATE_INSTRUMENT,		// Instrument has been added / removed
	UPDATE_PROPERTIES,		// Module properties has changed (including channel count)
	UPDATE_HIGHLIGHT,		// Row highlight option has changed
	UPDATE_COLUMNS,			// Effect columns has changed
	UPDATE_CLOSE			// Document is closing (TODO remove)
};

// Old sequence list, kept for compability
struct stSequence {
	unsigned int Count;
	signed char Length[MAX_SEQUENCE_ITEMS];
	signed char Value[MAX_SEQUENCE_ITEMS];
};

// Access data types used by the document class
#include "PatternData.h"
#include "Instrument.h"
#include "Sequence.h"
#include "Groove.h"		// // //

// External classes
class CTrackerChannel;
class CDocumentFile;

//
// I'll try to organize this class, things are quite messy right now!
//

class CFamiTrackerDoc : public CDocument
{
protected: // create from serialization only
	CFamiTrackerDoc();
	DECLARE_DYNCREATE(CFamiTrackerDoc)

	// Static functions
public:
	static CFamiTrackerDoc* GetDoc();


	// Other
#ifdef AUTOSAVE
	void AutoSave();
#endif

	//
	// Public functions
	//
public:

	CString GetFileTitle() const;

	//
	// Document file I/O
	//
	bool IsFileLoaded() const;
	bool HasLastLoadFailed() const;

	// Import
	CFamiTrackerDoc* LoadImportFile(LPCTSTR lpszPathName) const;
	bool ImportInstruments(CFamiTrackerDoc *pImported, int *pInstTable);
	bool ImportGrooves(CFamiTrackerDoc *pImported, int *pGrooveMap);		// // //
	bool ImportDetune(CFamiTrackerDoc *pImported);			// // //
	bool ImportTrack(int Track, CFamiTrackerDoc *pImported, int *pInstTable, int *pGrooveMap);		// // //

	//
	// Interface functions (not related to document data) TODO move this?
	//
	void			ResetChannels();
	void			RegisterChannel(CTrackerChannel *pChannel, int ChannelType, int ChipType);
	CTrackerChannel* GetChannel(int Index) const;
	int				GetChannelIndex(int Channel) const;

	int				GetChannelType(int Channel) const;
	int				GetChipType(int Channel) const;
	int				GetChannelCount() const;
	int				GetChannelPosition(int Channel, unsigned char Chip);		// // //

	// Synchronization
	BOOL			LockDocument() const;
	BOOL			LockDocument(DWORD dwTimeout) const;
	BOOL			UnlockDocument() const;

	//
	// Document data access functions
	//

	// Local (song) data
	void			SetPatternLength(unsigned int Track, unsigned int Length);
	void			SetFrameCount(unsigned int Track, unsigned int Count);
	void			SetSongSpeed(unsigned int Track, unsigned int Speed);
	void			SetSongTempo(unsigned int Track, unsigned int Tempo);
	void			SetSongGroove(unsigned int Track, bool Groove);		// // //

	unsigned int	GetPatternLength(unsigned int Track) const;
	unsigned int	GetFrameCount(unsigned int Track) const;
	unsigned int	GetSongSpeed(unsigned int Track) const;
	unsigned int	GetSongTempo(unsigned int Track) const;
	bool			GetSongGroove(unsigned int Track) const;		// // //

	unsigned int	GetEffColumns(unsigned int Track, unsigned int Channel) const;
	void			SetEffColumns(unsigned int Track, unsigned int Channel, unsigned int Columns);

	unsigned int 	GetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel) const;
	void			SetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Pattern);

	bool			IsPatternEmpty(unsigned int Track, unsigned int Channel, unsigned int Pattern) const;

	void			MakeKraid();				// // // Easter Egg

	// Pattern editing
	void			SetNoteData(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, const stChanNote *pData);
	void			GetNoteData(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *pData) const;

	void			SetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, const stChanNote *pData);
	void			GetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *pData) const;

	void			ClearPatterns(unsigned int Track);
	void			ClearPattern(unsigned int Track, unsigned int Frame, unsigned int Channel);

	bool			InsertRow(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			DeleteNote(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Column);
	bool			ClearRow(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			ClearRowField(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, unsigned int Column);
	bool			RemoveNote(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			PullUp(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			CopyPattern(unsigned int Track, int Target, int Source, int Channel);

	void			SwapChannels(unsigned int Track, unsigned int First, unsigned int Second);		// // //

	// Frame editing
	bool			InsertFrame(unsigned int Track, unsigned int Frame);
	bool			RemoveFrame(unsigned int Track, unsigned int Frame);
	bool			DuplicateFrame(unsigned int Track, unsigned int Frame);
	bool			DuplicatePatterns(unsigned int Track, unsigned int Frame);
	bool			MoveFrameDown(unsigned int Track, unsigned int Frame);
	bool			MoveFrameUp(unsigned int Track, unsigned int Frame);
	void			DeleteFrames(unsigned int Track, unsigned int Frame, int Count);

	// Global (module) data
	void			SetEngineSpeed(unsigned int Speed);
	void			SetMachine(unsigned int Machine);
	unsigned int	GetMachine() const		{ return m_iMachine; };
	unsigned int	GetEngineSpeed() const	{ return m_iEngineSpeed; };
	unsigned int	GetFrameRate() const;

	void			SelectExpansionChip(unsigned char Chip, bool Move = false);		// // //
	unsigned char	GetExpansionChip() const { return m_iExpansionChip; };
	bool			ExpansionEnabled(int Chip) const;
	int				GetNamcoChannels() const;
	void			SetNamcoChannels(int Channels, bool Move = false);		// // //

	// Todo: remove this, use getchannelcount instead
	unsigned int	GetAvailableChannels()	const { return m_iChannelsAvailable; };

	// Todo: Replace with CString
	const char*		GetSongName() const;
	const char*		GetSongArtist() const;
	const char*		GetSongCopyright() const;
	void			SetSongName(const char *pName);
	void			SetSongArtist(const char *pArtist);
	void			SetSongCopyright(const char *pCopyright);

	vibrato_t		GetVibratoStyle() const;
	void			SetVibratoStyle(vibrato_t Style);

	bool			GetLinearPitch() const;
	void			SetLinearPitch(bool Enable);

	void			SetComment(CString &comment, bool bShowOnLoad);
	CString			GetComment() const;
	bool			ShowCommentOnOpen() const;

	void			SetSpeedSplitPoint(int SplitPoint);
	int				GetSpeedSplitPoint() const;

	void			SetHighlight(unsigned int Track, const stHighlight Hl);		// // //
	stHighlight		GetHighlight(unsigned int Track) const;

	void			SetHighlight(const stHighlight Hl);		// // //
	stHighlight		GetHighlight() const;
	unsigned int	GetHighlightAtRow(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //

	void			SetDetuneOffset(int Chip, int Note, int Detune);		// // //
	int				GetDetuneOffset(int Chip, int Note) const;
	void			ResetDetuneTables();

	CGroove			*GetGroove(int Index) const;		// // //
	void			SetGroove(int Index, const CGroove* Groove);

	std::vector<stBookmark> *const GetBookmarkList(unsigned int Track);		// // //
	void			SetBookmarkList(unsigned int Track, std::vector<stBookmark> *const List);
	void			ClearBookmarkList(unsigned int Track);

	int				GetFrameLength(unsigned int Track, unsigned int Frame) const;

	// Track management functions
	int				AddTrack();
	void			RemoveTrack(unsigned int Track);
	unsigned int	GetTrackCount() const;
	CString			GetTrackTitle(unsigned int Track) const;
	void			SetTrackTitle(unsigned int Track, const CString &title);
	void			MoveTrackUp(unsigned int Track);
	void			MoveTrackDown(unsigned int Track);

	// Instruments functions
	CInstrument*	GetInstrument(unsigned int Index) const;
	unsigned int	GetInstrumentCount() const;
	bool			IsInstrumentUsed(unsigned int Index) const;
	int				AddInstrument(const char *pName, int ChipType);					// Add a new instrument
	int				AddInstrument(CInstrument *pInstrument);
	void			AddInstrument(CInstrument *pInstrument, unsigned int Slot);
	void			RemoveInstrument(unsigned int Index);							// Remove an instrument
	void			SetInstrumentName(unsigned int Index, const char *pName);		// Set the name of an instrument
	void			GetInstrumentName(unsigned int Index, char *pName) const;		// Get the name of an instrument
	int				CloneInstrument(unsigned int Index);							// Create a copy of an instrument
	CInstrument*	CreateInstrument(inst_type_t InstType) const;					// Creates a new instrument of InstType
	int				FindFreeInstrumentSlot() const;
	inst_type_t		GetInstrumentType(unsigned int Index) const;
	int				DeepCloneInstrument(unsigned int Index);
	void			SaveInstrument(unsigned int Index, CString FileName) const;
	int 			LoadInstrument(CString FileName);

	// Sequences functions
	// // // take instrument type as parameter rather than chip type
	CSequence*		GetSequence(inst_type_t InstType, unsigned int Index, int Type);
	CSequence*		GetSequence(inst_type_t InstType, unsigned int Index, int Type) const;		// // //
	unsigned int	GetSequenceItemCount(inst_type_t InstType, unsigned int Index, int Type) const;		// // //
	int				GetFreeSequence(inst_type_t InstType, int Type) const;		// // //
	int				GetSequenceCount(int Type) const;

	// DPCM samples
	CDSample*		GetSample(unsigned int Index);
	const CDSample*	GetSample(unsigned int Index) const;
	bool			IsSampleUsed(unsigned int Index) const;
	unsigned int	GetSampleCount() const;
	int				GetFreeSampleSlot() const;
	void			RemoveSample(unsigned int Index);
	unsigned int	GetTotalSampleSize() const;

	// Other
	unsigned int	ScanActualLength(unsigned int Track, unsigned int Count) const;		// // //
	double			GetStandardLength(int Track, unsigned int ExtraLoops) const;		// // //
	unsigned int	GetFirstFreePattern(unsigned int Track, unsigned int Channel) const;		// // //

	// Operations
	void			RemoveUnusedInstruments();
	void			RemoveUnusedSamples();		// // //
	void			RemoveUnusedPatterns();
	void			MergeDuplicatedPatterns();
	void			PopulateUniquePatterns();		// // //
	void			SwapInstruments(int First, int Second);

	// For file version compability
	static void		ConvertSequence(stSequence *pOldSequence, CSequence *pNewSequence, int Type);

	bool			GetExceededFlag() { return m_bExceeded; };
	void			SetExceededFlag(bool Exceed = 1);		// // //

	// Constants
public:
	static const char*	DEFAULT_TRACK_NAME;
	static const int	DEFAULT_ROW_COUNT;
	static const char*	NEW_INST_NAME;

	static const int	DEFAULT_NAMCO_CHANS;

	static const int	DEFAULT_FIRST_HIGHLIGHT;
	static const int	DEFAULT_SECOND_HIGHLIGHT;
	static const stHighlight DEFAULT_HIGHLIGHT;		// // //

	static const bool	DEFAULT_LINEAR_PITCH;


	//
	// Private functions
	//
private:

	//
	// File management functions (load/save)
	//

	void			CreateEmpty();

	BOOL			SaveDocument(LPCTSTR lpszPathName) const;
	BOOL			OpenDocument(LPCTSTR lpszPathName);

	BOOL			OpenDocumentOld(CFile *pOpenFile);
	BOOL			OpenDocumentNew(CDocumentFile &DocumentFile);

	bool			WriteBlocks(CDocumentFile *pDocFile) const;
	bool			WriteBlock_Parameters(CDocumentFile *pDocFile) const;
	bool			WriteBlock_SongInfo(CDocumentFile *pDocFile) const;
	bool			WriteBlock_Header(CDocumentFile *pDocFile) const;
	bool			WriteBlock_Instruments(CDocumentFile *pDocFile) const;
	bool			WriteBlock_Sequences(CDocumentFile *pDocFile) const;
	bool			WriteBlock_Frames(CDocumentFile *pDocFile) const;
	bool			WriteBlock_Patterns(CDocumentFile *pDocFile) const;
	bool			WriteBlock_DSamples(CDocumentFile *pDocFile) const;
	bool			WriteBlock_Comments(CDocumentFile *pDocFile) const;
	bool			WriteBlock_ChannelLayout(CDocumentFile *pDocFile) const;
	bool			WriteBlock_SequencesVRC6(CDocumentFile *pDocFile) const;
	bool			WriteBlock_SequencesN163(CDocumentFile *pDocFile) const;
	bool			WriteBlock_SequencesS5B(CDocumentFile *pDocFile) const;

	bool			ReadBlock_Parameters(CDocumentFile *pDocFile);
	bool			ReadBlock_Header(CDocumentFile *pDocFile);
	bool			ReadBlock_Instruments(CDocumentFile *pDocFile);
	bool			ReadBlock_Sequences(CDocumentFile *pDocFile);
	bool			ReadBlock_Frames(CDocumentFile *pDocFile);
	bool			ReadBlock_Patterns(CDocumentFile *pDocFile);
	bool			ReadBlock_DSamples(CDocumentFile *pDocFile);
	bool			ReadBlock_Comments(CDocumentFile *pDocFile);
	bool			ReadBlock_ChannelLayout(CDocumentFile *pDocFile);
	bool			ReadBlock_SequencesVRC6(CDocumentFile *pDocFile);
	bool			ReadBlock_SequencesN163(CDocumentFile *pDocFile);
	bool			ReadBlock_SequencesS5B(CDocumentFile *pDocFile);

	// // //
	bool			WriteBlock_DetuneTables(CDocumentFile *pDocFile) const;
	bool			ReadBlock_DetuneTables(CDocumentFile *pDocFile);
	bool			WriteBlock_Grooves(CDocumentFile *pDocFile) const;
	bool			ReadBlock_Grooves(CDocumentFile *pDocFile);
	bool			WriteBlock_Bookmarks(CDocumentFile *pDocFile) const;
	bool			ReadBlock_Bookmarks(CDocumentFile *pDocFile);
	// For file version compability
	void			ReorderSequences();
	void			ConvertSequences();

#ifdef AUTOSAVE
	void			SetupAutoSave();
	void			ClearAutoSave();
#endif

	//
	// Internal module operations
	//

	void			AllocateTrack(unsigned int Song);
	CPatternData*	GetTrack(unsigned int Track);
	CPatternData*	GetTrack(unsigned int Track) const;
	void			SwapTracks(unsigned int Track1, unsigned int Track2);

	void			SetupChannels(unsigned char Chip);
	void			ApplyExpansionChip();



	//
	// Private variables
	//
private:

	//
	// Interface variables
	//

	// Channels (TODO: run-time state, remove or move these?)
	CTrackerChannel	*m_pChannels[CHANNELS];
	int				m_iRegisteredChannels;
	int				m_iChannelTypes[CHANNELS];
	int				m_iChannelChip[CHANNELS];


	//
	// State variables
	//

	bool			m_bFileLoaded;			// Is a file loaded?
	bool			m_bFileLoadFailed;		// Last file load operation failed
	unsigned int	m_iFileVersion;			// Loaded file version

	bool			m_bForceBackup;
	bool			m_bBackupDone;
	bool			m_bExceeded;			// // //
#ifdef TRANSPOSE_FDS
	bool			m_bAdjustFDSArpeggio;
#endif

#ifdef AUTOSAVE
	// Auto save
	int				m_iAutoSaveCounter;
	CString			m_sAutoSaveFile;
#endif

	//
	// Document data
	//

	// Patterns and song data
	CPatternData	*m_pTracks[MAX_TRACKS];						// List of all tracks
	CString			m_sTrackNames[MAX_TRACKS];
	std::vector<stBookmark> *m_pBookmarkList[MAX_TRACKS];			// // // Bookmarks

	unsigned int	m_iTrackCount;								// Number of tracks added
	unsigned int	m_iChannelsAvailable;						// Number of channels added

	// Instruments, samples and sequences
	CInstrument		*m_pInstruments[MAX_INSTRUMENTS];
	CDSample		m_DSamples[MAX_DSAMPLES];					// The DPCM sample list
	CSequence		*m_pSequences2A03[MAX_SEQUENCES][SEQ_COUNT];
	CSequence		*m_pSequencesVRC6[MAX_SEQUENCES][SEQ_COUNT];
	CSequence		*m_pSequencesN163[MAX_SEQUENCES][SEQ_COUNT];
	CSequence		*m_pSequencesS5B[MAX_SEQUENCES][SEQ_COUNT];
	CGroove			*m_pGrooveTable[MAX_GROOVE];				// // // Grooves

	// Module properties
	unsigned char	m_iExpansionChip;							// Expansion chip
	unsigned int	m_iNamcoChannels;
	vibrato_t		m_iVibratoStyle;							// 0 = old style, 1 = new style
	bool			m_bLinearPitch;
	unsigned int	m_iMachine;									// NTSC / PAL
	unsigned int	m_iEngineSpeed;								// Refresh rate
	unsigned int	m_iSpeedSplitPoint;							// Speed/tempo split-point
	int				m_iDetuneTable[6][96];						// // // Detune tables

	// NSF info
	char			m_strName[32];								// Song name
	char			m_strArtist[32];							// Song artist
	char			m_strCopyright[32];							// Song copyright

	// Comments
	CString			m_strComment;
	bool			m_bDisplayComment;

	// Row highlight (TODO remove)
	stHighlight		m_vHighlight;								// // //

	// Things below are for compability with older files
	CArray<stSequence> m_vTmpSequences;
	CArray<stSequence[SEQ_COUNT]> m_vSequences;

	//
	// End of document data
	//

	// Thread synchronization
private:
	mutable CCriticalSection m_csInstrument;
	mutable CMutex			 m_csDocumentLock;

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	virtual void DeleteContents();
	virtual void SetModifiedFlag(BOOL bModified = 1);
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CFamiTrackerDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
};
