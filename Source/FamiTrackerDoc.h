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


// Synchronization objects
#include <afxmt.h>

#include <vector>
#include <string>		// !! !!
#include <memory>		// // //

// Get access to some APU constants
#include "APU/Types.h"
// Constants, types and enums
#include "FamiTrackerTypes.h"
// // //
#include "FTMComponentInterface.h"
#include "Settings.h"		// // //

#include "json/json.hpp" // !! !!

using json = nlohmann::json;

#define TRANSPOSE_FDS

// #define DISABLE_SAVE		// // //

// Default song settings
const machine_t    DEFAULT_MACHINE_TYPE		 = NTSC;
const unsigned int DEFAULT_SPEED_SPLIT_POINT = 32;
const unsigned int OLD_SPEED_SPLIT_POINT	 = 21;

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

// Dn-FT JSON block format version 1.1
// https://github.com/Dn-Programming-Core-Management/Dn-FamiTracker/wiki/JSON-block-format
struct stJSONOptionalData {
	// Device mixing offsets, described in centibels. too late to change to millibels.
	// range is +- 12 db.
	int16_t APU1_OFFSET = 0;
	int16_t APU2_OFFSET = 0;
	int16_t VRC6_OFFSET = 0;
	int16_t VRC7_OFFSET = 0;
	int16_t FDS_OFFSET = 0;
	int16_t MMC5_OFFSET = 0;
	int16_t N163_OFFSET = 0;
	int16_t S5B_OFFSET = 0;

	// Use hardware based mixing values derived from survey: https://forums.nesdev.org/viewtopic.php?f=2&t=17741
	bool USE_SURVEY_MIX = false;
};

// Access data types used by the document class
#include "PatternData.h"
#include "Instrument.h"
#include "Sequence.h"
#include "OldSequence.h"		// // //
#include "Groove.h"		// // //
#include "Bookmark.h"		// // //

#include "PatternEditorTypes.h"		// // //
// #include "FrameEditorTypes.h"		// // //

// External classes
class CTrackerChannel;
class CDocumentFile;
class stFullState;		// // //
class CSeqInstrument;		// // // TODO: move to instrument manager
class CDSample;		// // //

//
// I'll try to organize this class, things are quite messy right now!
//

