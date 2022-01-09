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

//
// This thread will take care of the NES sound generation
//

#include <afxmt.h>		// Synchronization objects
#include <queue>		// // //
#include "Common.h"

#include <atomic>
#include <memory>
#include <mutex>

const int VIBRATO_LENGTH = 256;
const int TREMOLO_LENGTH = 256;

// Custom messages
enum { 
	WM_USER_SILENT_ALL = WM_USER + 1,
	WM_USER_LOAD_SETTINGS,
	WM_USER_PLAY,
	WM_USER_STOP,
	WM_USER_RESET,
	WM_USER_START_RENDER,
	WM_USER_STOP_RENDER,
	WM_USER_PREVIEW_SAMPLE,
	WM_USER_WRITE_APU,
	WM_USER_CLOSE_SOUND,
	WM_USER_SET_CHIP,
	WM_USER_VERIFY_EXPORT,
	WM_USER_REMOVE_DOCUMENT
};

// Player modes
enum play_mode_t {
	MODE_PLAY,				// Play from top of pattern
	MODE_PLAY_START,		// Play from start of song
	MODE_PLAY_REPEAT,		// Play and repeat
	MODE_PLAY_CURSOR,		// Play from cursor
	MODE_PLAY_FRAME,		// Play frame
	MODE_PLAY_MARKER,		// // // 050B (row marker, aka "bookmark")
};

enum render_end_t { 
	SONG_TIME_LIMIT, 
	SONG_LOOP_LIMIT 
};

class stChanNote;		// // //
struct stRecordSetting;

enum note_prio_t;

class CChannelHandler;
class CFamiTrackerView;
class CFamiTrackerDoc;
class CInstrument;		// // //
class CSequence;		// // //
class CAPU;
class CDSound;
class CDSoundChannel;
class CWaveFile;		// // //
class CVisualizerWnd;
class CDSample;
class CTrackerChannel;
class CFTMComponentInterface;		// // //
class CInstrumentRecorder;		// // //
class CRegisterState;		// // //

// CSoundGen

class CSoundGen : public CWinThread, IAudioCallback
{
protected:
	DECLARE_DYNCREATE(CSoundGen)
public:
	CSoundGen();
	virtual ~CSoundGen();

private:		// // //
	CInstrumentRecorder *m_pInstRecorder;

	//
	// Public functions
	//
public:

	// One time initialization 
	void		AssignDocument(CFamiTrackerDoc *pDoc);
	void		AssignView(CFamiTrackerView *pView);
	void		RemoveDocument();
	void		SetVisualizerWindow(CVisualizerWnd *pWnd);

	// Multiple times initialization
	void		RegisterChannels(int Chip, CFamiTrackerDoc *pDoc);
	void		SelectChip(int Chip);
	void		LoadMachineSettings();		// // // 050B

	// Sound
	bool		InitializeSound(HWND hWnd);
	void		FlushBuffer(int16_t const * pBuffer, uint32_t Size);
	CDSound		*GetSoundInterface() const { return m_pDSound; };

	void		Interrupt() const;
	bool		GetSoundTimeout() const;
	bool		IsBufferUnderrun();
	bool		IsAudioClipping();

	bool		WaitForStop() const;
	bool		IsRunning() const;

	CChannelHandler *GetChannel(int Index) const;

	void		DocumentPropertiesChanged(CFamiTrackerDoc *pDocument);

public:
	// Vibrato
	void		 GenerateVibratoTable(vibrato_t Type);
	void		 SetupVibratoTable(vibrato_t Type);
	int			 ReadVibratoTable(int index) const;
	int			 ReadPeriodTable(int Index, int Table) const;		// // //

	// Player interface
	void		 StartPlayer(play_mode_t Mode, int Track);	
	void		 StopPlayer();
	void		 ResetPlayer(int Track);
	void		 LoadSettings();
	void		 SilentAll();