class CFamiTrackerDoc : public CDocument, public CFTMComponentInterface
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
	static CFamiTrackerDoc* LoadImportFile(LPCTSTR lpszPathName);
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

	unsigned int	GetCurrentPatternLength(unsigned int Track, int Frame) const;		// // // moved from pattern editor

	unsigned int	GetEffColumns(unsigned int Track, unsigned int Channel) const;
	void			SetEffColumns(unsigned int Track, unsigned int Channel, unsigned int Columns);

	unsigned int 	GetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel) const;
	void			SetPatternAtFrame(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Pattern);

	bool			IsPatternEmpty(unsigned int Track, unsigned int Channel, unsigned int Pattern) const;
	bool			ArePatternsSame(unsigned int Track, unsigned int Channel, unsigned int Pattern1, unsigned int Pattern2) const;		// // //

	void			MakeKraid();				// // // Easter Egg

	// Pattern editing
	void			SetNoteData(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, const stChanNote *pData);
	void			GetNoteData(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, stChanNote *pData) const;

	void			SetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, const stChanNote *pData);
	void			GetDataAtPattern(unsigned int Track, unsigned int Pattern, unsigned int Channel, unsigned int Row, stChanNote *pData) const;

	void			ClearPatterns(unsigned int Track);
	void			ClearPattern(unsigned int Track, unsigned int Frame, unsigned int Channel);
	
	void			PopulateUniquePatterns(unsigned int Track);		// // //

	bool			InsertRow(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			ClearRow(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			ClearRowField(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row, cursor_column_t Column);
	bool			RemoveNote(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	bool			PullUp(unsigned int Track, unsigned int Frame, unsigned int Channel, unsigned int Row);
	void			CopyPattern(unsigned int Track, int Target, int Source, int Channel);

	void			SwapChannels(unsigned int Track, unsigned int First, unsigned int Second);		// // //

	// Frame editing
	bool			InsertFrame(unsigned int Track, unsigned int Frame);
	bool			RemoveFrame(unsigned int Track, unsigned int Frame);
	bool			DuplicateFrame(unsigned int Track, unsigned int Frame);
	bool			CloneFrame(unsigned int Track, unsigned int Frame);		// // // renamed
	bool			MoveFrameDown(unsigned int Track, unsigned int Frame);
	bool			MoveFrameUp(unsigned int Track, unsigned int Frame);
	bool			AddFrames(unsigned int Track, unsigned int Frame, int Count);		// // //
	bool			DeleteFrames(unsigned int Track, unsigned int Frame, int Count);		// // //

	// Global (module) data
	void			SetEngineSpeed(unsigned int Speed);
	void			SetMachine(machine_t Machine, bool Redraw);		// // //
	machine_t		GetMachine() const		{ return m_iMachine; };		// // //
	unsigned int	GetEngineSpeed() const	{ return m_iEngineSpeed; };
	unsigned int	GetFrameRate() const;
	void			SetPlaybackRate(unsigned int Rate, unsigned int Type);
	unsigned int	GetPlaybackRate() const;
	unsigned int	GetPlaybackRateType() const { return m_iPlaybackRateType; };

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

	bool			GetSurveyMixCheck() const;
	void			SetSurveyMixCheck(bool SurveyMix);

	int16_t			GetLevelOffset(int device) const;
	void			SetLevelOffset(int device, int16_t offset);
	void			ResetLevelOffset();

	uint8_t			GetOPLLPatchByte(int index) const;
	void			SetOPLLPatchByte(int index, uint8_t data);
	void			ResetOPLLPatches();
	void			SetOPLLPatchSet(int patchset);

	std::string		GetOPLLPatchName(int index) const;
	void			SetOPLLPatchName(int index, std::string PatchName);

	bool			GetExternalOPLLChipCheck() const;
	void			SetExternalOPLLChipCheck(bool UserDefined);

	json			InterfaceToOptionalJSON() const;
	void			OptionalJSONToInterface(json& json);

	void			SetComment(CString &comment, bool bShowOnLoad);
	CString			GetComment() const;
	bool			ShowCommentOnOpen() const;

	void			SetSpeedSplitPoint(int SplitPoint);
	int				GetSpeedSplitPoint() const;

	void			SetHighlight(unsigned int Track, const stHighlight Hl);		// // //
	stHighlight		GetHighlight(unsigned int Track) const;

	void			SetHighlight(const stHighlight Hl);		// // //
	stHighlight		GetHighlight() const;
	stHighlight		GetHighlightAt(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //
	unsigned int	GetHighlightState(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //
	CBookmark*		GetBookmarkAt(unsigned int Track, unsigned int Frame, unsigned int Row) const;		// // //

	void			SetDetuneOffset(int Chip, int Note, int Detune);		// // //
	int				GetDetuneOffset(int Chip, int Note) const;
	void			ResetDetuneTables();
	void			SetTuning(int Semitone, int Cent);		// // // 050B
	int				GetTuningSemitone() const;		// // // 050B
	int				GetTuningCent() const;		// // // 050B

	CGroove			*GetGroove(int Index) const;		// // //
	void			SetGroove(int Index, const CGroove* Groove);

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
	std::shared_ptr<CInstrument>	GetInstrument(unsigned int Index) const;
	unsigned int	GetInstrumentCount() const;
	bool			IsInstrumentUsed(unsigned int Index) const;
	int				AddInstrument(const char *pName, int ChipType);					// Add a new instrument
	int				AddInstrument(CInstrument *pInstrument);
	void			AddInstrument(CInstrument *pInstrument, unsigned int Slot);
	void			RemoveInstrument(unsigned int Index);							// Remove an instrument
	void			SetInstrumentName(unsigned int Index, const char *pName);		// Set the name of an instrument
	void			GetInstrumentName(unsigned int Index, char *pName) const;		// Get the name of an instrument
	int				CloneInstrument(unsigned int Index);							// Create a copy of an instrument
	inst_type_t		GetInstrumentType(unsigned int Index) const;
	int				DeepCloneInstrument(unsigned int Index);
	void			SaveInstrument(unsigned int Index, CString FileName) const;
	int 			LoadInstrument(CString FileName);

	// Sequences functions
	// // // take instrument type as parameter rather than chip type
	CSequence*		GetSequence(inst_type_t InstType, unsigned int Index, int Type) const;		// // //
	unsigned int	GetSequenceItemCount(inst_type_t InstType, unsigned int Index, int Type) const;		// // //
	int				GetFreeSequence(inst_type_t InstType, int Type, CSeqInstrument *pInst = nullptr) const;		// // //
	int				GetSequenceCount(inst_type_t InstType, int Type) const;		// // //

	// DPCM samples
	const CDSample*	GetSample(unsigned int Index) const;		// // // non-const getter removed
	void			SetSample(unsigned int Index, CDSample *pSamp);		// // //
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
	void			SwapInstruments(int First, int Second);
	stFullState*	RetrieveSoundState(unsigned int Track, unsigned int Frame, unsigned int Row, int Channel);		// // //

	// For file version compability
	static void		ConvertSequence(stSequence *pOldSequence, CSequence *pNewSequence, int Type);

	bool			GetExceededFlag() { return m_bExceeded; };
	void			SetExceededFlag(bool Exceed = 1);		// // //

	// // // from the component interface
	CSequenceManager *const GetSequenceManager(int InstType) const;
	CInstrumentManager *const GetInstrumentManager() const;
	CDSampleManager *const GetDSampleManager() const;
	CBookmarkManager *const GetBookmarkManager() const;
	void			Modify(bool Change);
	void			ModifyIrreversible();

	// Constants
public:
	static const char*	NEW_INST_NAME;

	static const int	DEFAULT_NAMCO_CHANS;

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

	bool			WriteBlock_Parameters(CDocumentFile *pDocFile, const int Version) const;		// // // version
	bool			WriteBlock_SongInfo(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_Tuning(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_Header(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_Instruments(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_Sequences(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_Frames(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_Patterns(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_DSamples(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_Comments(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_ChannelLayout(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_SequencesVRC6(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_SequencesN163(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_SequencesS5B(CDocumentFile *pDocFile, const int Version) const;
	// // //
	bool			WriteBlock_ParamsExtra(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_DetuneTables(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_Grooves(CDocumentFile *pDocFile, const int Version) const;
	bool			WriteBlock_Bookmarks(CDocumentFile *pDocFile, const int Version) const;
	// !! !!
	bool			WriteBlock_JSON(CDocumentFile * pDocFile, const int Version) const;
	bool			WriteBlock_ParamsEmu(CDocumentFile* pDocFile, const int Version) const;

	void			ReadBlock_Parameters(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_SongInfo(CDocumentFile *pDocFile, const int Version);		// // //
	void			ReadBlock_Tuning(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_Header(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_Instruments(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_Sequences(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_Frames(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_Patterns(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_DSamples(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_Comments(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_ChannelLayout(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_SequencesVRC6(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_SequencesN163(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_SequencesS5B(CDocumentFile *pDocFile, const int Version);
	// // //
	void			ReadBlock_ParamsExtra(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_DetuneTables(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_Grooves(CDocumentFile *pDocFile, const int Version);
	void			ReadBlock_Bookmarks(CDocumentFile *pDocFile, const int Version);
	// !! !!
	void			ReadBlock_JSON(CDocumentFile * pDocFile, const int Version);
	void			ReadBlock_ParamsEmu(CDocumentFile* pDocFile, const int Version);

	// For file version compability
	void			ReorderSequences();

	/*!	\brief Validates a given condition and throws an exception otherwise.
		\details This method replaces the previous ASSERT_FILE_DATA preprocessor macro.
		\param Cond The condition to check against.
		\param Msg The error message.
	*/
	template <module_error_level_t l = MODULE_ERROR_DEFAULT>
	void			AssertFileData(bool Cond, std::string Msg) const;		// // //
	
	template <module_error_level_t l = MODULE_ERROR_DEFAULT, typename T, typename U, typename V>
	typename std::enable_if<std::is_unsigned<T>::value, T>::type
	AssertRange(T Value, U Min, V Max, std::string Desc) const
	{
		try {
			return CModuleException::AssertRangeFmt<l>(Value, Min, Max, Desc, "%u");
		}
		catch (CModuleException *e) {
			if (m_pCurrentDocument)
				m_pCurrentDocument->SetDefaultFooter(e);
			throw;
		}
	}
	
	template <module_error_level_t l = MODULE_ERROR_DEFAULT, typename T, typename U, typename V>
	typename std::enable_if<std::is_signed<T>::value, T>::type
	AssertRange(T Value, U Min, V Max, std::string Desc) const
	{
		try {
			return CModuleException::AssertRangeFmt<l>(Value, Min, Max, Desc, "%i");
		}
		catch (CModuleException *e) {
			if (m_pCurrentDocument)
				m_pCurrentDocument->SetDefaultFooter(e);
			throw;
		}
	}

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
	int				GetChannelPosition(int Channel, unsigned char Chip);		// // //

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
	bool			m_bFileDnModule;		// Is a Dn-FT module (non-vanilla)

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

	unsigned int	m_iTrackCount;								// Number of tracks added
	unsigned int	m_iChannelsAvailable;						// Number of channels added

	// Instruments, samples and sequences
	CInstrumentManager *m_pInstrumentManager;					// // //
	CBookmarkManager *m_pBookmarkManager;						// // //
	CGroove			*m_pGrooveTable[MAX_GROOVE];				// // // Grooves

	// Module properties
	unsigned char	m_iExpansionChip;							// Expansion chip
	unsigned int	m_iNamcoChannels;
	vibrato_t		m_iVibratoStyle;							// 0 = old style, 1 = new style
	bool			m_bLinearPitch;

	machine_t		m_iMachine;									// // // NTSC / PAL
	unsigned int	m_iEngineSpeed;								// Engine refresh rate, in Hz
	unsigned int	m_iSpeedSplitPoint;							// Speed/tempo split-point
	int				m_iDetuneTable[6][96];						// // // Detune tables
	int				m_iDetuneSemitone, m_iDetuneCent;			// // // 050B tuning
	unsigned int	m_iPlaybackRate;							// NSF playback rate, in microseconds
	unsigned int	m_iPlaybackRateType;						// Playback rate type, 0 = default, 1 = custom, 2 = video

	// Emulation properties
	bool			m_bUseExternalOPLLChip;						// !! !! User-defined hardware patch set for OPLL
	uint8_t			m_iOPLLPatchBytes[19 * 8];
	std::string		m_strOPLLPatchNames[19];

	// JSON Optional properties
	int16_t			m_iDeviceLevelOffset[8];					// !! !! Device level offsets, described in centibels
	bool			m_bUseSurveyMixing;							// !! !! Use hardware-based mixing values, derived from survey: https://forums.nesdev.org/viewtopic.php?f=2&t=17741

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
	std::vector<COldSequence> m_vTmpSequences;		// // //

	mutable CDocumentFile *m_pCurrentDocument;		// // //

	//
	// End of document data
	//

	// Thread synchronization
private:
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