	void		 ResetState();
	void		 ResetTempo();
	void		 SetHighlightRows(int Rows);		// // //
	float		 GetTempo() const;
	float		 GetAverageBPM() const;		// // //
	float		 GetCurrentBPM() const;		// // //
	bool		 IsPlaying() const { return m_bPlaying; };

	// Stats
	unsigned int GetUnderruns() const;
	unsigned int GetFrameRate();

	// Tracker playing
	void		 SetJumpPattern(int Pattern);
	void		 SetSkipRow(int Row);
	void		 EvaluateGlobalEffects(stChanNote *NoteData, int EffColumns);

	stDPCMState	 GetDPCMState() const;
	int			 GetChannelVolume(int Channel) const;		// // //

	// Rendering
	bool		 RenderToFile(LPTSTR pFile, render_end_t SongEndType, int SongEndParam, int Track);
	void		 StopRendering();
	void		 GetRenderStat(int &Frame, int &Time, bool &Done, int &FramesToRender, int &Row, int &RowCount) const;
	bool		 IsRendering() const;	
	bool		 IsBackgroundTask() const;

	// Sample previewing
	void		 PreviewSample(const CDSample *pSample, int Offset, int Pitch);		// // //
	void		 CancelPreviewSample();
	bool		 PreviewDone() const;

	void		 WriteAPU(int Address, char Value);

	// Used by channels

	/** Add cycles to model execution time (capped at vblank). */
	void		AddCyclesUnlessEndOfFrame(int Count);

	// Other
	uint8_t		GetReg(int Chip, int Reg) const;
	CRegisterState *GetRegState(unsigned Chip, unsigned Reg) const;		// // //
	double		GetChannelFrequency(unsigned Chip, int Channel) const;		// // //
	double		GetFDSModFrequency() const;		// // //
	CString		RecallChannelState(int Channel) const;		// // //

	// FDS & N163 wave preview
	void		WaveChanged();
	bool		HasWaveChanged() const;
	void		ResetWaveChanged();

	void		WriteRegister(uint16_t Reg, uint8_t Value);

	void		RegisterKeyState(int Channel, int Note);
	void		SetNamcoMixing(bool bLinear);			// // //

	// Player
	int			GetPlayerRow() const;
	int			GetPlayerFrame() const;
	int			GetPlayerTrack() const;
	int			GetPlayerTicks() const;
	void		QueueNote(int Channel, stChanNote &NoteData, note_prio_t Priority) const;
	void		ForceReloadInstrument(int Channel);		// // //
	void		MoveToFrame(int Frame);
	void		SetQueueFrame(int Frame);
	int			GetQueueFrame() const;

	// // // Instrument recorder
	CInstrument		*GetRecordInstrument() const;
	void			ResetDumpInstrument();
	int				GetRecordChannel() const;
	void			SetRecordChannel(int Channel);
	stRecordSetting *GetRecordSetting() const;
	void			SetRecordSetting(stRecordSetting *Setting);

	bool HasDocument() const { return m_pDocument != NULL; };
	CFamiTrackerDoc *GetDocument() const { return m_pDocument; };
	CFTMComponentInterface *GetDocumentInterface() const;

	// Sequence play position
	void SetSequencePlayPos(const CSequence *pSequence, int Pos);
	int GetSequencePlayPos(const CSequence *pSequence);

	void SetMeterDecayRate(int Type) const;		// // // 050B
	int GetMeterDecayRate() const;		// // // 050B

	int GetDefaultInstrument() const;

	// 
	// Private functions
	//
private:
	// Internal initialization
	void		CreateChannels();
	void		AssignChannel(CTrackerChannel *pTrackerChannel);		// // //
	void		ResetAPU();

	// Audio
	bool		ResetAudioDevice();
	void		CloseAudioDevice();
	void		CloseAudio();
	template<class T, int SHIFT> void FillBuffer(int16_t const * pBuffer, uint32_t Size);
	bool		PlayBuffer();

	// Player
	void		UpdateChannels();
	void		UpdateAPU();
	void		UpdatePlayer();
	void		PlayChannelNotes();
	void	 	PlayNote(int Channel, stChanNote *NoteData, int EffColumns);
	void		RunFrame();
	void		CheckControl();
	void		ResetBuffer();
	void		BeginPlayer(play_mode_t Mode, int Track);
	void		HaltPlayer();
	void		MakeSilent();
	void		SetupSpeed();

	// Misc
	void		PlaySample(const CDSample *pSample, int Offset, int Pitch);
	
	// Player
	void		ReadPatternRow();
	void		PlayerStepRow();
	void		PlayerStepFrame();
	void		PlayerJumpTo(int Frame);
	void		PlayerSkipTo(int Row);

	void		ApplyGlobalState();		// // //

public:
	static const double NEW_VIBRATO_DEPTH[];
	static const double OLD_VIBRATO_DEPTH[];

	static const int AUDIO_TIMEOUT = 2000;		// 2s buffer timeout

	//
	// Private variables
	//
private:
	// Objects
	CChannelHandler		*m_pChannels[CHANNELS];
	CTrackerChannel		*m_pTrackerChannels[CHANNELS];
	CFamiTrackerDoc		*m_pDocument;
	CFamiTrackerView	*m_pTrackerView;

	// Sound
	CDSound				*m_pDSound;
	CDSoundChannel		*m_pDSoundChannel;
	CVisualizerWnd		*m_pVisualizerWnd;
	CAPU				*m_pAPU;
	int currN163LevelOffset;

	const CDSample		*m_pPreviewSample;

	bool				m_bRunning;

	// Thread synchronization
private:
	mutable std::mutex m_csAPULock;		// // //
	mutable CCriticalSection m_csVisualizerWndLock;

	// Handles
	HANDLE				m_hInterruptEvent;					// Used to interrupt sound buffer syncing

// Sound variables (TODO: move sound to a new class?)
private:
	unsigned int		m_iSampleSize;						// Size of samples, in bits
	unsigned int		m_iBufSizeSamples;					// Buffer size in samples
	unsigned int		m_iBufSizeBytes;					// Buffer size in bytes
	unsigned int		m_iBufferPtr;						// This will point in samples
	char				*m_pAccumBuffer;
	short				*m_iGraphBuffer;
	int					m_iAudioUnderruns;					// Keep track of underruns to inform user
	bool				m_bBufferTimeout;
	bool				m_bBufferUnderrun;
	bool				m_bAudioClipping;
	int					m_iClipCounter;
	
// Tracker playing variables
private:
	unsigned int		m_iTempo;							// Tempo and speed
	unsigned int		m_iSpeed;							
	int					m_iGrooveIndex;						// // // Current groove
	unsigned int		m_iGroovePosition;					// // // Groove position
	int					m_iTempoAccum;						// Used for speed calculation
	unsigned int		m_iPlayTicks;
	bool				m_bPlaying;							// True when tracker is playing back the module
	std::atomic<bool>	m_bHaltRequest;						// True when a halt is requested
	bool				m_bPlayLooping;
	int					m_iFrameCounter;

	int					m_iUpdateCycles;					// Number of cycles/APU update
	int					m_iConsumedCycles;					// Cycles consumed by the update registers functions

	unsigned int		m_iSpeedSplitPoint;					// Speed/tempo split point fetched from the module
	unsigned int		m_iFrameRate;						// Module frame rate
	int					m_iLastHighlight;					// // //

	bool				m_bUpdatingAPU;						// // //

	// Play control
	int					m_iJumpToPattern;
	int					m_iSkipToRow;
	bool				m_bDoHalt;							// // // Cxx effect
	int					m_iStepRows;						// # of rows skipped last update
	int					m_iRowTickCount;					// // // 050B
	play_mode_t			m_iPlayMode;

	unsigned int		m_iNoteLookupTableNTSC[96];			// For 2A03
	unsigned int		m_iNoteLookupTablePAL[96];			// For 2A07
	unsigned int		m_iNoteLookupTableSaw[96];			// For VRC6 sawtooth
	unsigned int		m_iNoteLookupTableVRC7[12];			// // // For VRC7
	unsigned int		m_iNoteLookupTableFDS[96];			// For FDS
	unsigned int		m_iNoteLookupTableN163[96];			// For N163
	unsigned int		m_iNoteLookupTableS5B[96];			// // // For 5B, internal use only
	int					m_iVibratoTable[VIBRATO_LENGTH];

	machine_t			m_iMachineType;						// // // NTSC/PAL

	// Rendering
	bool				m_bRequestRenderStart;
	bool				m_bRendering;
	bool				m_bRequestRenderStop;
	bool				m_bStoppingRender;					// // //
	render_end_t		m_iRenderEndWhen;
	unsigned int		m_iRenderEndParam;
	int					m_iDelayedStart;
	int					m_iDelayedEnd;
	int					m_iRenderTrack;
	unsigned int		m_iRenderRowCount;
	int					m_iRenderRow;

	int					m_iTempoDecrement;
	int					m_iTempoRemainder;
	bool				m_bUpdateRow;

	static const int	AVERAGE_BPM_SIZE = 24;		// // // 050B
	float				m_fBPMCacheValue[AVERAGE_BPM_SIZE];
	int					m_iBPMCacheTicks[AVERAGE_BPM_SIZE];
	int					m_iBPMCachePosition;

	std::queue<int>		m_iRegisterStream;					// // // vgm export

	std::unique_ptr<CWaveFile> m_pWaveFile;

	// FDS & N163 waves
	volatile bool		m_bWaveChanged;
	volatile bool		m_bInternalWaveChanged;

	// Player state
	int					m_iQueuedFrame;					// Queued frame
	int					m_iPlayTrack;					// Current track that is playing
	int					m_iPlayFrame;					// Current frame to play
	int					m_iPlayRow;						// Current row to play
	bool				m_bDirty;						// Row/frame has changed
	unsigned int		m_iFramesPlayed;				// Total number of frames played since start
	unsigned int		m_iRowsPlayed;					// Total number of rows played since start
	bool				m_bFramePlayed[MAX_FRAMES];		// true for each frame played

	// Sequence play visualization
	const CSequence		*m_pSequencePlayPos;
	int					m_iSequencePlayPos;
	int					m_iSequenceTimeout;

	// Overloaded functions
public:
	virtual BOOL InitInstance();
	virtual int	 ExitInstance();
	virtual BOOL OnIdle(LONG lCount);

	// Implementation
public:
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSilentAll(WPARAM wParam, LPARAM lParam);
	afx_msg void OnLoadSettings(WPARAM wParam, LPARAM lParam);
	afx_msg void OnStartPlayer(WPARAM wParam, LPARAM lParam);
	afx_msg void OnStopPlayer(WPARAM wParam, LPARAM lParam);
	afx_msg void OnResetPlayer(WPARAM wParam, LPARAM lParam);
	afx_msg void OnStartRender(WPARAM wParam, LPARAM lParam);
	afx_msg void OnStopRender(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPreviewSample(WPARAM wParam, LPARAM lParam);
	afx_msg void OnWriteAPU(WPARAM wParam, LPARAM lParam);
	afx_msg void OnCloseSound(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSetChip(WPARAM wParam, LPARAM lParam);
	afx_msg void OnRemoveDocument(WPARAM wParam, LPARAM lParam);

public:
	std::unique_lock<std::mutex> Lock() {
		return std::unique_lock<std::mutex>(m_csAPULock);
	}

	std::unique_lock<std::mutex> DeferLock() {
		return std::unique_lock<std::mutex>(m_csAPULock, std::defer_lock);
	}
};
